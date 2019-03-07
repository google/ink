// Copyright 2018 Google LLC
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

#include "ink/engine/geometry/tess/cdrefinement.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "third_party/GeoPredicates/GeoPredicates.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {
namespace {

const bool kDebugPrint = false;

std::once_flag init_geo_predicates_flag;

void InitGeoPredicates() {
  std::call_once(init_geo_predicates_flag, []() { exactinit(); });
}

}  // namespace

inline float Angle(const MeshTriSegment& s1, const MeshTriSegment& s2,
                   const Mesh* mesh) {
  // assert segments are connected, and in order
  // (assert is too slow) ASSERT(s1.idx[1] == s2.idx[0]);

  auto& v1 = mesh->verts[s1.idx[0]].position;
  auto& v2 = mesh->verts[s1.idx[1]].position;
  auto& v3 = mesh->verts[s2.idx[1]].position;
  auto outerang = TurnAngle(v1, v2, v3);
  return NormalizeAngle(M_PI - outerang);
}

inline float Angle(const MeshTriVert& v1, const MeshTriVert& v2,
                   const MeshTriVert& v3, const Mesh* mesh) {
  auto outerang = TurnAngle(mesh->verts[v1.tri->idx[v1.interior_idx]].position,
                            mesh->verts[v2.tri->idx[v2.interior_idx]].position,
                            mesh->verts[v3.tri->idx[v3.interior_idx]].position);
  return NormalizeAngle(M_PI - outerang);
}

inline bool IsCCW(const MeshTriangle* tri, const Mesh* mesh) {
  auto v1 = mesh->verts[tri->idx[0]].position;
  auto v2 = mesh->verts[tri->idx[1]].position;
  auto v3 = mesh->verts[tri->idx[2]].position;

  //    (xi - xi-1) * (yi+1 - yi) - (yi - yi-1) * (xi+1 - xi)
  auto x = (v2.x - v1.x) * (v3.y - v2.y) - (v2.y - v1.y) * (v3.x - v2.x);
  return x > 0.01f;
}

CDR::CDR(Mesh* mesh) : mesh_(mesh) {
  EXPECT(!mesh_->idx.empty());
  EXPECT(mesh_->idx.size() % 3 == 0);
  ntris_ = mesh_->idx.size() / 3;
  // printf("ntris: %i\n",ntris_);
  tris_ = reinterpret_cast<MeshTriangle*>(&mesh_->idx[0]);
  InitData();
  InitGeoPredicates();
}

uint32_t CDR::GetTrisForSeg(MeshTriSegment seg, MeshTriangle** t1,
                            MeshTriangle** t2) {
  ASSERT(t1 != nullptr && t2 != nullptr);

  auto seg_it = seg_to_tri_.find(seg);
  if (seg_it == seg_to_tri_.end()) {
    return 0;
  }
  const auto& tri_indices = seg_it->second;
  uint32_t n = tri_indices.size();
  EXPECT(n == 1 || n == 2);
  *t1 = &tris_[tri_indices[0]];
  if (n == 2) {
    *t2 = &tris_[tri_indices[1]];
  }

  // check that both tris contain this segment
  if (n == 2) {
#if INK_DEBUG
    bool found1 = false;
    bool found2 = false;
    for (int i = 0; i < 3; i++) {
      if ((*t1)->Segment(i) == seg) found1 = true;
      if ((*t2)->Segment(i) == seg) found2 = true;
    }
    ASSERT(found1 && found2);
#endif  // INK_DEBUG

    if (**t1 == **t2) {
      SLOG(SLOG_ERROR, "found duplicate tris $0, $1", (*t1)->ToString(),
           (*t2)->ToString());
      ASSERT(false);
      return 0;
    }
  }

  return n;
}

bool CDR::ShouldFlip(MeshTetrahedron trh) { return ShouldFlip_Circle(trh); }

bool CDR::ShouldFlip_Circle(MeshTetrahedron trh) {
  auto tri = trh.t1;
  auto p1 = mesh_->verts[tri->idx[0]].position;
  auto p2 = mesh_->verts[tri->idx[1]].position;
  auto p3 = mesh_->verts[tri->idx[2]].position;

  // likely suboptimal way of getting p4...
  glm::vec2 p4{0, 0};
  MeshTriVert tv(trh.t2, 0);
  if (!trh.IsShared(tv)) {
    p4 = mesh_->verts[tv.Idx()].position;
  } else {
    tv = tv.Advance();
    if (!trh.IsShared(tv)) {
      p4 = mesh_->verts[tv.Idx()].position;
    } else {
      tv = tv.Advance();
      if (!trh.IsShared(tv)) {
        p4 = mesh_->verts[tv.Idx()].position;
      } else {
        EXPECT(false);
      }
    }
  }

  float f1[2] = {p1.x, p1.y};
  float f2[2] = {p2.x, p2.y};
  float f3[2] = {p3.x, p3.y};
  float f4[2] = {p4.x, p4.y};

  if (incircle(f1, f2, f3, f4) > 0) return true;
  return false;
}

bool CDR::Flip(MeshTetrahedron trh) {
  // flip the interior line of a tetrahedron
  // assumes the tetrahedron is convex
  // returns segment between the non-shared verts

  // printf("flip\n");
  auto last = MeshTriVert(trh.t1, 0);
  // start on a non-shared vert
  while (trh.IsShared(last)) last = last.Advance();

  auto current = trh.Advance(last);
  auto next = trh.Advance(current);

  MeshTriangle nt1, nt2;
  uint32_t ishared = 0;
  uint32_t inshared = 0;

  // For debugging:
  // Mesh::IndexType nshared[2];
  // Mesh::IndexType shared[2];
  // Mesh::IndexType lookedat[4];

  for (int i = 0; i < 4; i++) {
    // lookedat[i] = current.idx();

    auto other = trh.t1;
    if (current.tri == other) other = trh.t2;

    Mesh::IndexType other_interior_idx;
    if (other->HasIdx(current.Idx(), &other_interior_idx)) {
      // we should not be able to have anything other than 2 shared, 2
      // non-shared verts. The tris may have different winding orders, confusing
      // the advance logic
      if (ishared > 1) {
        return false;
      }
      // printf("looking at %i (shared)\n", current.idx());

      auto nt = &nt1;
      if (ishared == 1) nt = &nt2;

      nt->idx[0] = last.Idx();
      nt->idx[1] = current.Idx();
      nt->idx[2] = next.Idx();

      ishared++;
      // shared[ishared] = current.idx();
    } else {
      // we should not be able to have anything other than 2 shared, 2
      // non-shared verts. The tris may have different winding orders, confusing
      // the advance logic
      if (inshared > 1) {
        return false;
      }

      inshared++;
      // printf("looking at %i (non-shared)\n", current.idx());
      // nshared[inshared] = current.idx();
    }

    last = current;
    current = next;
    next = trh.Advance(next);
  }

  ASSERT(ishared == 2);
  ASSERT(inshared == 2);
  // printf("flipping triangle %s to %s\n", trh.t1->str().c_str(),
  // nt1.str().c_str());
  // printf("flipping triangle %s to %s\n", trh.t2->str().c_str(),
  // nt2.str().c_str());
  for (int i = 0; i < 3; i++) {
    trh.t1->idx[i] = nt1.idx[i];
    trh.t2->idx[i] = nt2.idx[i];
  }
  return true;
}

void CDR::InitData() {
  seg_to_tri_.reserve(mesh_->idx.size());
  for (uint32_t i = 0; i < ntris_; i++) {
    auto& tri = tris_[i];
    if (!tri.Valid()) continue;
    AddTriToMap(&tri, i);
  }
  // Order the segments so that any given set of segments and
  // triangle associations have deterministic behaviour.
  // segs will give us an ordered set of unique segments.
  std::set<MeshTriSegment> segs;
  for (auto ai = seg_to_tri_.begin(); ai != seg_to_tri_.end(); ai++) {
    segs.insert(ai->first);
  }
  // Add segments to the stack, only if they are interior segments. We know the
  // stack is at least as large as the unique segments observed.
  seg_stack_.reserve(segs.size());
  in_seg_stack_.reserve(segs.size());
  for (auto& seg : segs) {
    // Only enqueue segments that have two triangles associated (ie, inner
    // segments).
    const bool is_interior_segment = seg_to_tri_[seg].size() == 2;
    in_seg_stack_[seg] = is_interior_segment;
    if (is_interior_segment) {
      seg_stack_.emplace_back(seg);
    }
  }
}

void CDR::AddTriToMap(const MeshTriangle* t, uint32_t idx) {
  if (kDebugPrint) printf("adding tri %s\n", Str(*t).c_str());

  // check triangle winding order

  // check for duplicate triangles
  bool duplicate = false;
  for (uint32_t j = 0; j < 3; j++) {
    auto seg = t->Segment(j);
    auto seg_it = seg_to_tri_.find(seg);
    if (seg_it == seg_to_tri_.end()) {
      continue;
    }
    const auto& tri_indices = seg_it->second;
    auto count = tri_indices.size();
    duplicate = (count > 1 || (count == 1 && tris_[tri_indices[0]] == *t));
    if (duplicate) {
      if (kDebugPrint) {
        printf("segtotri already contains the segments we're adding!\n");
        printf("duplicate %s - map contains: ", Str(*t).c_str());
        for (auto ai : tri_indices) {
          printf("%s, ", Str(tris_[ai]).c_str());
        }
        printf("\n");
      }
      break;
    }
  }

  // check winding order and colinearity
  bool ccw = false;
  if (IsCCW(t, mesh_)) ccw = true;

  if (ccw && !duplicate) {
    for (uint32_t j = 0; j < 3; j++) {
      auto emplace_result = seg_to_tri_.try_emplace(
          t->Segment(j), absl::InlinedVector<uint32_t, 2>{});
      emplace_result.first->second.emplace_back(idx);
    }
  }
}

uint32_t CDR::RemoveTriFromMap(const MeshTriangle* tri) {
  uint32_t id = -1;

  // find the triangle id
  {
    auto seg_it = seg_to_tri_.find(tri->Segment(0));

#if INK_DEBUG
    bool found = false;
#endif
    const auto& tri_indices = seg_it->second;
    for (auto ai : tri_indices) {
      if (tris_[ai] == *tri) {
        id = ai;
#if INK_DEBUG
        found = true;
#endif
        break;
      }
    }
#if INK_DEBUG
    ASSERT(found);
#endif
  }

  if (kDebugPrint) printf("removing tri %s, id=%i\n", Str(*tri).c_str(), id);

  int nerased = 0;
  for (uint32_t j = 0; j < 3; j++) {
    do {
      auto seg = tri->Segment(j);
      auto seg_it = seg_to_tri_.find(seg);
      if (seg_it == seg_to_tri_.end()) {
        break;
      }
      auto& tri_indices = seg_it->second;
      bool erased = false;
      auto it = std::find(tri_indices.begin(), tri_indices.end(), id);
      if (it != tri_indices.end()) {
        tri_indices.erase(it);
        erased = true;
        nerased++;
      }
      if (tri_indices.empty()) {
        seg_to_tri_.erase(seg_it);
      }
      if (!erased) break;
    } while (true);
  }
  // printf("erased: %i\n", nerased);
  ASSERT(nerased == 3);

  return id;
}

void CDR::RefineMesh() {
  if (kDebugPrint) printf("starting mesh refinement\n");

  uint32_t iterations = 0;
  while (!seg_stack_.empty()) {
    iterations++;
    MeshTriSegment seg = seg_stack_.back();
    seg_stack_.pop_back();
    // Mark the element as not in the stack.
    auto seg_iter = in_seg_stack_.find(seg);
    if (seg_iter == in_seg_stack_.end()) {
      // This should never happen, as we expect that the map entry
      // is set to true if the segment is in the stack. In any case,
      // silently ignore this entry.
      continue;
    }
    seg_iter->second = false;

    MeshTetrahedron trh;
    int n = 0;
    n = GetTrisForSeg(seg, &trh.t1, &trh.t2);
    if (n != 2) continue;

    if (kDebugPrint) {
      printf("examining triangles: %s, %s\n", Str(*trh.t1).c_str(),
             Str(*trh.t2).c_str());
    }

    if (ShouldFlip(trh)) {
      auto t1id = RemoveTriFromMap(trh.t1);
      auto t2id = RemoveTriFromMap(trh.t2);
      bool flipped = Flip(trh);
      AddTriToMap(trh.t1, t1id);
      AddTriToMap(trh.t2, t2id);
      if (!flipped) {
        // The flip failed. Treat this as a nonflippable segment.
        continue;
      }

      // Only add an edge back if it's not the new edge. The new edge
      // will be shared on both tris and should have a count of 2.
      std::map<MeshTriSegment, int> flipped_tri_segs;
      for (int i = 0; i < 3; i++) {
        auto seg1 = trh.t1->Segment(i);
        auto seg2 = trh.t2->Segment(i);
        flipped_tri_segs[seg1]++;
        flipped_tri_segs[seg2]++;
      }
      for (auto& seg_and_count : flipped_tri_segs) {
        auto emplace_result =
            in_seg_stack_.try_emplace(seg_and_count.first, false);
        if (seg_and_count.second == 1 && !emplace_result.first->second) {
          emplace_result.first->second = true;
          seg_stack_.emplace_back(seg_and_count.first);
        }
      }
    }

    // This loop terminating depends on the numerical stability of the
    // shouldFlip code.
    // This is affected by all sorts of things, and should always succeed.
    // However, should we get stuck in this loop the consequences are pretty
    // severe, lost work, freezing,
    // pegging the cpu at 100%, perf degredation, etc
    // Don't get stuck in this loop no matter what!
    if (iterations > 500000) {
      ASSERT(false);
      SLOG(SLOG_ERROR, "unbounded interations in CDR!");
      break;
    }
  }
}
}  // namespace ink
