// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INK_GEOMETRY_MODELED_SHAPE_H_
#define INK_GEOMETRY_MODELED_SHAPE_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/inlined_vector.h"
#include "absl/functional/function_ref.h"
#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/span.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/static_rtree.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_packing_types.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/quad.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/segment.h"
#include "ink/geometry/triangle.h"

namespace ink {

// A triangulated shape, consisting of zero or more non-empty meshes, which may
// be indexed for faster geometric queries. These meshes are divided among zero
// or more "render groups"; all the meshes in a render group must have the same
// format. This also optionally carries one or more "outlines", which are
// (potentially incomplete) traversals of the vertices in the meshes, which
// could be used e.g. for path-based rendering.
//
// The spatial index is lazily initialized on the first call that requires it,
// as indicated in method comments. It may also be pre-emptively initialized via
// `InitializeSpatialIndex`; you might choose to do this to reduce the burden on
// a performance critical thread, because it's a relatively expensive operation
// and it acquires a mutex lock while initialization is in-progress. Note that
// non-member functions `Distance` and `Intersects` will also initialize the
// spatial index.
//
// `ModeledShape` stores its data in `std::shared_ptr`s; making a copy only
// involves copying the `std::shared_ptr`s, making them very cheap.
class ModeledShape {
 public:
  // A pair of indices identifying a point in an outline, by referring
  // to a vertex in one of the `Mesh`es.
  struct VertexIndexPair {
    // The index of the `Mesh` that the vertex belongs to.
    uint16_t mesh_index;
    // The index of the vertex within the `Mesh`.
    uint16_t vertex_index;
  };

  // A pair of indices identifying a triangle in one of the `Mesh`es.
  struct TriangleIndexPair {
    // The index of the `Mesh` that the triangle belongs to.
    uint16_t mesh_index;
    // The index of the triangle within the `Mesh`.
    uint16_t triangle_index;
  };

  // One render group for a `ModeledShape`, expressed using `MutableMesh`.
  struct MutableMeshGroup {
    // TODO: b/295166196 - Once `MutableMesh` always uses 16-bit indices, change
    // this field to `absl::Span<const MutableMesh> meshes` (and change the type
    // of `outlines` to use `VertexIndexPair`).
    absl::Nonnull<const MutableMesh*> mesh;
    absl::Span<const absl::Span<const uint32_t>> outlines;
    // A list of mesh attributes present in the `MutableMesh` that should be
    // stripped out during construction of the `ModeledShape`.
    absl::Span<const MeshFormat::AttributeId> omit_attributes;
    absl::Span<const std::optional<MeshAttributeCodingParams>> packing_params;
  };

  // One render group for a `ModeledShape`, expressed using `Mesh`.
  struct MeshGroup {
    absl::Span<const Mesh> meshes;
    // An optional list of outlines. The `mesh_index` of each `VertexIndexPair`
    // is an index into to the `meshes` span of this particular `MeshGroup`.
    absl::Span<const absl::Span<const VertexIndexPair>> outlines;
  };

  // Constructs an empty shape. Note that, since `ModeledShape` is read-only,
  // you can't do much with an empty shape. See `ModeledShape::FromMutableMesh`
  // and `ModeledShape::FromMeshes` for creating non-empty shapes.
  ModeledShape() = default;

  // Constructs a `ModeledShape` with no meshes, and the given number of render
  // groups (which will each be empty).
  static ModeledShape WithEmptyGroups(uint32_t num_groups);

  // Constructs a `ModeledShape` from a `MutableMesh`, fetching the
  // (non-mutable) `Mesh`es via `mesh.AsMeshes()`. `outlines`, if given, should
  // contain spans of indices into `mesh`, each describing an outline.
  // `packing_params`, if given, will be used instead of the default
  // MeshAttributeCodingParams. Returns an error if:
  // - `mesh` is empty
  // - `mesh.AsMeshes()` fails.
  // - `outlines` contains any index >= `mesh.VertexCount()`
  // TODO: b/295166196 - Once `MutableMesh` always uses 16-bit indices, this can
  // be replaced with a "FromMutableMeshes" factory method analogous to
  // `FromMeshes`.
  static absl::StatusOr<ModeledShape> FromMutableMesh(
      const MutableMesh& mesh,
      absl::Span<const absl::Span<const uint32_t>> outlines = {},
      absl::Span<const MeshFormat::AttributeId> omit_attributes = {},
      absl::Span<const std::optional<MeshAttributeCodingParams>>
          packing_params = {});

  // Constructs a `ModeledShape` with zero or more render groups. Returns an
  // error if:
  // - Any group contains an empty mesh.
  // - `AsMeshes()` fails for any of the meshes.
  // - The total number of `Mesh` objects post-`AsMeshes()` across all groups is
  //   more than 65536 (2^16).
  // - Any outline contains any element that does not correspond to a vertex.
  static absl::StatusOr<ModeledShape> FromMutableMeshGroups(
      absl::Span<const MutableMeshGroup> groups);

  // Constructs a `ModeledShape` from a span of `Mesh`es. `outlines`, if given,
  // should contain spans of `VertexIndexPair`s, each describing an
  // outline. Returns an error if:
  // - `meshes` contains more than 65536 (2^16) elements
  // - any element of `meshes` is empty
  // - any element of `meshes` has a different `MeshFormat` from the others
  // - `outlines` contains any element that does not correspond to a
  //    mesh or vertex.
  static absl::StatusOr<ModeledShape> FromMeshes(
      absl::Span<const Mesh> meshes,
      absl::Span<const absl::Span<const VertexIndexPair>> outlines = {});

  // Constructs a `ModeledShape` with zero or more render groups. Returns an
  // error if:
  // - Any group contains a mesh that is empty.
  // - Any group contains two meshes with different `MeshFormat`s.
  // - The total number of meshes across all groups is more than 65536 (2^16).
  // - Any outline contains any element that does not correspond to a mesh or
  //   vertex.
  static absl::StatusOr<ModeledShape> FromMeshGroups(
      absl::Span<const MeshGroup> groups);

  ModeledShape(const ModeledShape&) = default;
  ModeledShape(ModeledShape&&) = default;
  ModeledShape& operator=(const ModeledShape&) = default;
  ModeledShape& operator=(ModeledShape&&) = default;

  // Returns the number of render groups in this modeled shape.
  uint32_t RenderGroupCount() const;

  // Returns the format used for the meshes in render group `group_index`.
  //
  // This method CHECK-fails if `group_index` >= `RenderGroupCount()`.
  const MeshFormat& RenderGroupFormat(uint32_t group_index) const;

  // Returns the meshes that make up render group `group_index`, listed in
  // z-order (the first mesh in the span should be rendered on bottom; the last
  // mesh should be rendered on top).
  //
  // This method CHECK-fails if `group_index` >= `RenderGroupCount()`.
  absl::Span<const Mesh> RenderGroupMeshes(uint32_t group_index) const;

  // Returns the set of all meshes in the `ModeledShape`, across all render
  // groups, listed in z-order (the first mesh in the span should be rendered on
  // bottom; the last mesh should be rendered on top).
  absl::Span<const Mesh> Meshes() const;

  // Returns the number of outlines (which may be zero) in render group
  // `group_index`.
  //
  // This method CHECK-fails if `group_index` >= `RenderGroupCount()`.
  uint32_t OutlineCount(uint32_t group_index) const;

  // Returns a span over the `VertexIndexPair`s specifying the outline at
  // `outline_index` within render group `group_index`. The `mesh_index` of each
  // `VertexIndexPair` in the returned outline is an index into the span
  // returned by `RenderGroupMeshes(group_index)`.
  //
  // This method CHECK-fails if `group_index` >= `RenderGroupCount()` or if
  // `outline_index` >= `OutlineCount(group_index)`. The returned span is
  // guaranteed to be non-empty.
  absl::Span<const VertexIndexPair> Outline(uint32_t group_index,
                                            uint32_t outline_index) const;

  // Returns the position of the vertex at `vertex_index` in the outline at
  // `outline_index` within render group `group_index`. This is equivalent to:
  //
  //   VertexIndexPair idx =
  //       shape.Outline(group_index, outline_index)[vertex_index];
  //   shape.RenderGroupMeshes(group_index)[idx.mesh_index]
  //       .VertexPosition(idx.vertex_index);
  //
  // This method CHECK-fails if `group_index` >= `RenderGroupCount()` or if
  // `outline_index` >= `OutlineCount(group_index)` or if `vertex_index` >=
  // `Outline(group_index, outline_index).size()`.
  Point OutlinePosition(uint32_t group_index, uint32_t outline_index,
                        uint32_t vertex_index) const;

  // Fetches the bounds of the `ModeledShape`, i.e. the bounds of its `Mesh`es.
  // The bounds will be empty if the meshes are empty.
  Envelope Bounds() const;

  // Forces initialization of the spatial index. This is a no-op if the spatial
  // index has already been initialized, or if the `ModeledShape` contains no
  // meshes.
  void InitializeSpatialIndex();

  // Returns true if the spatial index has already been initialized.
  bool IsSpatialIndexInitialized() const;

  // This enumerator is returned by visitor functions, indicating
  // whether the search should continue to the next element, or stop.
  enum class FlowControl : uint8_t { kBreak, kContinue };

  // Visits all triangles in the `ModeledShape`'s meshes that intersect `query`,
  // as per the `Intersects` family of functions. `visitor`'s return value
  // indicates whether the visit should continue or stop early. The visitation
  // order is dependent on the internal structure of the `ModeledShape`'s index,
  // which should be assumed to be arbitrary, and may be non-deterministic.
  //
  // Optional argument `query_to_this` contains the transform that maps from
  // `query`'s coordinate space to this `ModeledShape`'s coordinate space, which
  // defaults to the identity transform. This will initialize the index if it
  // has not already been done.
  void VisitIntersectedTriangles(
      Point query, absl::FunctionRef<FlowControl(TriangleIndexPair)> visitor,
      const AffineTransform& query_to_this = {}) const;
  void VisitIntersectedTriangles(
      const Segment& query,
      absl::FunctionRef<FlowControl(TriangleIndexPair)> visitor,
      const AffineTransform& query_to_this = {}) const;
  void VisitIntersectedTriangles(
      const Triangle& query,
      absl::FunctionRef<FlowControl(TriangleIndexPair)> visitor,
      const AffineTransform& query_to_this = {}) const;
  void VisitIntersectedTriangles(
      const Rect& query,
      absl::FunctionRef<FlowControl(TriangleIndexPair)> visitor,
      const AffineTransform& query_to_this = {}) const;
  void VisitIntersectedTriangles(
      const Quad& query,
      absl::FunctionRef<FlowControl(TriangleIndexPair)> visitor,
      const AffineTransform& query_to_this = {}) const;
  void VisitIntersectedTriangles(
      const ModeledShape& query,
      absl::FunctionRef<FlowControl(TriangleIndexPair)> visitor,
      const AffineTransform& query_to_this = {}) const;

  // Computes an approximate measure of what portion of the `ModeledShape` is
  // covered by or overlaps with `query`. This is calculated by finding the sum
  // of areas of the triangles that intersect the given object, and dividing
  // that by the sum of the areas of all triangles in the `ModeledShape`, all in
  // the `ModeledShape`'s coordinate space. Triangles in the `ModeledShape` that
  // overlap each other (e.g. in the case of a stroke that loops back over
  // itself) are counted individually. Note that, if any triangles have negative
  // area (due to winding, see `Triangle::SignedArea`), the absolute value of
  // their area will be used instead.
  //
  // On an empty `ModeledShape`, this will always return 0.
  //
  // Optional argument `query_to_this` contains the transform that maps from
  // `query`'s coordinate space to this `ModeledShape`'s coordinate space, which
  // defaults to the identity transform.
  //
  // This will initialize the index if it has not already been done.
  //
  // There are no overloads for Point or Segment because they don't have an
  // area.
  float Coverage(const Triangle& query,
                 const AffineTransform& query_to_this = {}) const;
  float Coverage(const Rect& query,
                 const AffineTransform& query_to_this = {}) const;
  float Coverage(const Quad& query,
                 const AffineTransform& query_to_this = {}) const;
  float Coverage(const ModeledShape& query,
                 const AffineTransform& query_to_this = {}) const;

  // Returns true if the approximate portion of the `ModeledShape` covered by
  // `query` is greater than `coverage_threshold`. This is equivalient to
  // `modeled_shape.Coverage(query, query_to_this) > coverage_threshold`
  // but may be faster.
  //
  // On an empty `ModeledShape`, this will always return false.
  //
  // Optional argument `query_to_this` contains the transform that maps from
  // `query`'s coordinate space to this `ModeledShape`'s coordinate space, which
  // defaults to the identity transform.
  //
  // This will initialize the index if it has not already been done.
  //
  // There are no overloads for Point or Segment because they don't have an
  // area.
  bool CoverageIsGreaterThan(const Triangle& query, float coverage_threshold,
                             const AffineTransform& query_to_this = {}) const;
  bool CoverageIsGreaterThan(const Rect& query, float coverage_threshold,
                             const AffineTransform& query_to_this = {}) const;
  bool CoverageIsGreaterThan(const Quad& query, float coverage_threshold,
                             const AffineTransform& query_to_this = {}) const;
  bool CoverageIsGreaterThan(const ModeledShape& query,
                             float coverage_threshold,
                             const AffineTransform& query_to_this = {}) const;

 private:
  // Convenience alias for the R-Tree.
  using RTree = geometry_internal::StaticRTree<TriangleIndexPair>;

  // Contains the data that makes up the `ModeledShape`, which is shared between
  // instances in order to enable fast copying.
  //
  // Note that we use a shared pointer to a data class, instead of separate
  // shared pointers to each object, because `rtree` maintains a reference to
  // `meshes` (via the bounds function for the leaf nodes), and so `meshes`
  // needs to outlive `rtree`, even if the `ModeledShape` is copied. Doing this
  // also means that copies that refer to the same meshes can share the cached
  // R-Tree, even if it was initialized after the copy.
  class Data {
   public:
    static absl::StatusOr<absl::Nonnull<std::unique_ptr<Data>>> FromMeshGroups(
        absl::Span<const MeshGroup> groups);

    uint32_t RenderGroupCount() const;
    const MeshFormat& RenderGroupFormat(uint32_t group_index) const;
    absl::Span<const Mesh> RenderGroupMeshes(uint32_t group_index) const;
    absl::Span<const Mesh> Meshes() const;
    absl::Span<const std::vector<VertexIndexPair>> Outlines(
        uint32_t group_index) const;

    // Fetches the spatial index, initializing it if needed. This CHECK-fails
    // if `Meshes()` is empty; this is expected to be guaranteed by the caller.
    //
    // The spatial index's structure only depends on the `Mesh`es, which are
    // immutable, so the it never needs to be invalidated.
    const RTree& SpatialIndex() const;

    // Returns true if the spatial index has already been initialized.
    bool IsSpatialIndexInitialized() const;

    // Fetchs the total absolute area of the `ModeledShape` (i.e. the sum of the
    // absolute values of the areas of every triangle), for use with `Coverage`
    // and `CoverageIsGreaterThan`.
    //
    // This will cache the value to avoid recomputing it one subsequent calls;
    // because the `ModeledShape`'s meshes and triangles cannot be changed, it
    // never needs to be invalidated.
    float TotalAbsoluteArea() const;

   private:
    absl::InlinedVector<Mesh, 1> meshes_;
    absl::InlinedVector<std::vector<VertexIndexPair>, 1> outlines_;
    // For each render group, the index into `meshes_` for the first mesh in
    // that group.
    absl::InlinedVector<uint16_t, 1> group_first_mesh_indices_;
    // For each render group, the index into `outlines_` for the first outline
    // in that group.
    absl::InlinedVector<uint32_t, 1> group_first_outline_indices_;
    // For each render group, the `MeshFormat` shared by all meshes in that
    // group.
    absl::InlinedVector<MeshFormat, 1> group_formats_;

    mutable absl::Mutex cache_mutex_;
    // Note that the mutex guards the `std::unique_ptr`, not the pointee, and
    // that the raw pointer is returned by `SpatialIndex()`. This is safe
    // because:
    // - `SpatialIndex()` returns a const pointer
    // - `StaticRTree` is thread-compatible
    // - Once initialized, `rtree_` is not modified until `Data` is destroyed
    // - `SpatialIndex()` is only called from within `ModeledShape` methods;
    //   since the `ModeledShape` keeps `Data` alive, `rtree_` will not be freed
    //   unless the `ModeledShape` were freed, moved/copied into, or moved out
    //   of while the method is executing. This would be a syncronization bug in
    //   the caller and would cause undefined behavior and/or a use-after-free
    //   of `Data` even if we mutex-guarded the pointee
    mutable absl::Nullable<std::unique_ptr<const RTree>> rtree_
        ABSL_GUARDED_BY(cache_mutex_);
    mutable std::optional<float> cached_total_absolute_area_
        ABSL_GUARDED_BY(cache_mutex_);
  };

  // Constructor used by `FromMeshes` to instantiate the `ModeledShape` with
  // `Data`.
  explicit ModeledShape(absl::Nonnull<std::unique_ptr<Data>> data);

  absl::Nullable<std::shared_ptr<const Data>> data_;
};

////////////////////////////////////////////////////////////////////////////////
// Inline function definitions
////////////////////////////////////////////////////////////////////////////////

inline uint32_t ModeledShape::RenderGroupCount() const {
  if (!data_) return 0;
  return data_->RenderGroupCount();
}

inline const MeshFormat& ModeledShape::RenderGroupFormat(
    uint32_t group_index) const {
  // If data_ is null, then there are zero groups, so group_index is necessarily
  // out of bounds.
  ABSL_CHECK(data_);
  return data_->RenderGroupFormat(group_index);
}

inline absl::Span<const Mesh> ModeledShape::RenderGroupMeshes(
    uint32_t group_index) const {
  // If data_ is null, then there are zero groups, so group_index is necessarily
  // out of bounds.
  ABSL_CHECK(data_);
  return data_->RenderGroupMeshes(group_index);
}

inline absl::Span<const Mesh> ModeledShape::Meshes() const {
  if (!data_) return {};
  return data_->Meshes();
}

inline uint32_t ModeledShape::OutlineCount(uint32_t group_index) const {
  // If data_ is null, then there are zero groups, so group_index is necessarily
  // out of bounds.
  ABSL_CHECK(data_);
  return data_->Outlines(group_index).size();
}

inline absl::Span<const ModeledShape::VertexIndexPair> ModeledShape::Outline(
    uint32_t group_index, uint32_t outline_index) const {
  ABSL_CHECK_LT(outline_index, OutlineCount(group_index));
  return data_->Outlines(group_index)[outline_index];
}

inline Point ModeledShape::OutlinePosition(uint32_t group_index,
                                           uint32_t outline_index,
                                           uint32_t vertex_index) const {
  absl::Span<const VertexIndexPair> outline =
      Outline(group_index, outline_index);
  ABSL_CHECK_LT(vertex_index, outline.size());
  VertexIndexPair index = outline[vertex_index];
  return data_->RenderGroupMeshes(group_index)[index.mesh_index].VertexPosition(
      index.vertex_index);
}

inline void ModeledShape::InitializeSpatialIndex() {
  if (!data_) return;

  (void)data_->SpatialIndex();
}

inline bool ModeledShape::IsSpatialIndexInitialized() const {
  return data_ && data_->IsSpatialIndexInitialized();
}

inline ModeledShape::ModeledShape(absl::Nonnull<std::unique_ptr<Data>> data)
    : data_(std::move(data)) {}

inline uint32_t ModeledShape::Data::RenderGroupCount() const {
  return group_first_mesh_indices_.size();
}

inline const MeshFormat& ModeledShape::Data::RenderGroupFormat(
    uint32_t group_index) const {
  ABSL_CHECK_LT(group_index, RenderGroupCount());
  return group_formats_[group_index];
}

inline absl::Span<const Mesh> ModeledShape::Data::RenderGroupMeshes(
    uint32_t group_index) const {
  ABSL_CHECK_LT(group_index, RenderGroupCount());
  size_t start = group_first_mesh_indices_[group_index];
  size_t end = group_index + 1 < group_first_mesh_indices_.size()
                   ? group_first_mesh_indices_[group_index + 1]
                   : meshes_.size();
  return Meshes().subspan(start, end - start);
}

inline absl::Span<const Mesh> ModeledShape::Data::Meshes() const {
  return meshes_;
}

inline absl::Span<const std::vector<ModeledShape::VertexIndexPair>>
ModeledShape::Data::Outlines(uint32_t group_index) const {
  ABSL_CHECK_LT(group_index, RenderGroupCount());
  size_t start = group_first_outline_indices_[group_index];
  size_t end = group_index + 1 < group_first_outline_indices_.size()
                   ? group_first_outline_indices_[group_index + 1]
                   : outlines_.size();
  return absl::MakeConstSpan(outlines_).subspan(start, end - start);
}

inline bool ModeledShape::Data::IsSpatialIndexInitialized() const {
  absl::MutexLock lock(&cache_mutex_);
  return rtree_ != nullptr;
}

}  // namespace ink

#endif  // INK_GEOMETRY_MODELED_SHAPE_H_
