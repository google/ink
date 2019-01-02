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

// An IHost is everything that the SEngine needs to communicate with at runtime.

#ifndef INK_ENGINE_PUBLIC_HOST_IHOST_H_
#define INK_ENGINE_PUBLIC_HOST_IHOST_H_

#include "ink/engine/public/host/ielement_listener.h"
#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/public/host/imutation_listener.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/public/host/iscene_change_listener.h"

namespace ink {
class IHost : public IElementListener,
              public IEngineListener,
              public IPagePropertiesListener,
              public IPlatform,
              public IMutationListener,
              public ISceneChangeListener {};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_IHOST_H_
