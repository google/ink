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

#ifndef INK_PUBLIC_CONTRIB_EXTENSIONS_EXTENSION_POINTS_H_
#define INK_PUBLIC_CONTRIB_EXTENSIONS_EXTENSION_POINTS_H_

#include "ink/engine/public/sengine.h"
#include "ink/engine/service/definition_list.h"
#include "ink/engine/util/unique_void_ptr.h"

namespace ink {
namespace extensions {

// Parameters are platform specific.
// For NaCl: argp[0] = InkInstance*
// Does nothing for all other platforms.
void PreConstruct(int argc, void** argp);

// Returns the service definitions to be used to create the
// engine.
std::unique_ptr<service::DefinitionList> GetServiceDefinitions();

// Run after the engine has been created.
void PostConstruct(SEngine* sengine);

// For platforms that have a messaging API, run when a "runExtension" message is
// seen. arg will contain platform-specific message data.
//
// For NaCl:
//   arg will be a pp::VarDictionary of the following format:
//   {
//      "extension_cmd": VarString,
//      "token": int
//      // Other fields based on the extension_cmd...
//   }
//   The return value of Run(...) will be a unique_ptr<pp::VarDictionary>.
//
//   The extension provider will post a message back to the window with the
//   result if message['token'] is defined. The message will have the format:
//   {
//     "event_type": "extension_result",
//     "token": <value of arg["token"]>
//     "extension_result": <the result of run (a dictionary)>
//   }
//
// Does nothing for all other platforms.
util::unique_void_ptr Run(SEngine* sengine, void* arg);

}  // namespace extensions
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_EXTENSIONS_EXTENSION_POINTS_H_
