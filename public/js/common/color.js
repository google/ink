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

goog.provide('ink.Color');



/**
 * 32bit color representation. Each channels is a 8 bit uint.
 * @param {number} argb The 32-bit color.
 * @constructor
 * @struct
 */
ink.Color = function(argb) {
  /** @type {number} */
  this.argb = argb;

  /** @type {number} */
  this.a = ink.Color.alphaFromArgb(argb);

  /** @type {number} */
  this.r = ink.Color.redFromArgb(argb);

  /** @type {number} */
  this.g = ink.Color.greenFromArgb(argb);

  /** @type {number} */
  this.b = ink.Color.blueFromArgb(argb);
};


/**
 * Creates a new ink.Color based on an argb string. If no alpha is specified,
 *   it will default to 0xFF.
 * @param {string} colorString A string in the format of 'AARRGGBB' or 'RRGGBB'.
 * @return {!ink.Color}
 */
ink.Color.fromString = function(colorString) {
  if (colorString.startsWith('#')) {
    colorString = colorString.substr(1);
  }
  // Expand three-digit strings.
  if (colorString.length === 3) {
    colorString = colorString[0] + colorString[0] + colorString[1] +
        colorString[1] + colorString[2] + colorString[2];
  }
  let colorNumber = parseInt(colorString, 16);
  if (colorString.length === 6) {
    colorNumber += 0xFF000000;
  }
  return new ink.Color(colorNumber);
};


/**
 * @return {string} The color string (excluding alpha) that can be used as a
 *     fillStyle.
 */
ink.Color.prototype.getRgbString = function() {
  return 'rgb(' + [this.r, this.g, this.b].join(',') + ')';
};


/** @return {string} The color string that can be used as a fillStyle. */
ink.Color.prototype.getRgbaString = function() {
  return 'rgba(' + [this.r, this.g, this.b, this.a / 255].join(',') + ')';
};


/** @return {!Uint32Array} color as rgba 32-bit unsigned integer */
ink.Color.prototype.getRgbaUint32 = function() {
  return new Uint32Array(
      [(this.r << 24) | (this.g << 16) | (this.b << 8) | this.a]);
};


/**
 * @return {number} The alpha in the range 0-1 that can be used as a
 *     globalAlpha.
 */
ink.Color.prototype.getAlphaAsFloat = function() {
  return this.a / 255;
};


/**
 * Helper function that returns a function that right logical shifts by the
 * provided amount and masks off the result.
 * @param {number} shiftAmount The amount that the function should shift by.
 * @return {!Function}
 * @private
 */
ink.Color.shiftAndMask_ = function(shiftAmount) {
  return function(argb) {
    return (argb >>> shiftAmount) & 0xFF;
  };
};


/**
 * @param {number} argb The argb number.
 * @return {number} alpha in the range 0 to 255.
 */
ink.Color.alphaFromArgb = ink.Color.shiftAndMask_(24);


/**
 * @param {number} argb The argb number.
 * @return {number} red in the range 0 to 255.
 */
ink.Color.redFromArgb = ink.Color.shiftAndMask_(16);


/**
 * @param {number} argb The argb number.
 * @return {number} green in the range 0 to 255.
 */
ink.Color.greenFromArgb = ink.Color.shiftAndMask_(8);


/**
 * @param {number} argb The argb number.
 * @return {number} blue in the range 0 to 255.
 */
ink.Color.blueFromArgb = ink.Color.shiftAndMask_(0);


/** @type {!ink.Color} */
ink.Color.BLACK = new ink.Color(0xFF000000);


/** @type {!ink.Color} */
ink.Color.WHITE = new ink.Color(0xFFFFFFFF);


/** @type {!ink.Color} */
ink.Color.DEFAULT_BACKGROUND_COLOR = new ink.Color(0xFFFAFAFA);

/** @type {!ink.Color} */
ink.Color.DEFAULT_OUT_OF_BOUNDS_COLOR = new ink.Color(0xFFE6E6E6);
