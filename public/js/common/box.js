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
 * @fileoverview An Ink box.
 */

goog.provide('ink.Box');

goog.require('goog.math.Box');


/**
 * Makes a new box.
 * @constructor
 * @extends {goog.math.Box}
 * @param {!Object} obj
 */
ink.Box = function(obj) {
  ink.Box.base(this, 'constructor', obj['top'], obj['right'], obj['bottom'], obj['left']);
};
goog.inherits(ink.Box, goog.math.Box);


/**
 * Makes an ink box from a closure box.
 * @param {!goog.math.Box} box
 * @return {!ink.Box}
 */
ink.Box.fromBox = function(box) {
  return new ink.Box(
      ink.Box.prototype.getSimple.call(/** @type {!ink.Box} */ (box)));
};


/**
 * Returns a simple box for non-closure users.
 * @return {!Object}
 */
ink.Box.prototype.getSimple = function() {
  return {
    'top': this.top,
    'right': this.right,
    'bottom': this.bottom,
    'left': this.left
  };
};
