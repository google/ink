// Copyright 2024-2025 Google LLC
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

#ifndef INK_STORAGE_BRUSH_PROVIDER_H_
#define INK_STORAGE_BRUSH_PROVIDER_H_

#include "absl/status/statusor.h"
#include "ink/brush/brush_family.h"
#include "ink/types/uri.h"

namespace ink {

// Maps from brush family URIs to BrushFamily objects.  The base class only
// supports Ink's built-in stock brushes, but clients can subclass it to provide
// their own client-specific brushes as well.
class BrushProvider {
 public:
  BrushProvider() = default;
  virtual ~BrushProvider() = default;

  // Returns the brush family that the given URI refers to.  If the URI's
  // registered name is "ink" (or omitted), then the URI is assumed to refer to
  // a stock brush.  Otherwise, this method defers to GetClientBrushFamily().
  //
  // Returns an InvalidArgument error if the URI's asset type isn't
  // "brush-family". Returns a NotFound error if no brush family with that URI
  // exists in this provider.
  absl::StatusOr<BrushFamily> GetBrushFamily(const Uri& uri) const;

 protected:
  // Subclasses should override this to handle any non-stock brushes they wish
  // to provide, deferring to the superclass for any URIs they don't support.
  // Implementations may assume that when this is called, the `uri` argument has
  // already been validated to have asset type "brush-family" and a nonempty,
  // non-"ink" registered name.
  //
  // The default implementation always returns a NotFound error.
  virtual absl::StatusOr<BrushFamily> GetClientBrushFamily(
      const Uri& uri) const;
};

}  // namespace ink

#endif  // INK_STORAGE_BRUSH_PROVIDER_H_
