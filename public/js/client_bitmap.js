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
 * @fileoverview Implementation of Sketchology ClientBitmap interface for web.
 * @suppress {const}
 */
goog.provide('ink.ClientBitmap');

goog.require('goog.math.Size');
goog.require('ink.AsmJsWrapper');


/**
 * Creates a JS object that implements the C++ ClientBitmap abstract class.
 *
 * @export
 * @constructor
 * @param {!Uint8ClampedArray} data
 * @param {!goog.math.Size} size
 * @param {!Module.ImageFormat} format
 * @return {?ink.ClientBitmap}
 */
ink.ClientBitmap = function(data, size, format) {

  /** @private {!Uint8ClampedArray} */
  this.data_;

  /** @private {!goog.math.Size} */
  this.size_;

  /** @private {!Module.ImageFormat} */
  this.format_;

  /** @private {?number} */
  this.imgPtr_;

  // The actual ClientBitmap constructor depends on first initializing the
  // emscripten code with the C++ bindings.  So we delay assigning the
  // constructor until first use of ClientBitmap.
  var constructor = ink.AsmJsWrapper.getInstance().extendClass(
      'ClientBitmap', ink.ClientBitmap.prototype);
  if (constructor) {
    ink.ClientBitmap = constructor;
    return /** @type {!ink.ClientBitmap} */ (
        new ink.ClientBitmap(data, size, format));
  } else {
    // Emscripten bindings not initialized.
    return null;
  }
};


/**
 * Override of C++ constructor for ClientBitmap sub-class.
 *
 * @export
 * @param {!Uint8ClampedArray} data
 * @param {!goog.math.Size} size
 * @param {!Module.ImageFormat} format
 */
ink.ClientBitmap.prototype.__construct =
    function(data, size, format) {
  var self = /** @type {!ink.ClientBitmap} */ (this);
  var ivec2_size = [size.width, size.height];
  self['__parent']['__construct'].call(self, ivec2_size, format);
  self.data_ = data;
  self.size_ = size;
  self.format_ = format;
  self.imgPtr_ = null;
};


/**
 * @export
 * Override of C++ destructor. Ensure we free any allocated memory.
 */
ink.ClientBitmap.prototype.__destruct = function() {
  var self = /** @type {!ink.ClientBitmap} */ (this);
  if (self.imgPtr_) {
    ink.AsmJsWrapper.getInstance().free(self.imgPtr_);
  }
  self['__parent']['__destruct'].call(self);
};


/**
 * Get pointer to image data on the Emscripten heap.
 *
 * @export
 * @return {number} Pointer to imageByteData on heap.
 */
ink.ClientBitmap.prototype.imageByteData = function() {
  var self = /** @type {!ink.ClientBitmap} */ (this);
  if (!self.imgPtr_) {
    var asm = ink.AsmJsWrapper.getInstance();

    var len = self.data_.length * self.data_.BYTES_PER_ELEMENT;

    self.imgPtr_ = asm.malloc(len);

    // Sadly we end up with two copies of the image in JS memory.
    asm.heapU8.set(self.data_, self.imgPtr_);
  }
  return self.imgPtr_;
};


goog.exportSymbol('ink.ClientBitmap', ink.ClientBitmap);
goog.exportProperty(ink.ClientBitmap.prototype, '__construct',
                    ink.ClientBitmap.prototype.__construct);
goog.exportProperty(ink.ClientBitmap.prototype, '__destruct',
                    ink.ClientBitmap.prototype.__destruct);
goog.exportProperty(ink.ClientBitmap.prototype, 'imageByteData',
                    ink.ClientBitmap.prototype.imageByteData);
