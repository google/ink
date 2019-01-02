/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_PUBLIC_DOCUMENT_BUNDLE_DATA_ATTACHMENTS_H_
#define INK_PUBLIC_DOCUMENT_BUNDLE_DATA_ATTACHMENTS_H_

#include <cstddef>

#include <vector>

namespace ink {

// A ink::proto::ElementBundle does not necessarily have all fields
// on it. This struct can be used to specify which fields are expected.
struct BundleDataAttachments {
  // corresponds to ElementBundle::transform
  bool attach_transform = false;

  // corresponds to ElementBundle::element
  bool attach_element = false;

  // corresponds to ElementBundle::uncompressed_element.outline
  bool attach_outline = false;

  BundleDataAttachments() {}
  BundleDataAttachments(bool attach_transform, bool attach_element,
                        bool attach_outline)
      : attach_transform(attach_transform),
        attach_element(attach_element),
        attach_outline(attach_outline) {}

  static BundleDataAttachments All() { return {true, true, true}; }
  static BundleDataAttachments None() { return {false, false, false}; }
  static BundleDataAttachments ForDocumentStorage() {
    return {true, true, false};
  }

  static std::vector<BundleDataAttachments> CreateCombinations() {
    return {
        {false, false, false},  // 000
        {false, false, true},   // 001
        {false, true, false},   // 010
        {false, true, true},    // 011
        {true, false, false},   // 100
        {true, false, true},    // 101
        {true, true, false},    // 110
        {true, true, true}      // 111
    };
  }

  // Interpret the bool list as binary
  size_t AsNumber() const {
    return (attach_transform ? 4 : 0) + (attach_element ? 2 : 0) +
           (attach_outline ? 1 : 0);
  }
};

}  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_BUNDLE_DATA_ATTACHMENTS_H_
