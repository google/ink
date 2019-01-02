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

goog.provide('ink.DrawTimer');
goog.provide('ink.DrawTimer.EventType');

goog.require('goog.events');
goog.require('goog.math');



/**
 * Triggers ink.Events.DRAW events based on a throttled framerate.
 * @extends {goog.events.EventTarget}
 * @constructor
 * @struct
 */
ink.DrawTimer = function() {
  ink.DrawTimer.base(this, 'constructor');

  /** @private {?number} */
  this.requestAnimationId_ = null;

  /** @private {function(number)} */
  this.boundHandler_ = goog.bind(this.handleAnimationFrame_, this);

  /**
   * Time that the last time a DRAW event was triggered.
   * @private {number}
   */
  this.lastDrawTime_ = 0;

  /**
   * Time that the last time requestAnimationFrame was called.
   * @private {number}
   */
  this.lastFrameTime_ = 0;

  /**
   * An array of recent time deltas, used for FPS calculations.
   * @private {!Array.<number>}
   */
  this.recentFrameTimeDeltas_ = [];

  /**
   * Actual FPS
   * @private {number}
   */
  this.actualFps_ = 60;

  /**
   * Requested FPS from Sketchology, usually 0, 30, or 60.
   * @private {number}
   */
  this.requestedFps_ = 60;

  /**
   * Frame interval to check in the draw loop.
   * @private {number}
   */
  this.frameInterval_ = 1 / 60;
};
goog.inherits(ink.DrawTimer, goog.events.EventTarget);


/** @override */
ink.DrawTimer.prototype.setParentEventTarget = function(parent) {
  ink.DrawTimer.base(this, 'setParentEventTarget', parent);
};


/** @enum {string} */
ink.DrawTimer.EventType = {
  // To improve input event handling performance, redrawing should be decoupled
  // from any other logic and framerate limited. Any canvas operations should
  // only be done in a event handler for DRAW.
  DRAW: goog.events.getUniqueId('draw')
};


/**
 * Starts the timer.
 */
ink.DrawTimer.prototype.start = function() {
  if (!this.requestAnimationId_) {
    this.requestAnimationId_ = requestAnimationFrame(this.boundHandler_);
  }
};


/**
 * Stops the timer.
 */
ink.DrawTimer.prototype.stop = function() {
  if (this.requestAnimationId_ !== null) {
    cancelAnimationFrame(this.requestAnimationId_);
    this.requestAnimationId_ = null;
  }
};


/**
 * Set the desired frame rate.  e.g. 0 or 60.
 * @param {number} fps
 */
ink.DrawTimer.prototype.setFps = function(fps) {
  var oldFps = this.requestedFps_;
  this.requestedFps_ = fps;
  this.frameInterval_ = 1 / fps;
  if (oldFps == 0 && fps > 0) {
    // Use start since it won't spawn a new rAF if one is running already.
    this.start();
  }
};


/**
 * Get the requested frame rate.
 * @return {number} requested FPS
 */
ink.DrawTimer.prototype.getRequestedFps = function() {
  return this.requestedFps_;
};


/**
 * Poke the renderer.
 */
ink.DrawTimer.prototype.poke = function() {
  if (this.requestedFps_ == 0) {
    this.setFps(60);
  }
};


/**
 * @param {number} timestamp current time when animation callbacks were fired
 * @private
 */
ink.DrawTimer.prototype.handleAnimationFrame_ = function(timestamp) {
  // Stop if requested FPS is zero.
  if (this.requestedFps_ == 0) {
    this.requestAnimationId_ = null;
    this.actualFps_ = 0;
    this.dispatchEvent(ink.embed.events.EventType.FPS_ZERO);
    return;
  }

  // Continue handling animation frames until a stop() is called.
  this.requestAnimationId_ = requestAnimationFrame(this.boundHandler_);

  if (goog.global['SHOW_FPS']) {
    // Do some calculations for logging:
    var frameTimeDelta = timestamp - this.lastFrameTime_;
    this.lastFrameTime_ = timestamp;
    this.recentFrameTimeDeltas_.push(frameTimeDelta);
    if (this.recentFrameTimeDeltas_.length > 5) {
      var avgDelta = goog.math.average.apply(null, this.recentFrameTimeDeltas_);
      this.actualFps_ = Math.round(1000 / avgDelta);
      this.recentFrameTimeDeltas_ = [];
      // Print the rate that animation frames are triggering every 5 frames.
      // Should be zero when idle, 60 when drawing, and 30 when panning.
      // Using console.log will slow down calculation, however.
      window.console.log('FPS:', this.actualFps_);
    }
  }

  // Throttle actual drawing based on requested frame rate.
  var drawTimeDelta = Math.round(timestamp - this.lastDrawTime_);
  if (this.requestedFps_ !== 60 &&
      drawTimeDelta < Math.round(this.frameInterval_ * 1000)) {
    return;
  }
  this.dispatchEvent(ink.DrawTimer.EventType.DRAW);
  this.lastDrawTime_ = timestamp;
};
