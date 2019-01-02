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
 * @fileoverview Wrapper for lower level asmjs functions. None of this should
 *   be ink specific.
 */

goog.provide('ink.AsmJsWrapper');

goog.require('goog.asserts');
goog.require('net.proto2.contrib.WireSerializer');



/**
 * @struct
 * @constructor
 */
ink.AsmJsWrapper = function() {
  /**
   * @type {!Object}
   * @private
   */
  this.module_ = goog.global['Module'];
  goog.asserts.assert(this.module_);

  /**
   * Protocol Buffer wire format serializer.
   * @type {!net.proto2.contrib.WireSerializer}
   * @private
   */
  this.wireSerializer_ = new net.proto2.contrib.WireSerializer();

  /** @type {!Uint8Array} */
  this.heapU8 = this.module_['HEAPU8'];

  /**
   * @param {number} bytes The number of bytes to allocate.
   * @return {number} Pointer to the allocated memory.
   */
  this.malloc = goog.bind(this.module_['_malloc'], this.module_);

  /**
   * @param {number} ptr Pointer to the memory that needs to be freed.
   */
  this.free = goog.bind(this.module_['_free'], this.module_);
};
goog.addSingletonGetter(ink.AsmJsWrapper);


/**
 * Get a factory function for new C++ objects.
 * @param {string} ObjectClass C++ class name
 * @return {function(new:Object, ...*)} factory function for the C++ class.
 */
ink.AsmJsWrapper.prototype.cFactory = function(ObjectClass) {
  var constructor =
      /** @type {function(new:Object, ...*)} */ (this.module_[ObjectClass]);
  return constructor;
};


/**
 * Create an implementing class with the given class name and object.
 * Returns null if the class can not be found or is not loaded yet.
 *
 * @param {string} className name of C++ class to extend
 * @param {!Object} obj object with attached method implementations
 * @return {?function(new:Object, ...*)} JS constructor for extended class
 */
ink.AsmJsWrapper.prototype.extendClass = function(className, obj) {
  if (!this.module_[className] || !this.module_[className]['extend']) {
    return null;
  }
  return this.module_[className]['extend'](className, obj);
};


/**
 * Creates an enum-like dictionary object for the given C++ enum.
 *
 * @param {string} name name of C++ enum to convert
 * @return {!Object.<string, number>} dictionary of enum values to ordinals
 */
ink.AsmJsWrapper.prototype.convertEnumToMap = function(name) {
  var en = this.module_[name];
  var values = /** @type {!Object.<string, number>} */ ({});
  for (var val in en) {
    var ord = en[val]['value'];
    if (en.hasOwnProperty(val) && goog.isNumber(ord)) {
      values[val] = ord;
    }
  }
  return values;
};


/**
 * Get the current WebGL context object.
 * @return {!WebGLRenderingContext}
 */
ink.AsmJsWrapper.prototype.getGLContext = function() {
  return this.module_['ctx'];
};
