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

#include "ink/strokes/input/recorded_test_inputs.h"

#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/storage/proto/incremental_stroke_inputs.pb.h"
#include "ink/storage/stroke_input_batch.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace ink {

namespace {

const absl::string_view kRecordedInputsDirectory =
    "_main/ink/strokes/input/testdata/";

// Gets the bounding box for both real and predicted input.
Rect GetBoundingBox(
    const std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>& batches) {
  Envelope envelope;
  for (const auto& [real, predicted] : batches) {
    for (StrokeInput real_input : real) {
      envelope.Add(real_input.position);
    }
    for (StrokeInput predicted_input : predicted) {
      envelope.Add(predicted_input.position);
    }
  }
  ABSL_CHECK(!envelope.IsEmpty());

  return *envelope.AsRect();
}

// Gets the bounding box for a single StrokeInputBatch.
Rect GetBoundingBox(const StrokeInputBatch& batch) {
  Envelope envelope;
  for (StrokeInput input : batch) {
    envelope.Add(input.position);
  }
  ABSL_CHECK(!envelope.IsEmpty());

  return *envelope.AsRect();
}

void ApplyAffineTransform(
    const AffineTransform& transform,
    std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>& batches) {
  for (auto& [real, predicted] : batches) {
    real.Transform(transform);
    predicted.Transform(transform);
  }
}

void BoundTestInputs(
    const Rect& bounds,
    std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>& batches) {
  Rect raw_bounds = GetBoundingBox(batches);
  auto transform = AffineTransform::Find(raw_bounds, bounds);
  ABSL_CHECK(transform.has_value());

  ApplyAffineTransform(*transform, batches);
}

void BoundTestInputs(const Rect& bounds, StrokeInputBatch& batch) {
  Rect raw_bounds = GetBoundingBox(batch);
  auto transform = AffineTransform::Find(raw_bounds, bounds);
  ABSL_CHECK(transform.has_value());

  batch.Transform(*transform);
}

StrokeInputBatch GetRealCombinedInputs(
    const std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>& batches) {
  StrokeInputBatch real_inputs;
  for (const auto& inputs : batches) {
    ABSL_CHECK_OK(real_inputs.Append(inputs.first));
  }
  return real_inputs;
}

absl::StatusOr<std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>>
LoadRawIncrementalStrokeInputs(absl::string_view filename) {
  const std::string& filepath =
      absl::StrCat(testing::SrcDir(), kRecordedInputsDirectory, filename);

  std::ifstream file(std::string(filepath), std::ios::binary);
  if (!file.is_open()) {
    return absl::NotFoundError(absl::StrCat("Failed to open file: ", filepath));
  }
  std::string str((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
  file.close();

  ink::proto::IncrementalStrokeInputs inputs_proto;
  if (!inputs_proto.ParseFromString(str)) {
    return absl::InvalidArgumentError(
        absl::StrCat("Failed to parse file: ", filepath));
  }

  std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>> inputs;

  for (const auto& input : inputs_proto.inputs()) {
    auto real = DecodeStrokeInputBatch(input.real());
    if (!real.ok()) return real.status();
    auto predicted = DecodeStrokeInputBatch(input.predicted());
    if (!predicted.ok()) return predicted.status();
    inputs.push_back({*real, *predicted});
  }

  return inputs;
}
}  // namespace

absl::StatusOr<std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>>
LoadIncrementalStrokeInputs(absl::string_view filename,
                            std::optional<Rect> bounds) {
  auto batches = LoadRawIncrementalStrokeInputs(filename);
  if (!batches.ok()) return batches.status();
  if (bounds.has_value()) BoundTestInputs(*bounds, *batches);
  return batches;
}

absl::StatusOr<StrokeInputBatch> LoadCompleteStrokeInputs(
    absl::string_view filename, std::optional<Rect> bounds) {
  auto batches = LoadRawIncrementalStrokeInputs(filename);
  if (!batches.ok()) return batches.status();
  ink::StrokeInputBatch batch = GetRealCombinedInputs(*batches);
  if (bounds.has_value()) BoundTestInputs(*bounds, batch);
  return batch;
}

}  // namespace ink
