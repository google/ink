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

#ifndef INK_ENGINE_SERVICE_DEPENDENCIES_H_
#define INK_ENGINE_SERVICE_DEPENDENCIES_H_

#include <tuple>

namespace ink {
namespace service {

// This typedef is used to declare a classes dependencies, e.g.:
//
// class A {
//  public:
//   using SharedDeps = service::Dependencies<B, C>;
// };
//
// Note: The tuple arguments must be pointers because the dependencies will
// often be abstract classes, and opt builds require that tuple typedefs be
// constructible even if they never are constructed.
template <typename... T>
using Dependencies = std::tuple<T*...>;

}  // namespace service
}  // namespace ink

#endif  // INK_ENGINE_SERVICE_DEPENDENCIES_H_
