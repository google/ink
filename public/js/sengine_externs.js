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

/**
 * @fileoverview Externs for Sketchology Javascript API from embind
 * @externs
 *
 * See embind.cc for details.
 */

/**
 * The global Emscripten Module object.
 * @const
 */
var Module;

/**
 * Get a value from the Emscripten heap
 *
 * @param {number} ptr offset into the Emscripten heap
 * @param {string} type LLVM IR type as string
 * @return {number} number type is always returned
 * See
 * https://kripken.github.io/emscripten-site/docs/api_reference/preamble.js.html#getValue
 */
Module.getValue;

// The following are views into Emscripten "memory"
// http://kripken.github.io/emscripten-site/docs/porting/emscripten-runtime-environment.html#emscripten-memory-model

/**
 * View of the heap as Uint32.
 * @type {Uint32Array}
 */
Module.HEAPU32;

/**
 * View of the heap as Float32
 * @type {Float32Array}
 */
Module.HEAPF32;

/**
 * View of the heap as Float64 a.k.a. Double
 * @type {Float64Array}
 */
Module.HEAPF64;

/**
 * Allocate a buffer on the Emscripten stack.
 *
 * @param {number} len Number of bytes to allocate
 * @return {number} offset onto Emscripten stack
 */
Module.stackAlloc;

/**
 * Restore the stack to the given stack pointer, saved from stackSave.
 *
 * @param {number} ptr stack pointer from stackSave
 */
Module.stackRestore;

/**
 * Save the current stack pointer, for use with stackRestore.
 *
 * @return {number} stack pointer
 */
Module.stackSave;

/** @typedef {{value: number}} */
Module.EnumValue;

/**
 * @enum {Module.EnumValue}
 */
Module.ToolType = {};

/**
 * @enum {Module.EnumValue}
 */
Module.BrushSizeType = {};

/**
 * @enum {Module.EnumValue}
 */
Module.InputType = {};

/**
 * @enum {Module.EnumValue}
 */
Module.MouseIds = {};

/**
 * @enum {Module.EnumValue}
 */
Module.InputFlag = {};

/**
 * @enum {Module.EnumValue}
 */
Module.ImageFormat = {};

/**
 * @enum {Module.EnumValue}
 */
Module.Wrap = {};

/**
 * @enum {Module.EnumValue}
 */
Module.Flag = {};

/**
 * @enum {Module.EnumValue}
 */
Module.AssetType = {};

/**
 * @enum {Module.EnumValue}
 */
Module.MouseWheelBehavior = {};

