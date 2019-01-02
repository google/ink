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
 * @fileoverview Watches for element size changes.
 *

goog.provide('ink.ElementSizeWatcher');

goog.require('goog.dom');
goog.require('goog.math.Size');
goog.require('goog.style');



/**
 * @struct
 * @constructor
 */
ink.ElementSizeWatcher = function() {
  /** @private {!Array<!Element>} */
  this.elementsList_ = [];

  /** @private {!Array<!Array<function(!Element, !goog.math.Size, number)>>} */
  this.listeners_ = [];

  /** @private {!Array<!goog.math.Size>} */
  this.dimensions_ = [];

  /** @private {boolean} */
  this.polling_ = false;

  /** @private {function()} */
  this.boundPoller_ = goog.bind(ink.ElementSizeWatcher.prototype.poller_, this);

  /** @private {number} */
  this.devicePixelRatio_ = goog.dom.getPixelRatio();
};
goog.addSingletonGetter(ink.ElementSizeWatcher);


/**
 * @param {!Element} elem
 * @param {function(!Element, !goog.math.Size, number)} listener
 */
ink.ElementSizeWatcher.prototype.watchElement = function(elem, listener) {
  var idx = this.elementsList_.indexOf(elem);
  if (idx === -1) {
    this.elementsList_.push(elem);
    this.listeners_.push([listener]);
    this.dimensions_.push(goog.style.getSize(elem));
  } else {
    this.listeners_[idx].push(listener);
  }

  if (!this.polling_) {
    this.polling_ = true;
    window.requestAnimationFrame(this.boundPoller_);
  }
};


/**
 * @param {!Element} elem
 * @param {function(!Element, !goog.math.Size, number)} listener
 * @return {boolean} true if removing the listener succeeded.
 */
ink.ElementSizeWatcher.prototype.unwatchElement = function(elem, listener) {
  var idx = this.elementsList_.indexOf(elem);
  if (idx === -1) {
    return false;
  }
  var listeners = this.listeners_[idx];
  var listenerIdx = listeners.indexOf(listener);
  if (listenerIdx === -1) {
    return false;
  }
  listeners.splice(listenerIdx, 1);
  if (listeners.length === 0) {
    this.elementsList_.splice(idx, 1);
    this.listeners_.splice(idx, 1);
    this.dimensions_.splice(idx, 1);
    if (this.elementsList_.length === 0) {
      // no need to continue polling
      this.polling_ = false;
    }
  }
  return true;
};


/**
 * @private
 */
ink.ElementSizeWatcher.prototype.poller_ = function() {
  const currentPixelRatio = goog.dom.getPixelRatio();
  for (let i = 0; i < this.elementsList_.length; i++) {
    const elem = this.elementsList_[i];
    const currentSize = goog.style.getSize(elem);
    if (!goog.math.Size.equals(this.dimensions_[i], currentSize) ||
        this.devicePixelRatio_ != currentPixelRatio) {
      this.dimensions_[i] = currentSize;
      for (const listener of this.listeners_[i]) {
        listener(elem, currentSize, currentPixelRatio);
      }
    }
  }
  this.devicePixelRatio_ = currentPixelRatio;
  if (this.polling_) {
    window.requestAnimationFrame(this.boundPoller_);
  }
};
