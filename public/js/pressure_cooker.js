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

goog.provide('ink.PressureCooker');

goog.require('goog.math');



/**
 * Force values provided by touch screen devices are generally not reliably in
 * the correct [0, 1] range. Normalize the raw pressure readings into a
 * reasonable range of values.
 *
 * This is largely ported logic from PressureCooker.java in Markers (though we
 * don't have any sort of notion of persisted ranges with this).
 *
 * @constructor
 * @struct
 */
ink.PressureCooker = function() {
  /**
   * An approximation of the lower bound of pressure readings that this device
   * will give us.
   * @private {number}
   */
  this.pressureMin_ = 0;

  /**
   * An approximation of the upper bound of pressure readings that this device
   * will give us.
   * @private {number}
   */
  this.pressureMax_ = 1;

  // The initial values of recentMin and recentMax are purposefully set to
  // unlikely values so they will be quickly replaced with actual data.
  /** @private {number} */
  this.pressureRecentMin_ = 1;

  /** @private {number} */
  this.pressureRecentMax_ = 0;

  /**
   * The value that the current pressure countdown started at.
   * @private {number}
   */
  this.countdownStart_ = ink.PressureCooker.UPDATE_STEPS_INITIAL;

  /** @private {number} */
  this.updateCountdown_ = this.countdownStart_;
};


/** @const {number} */
ink.PressureCooker.UPDATE_STEPS_INITIAL = 10;


/** @const {number} */
ink.PressureCooker.UPDATE_STEPS_MAX = 100;


/** @const {number} */
ink.PressureCooker.PRESSURE_UPDATE_DECAY = 0.3;


/**
 * @param {number} pressure The raw pressure reading.
 * @return {number} The adjusted pressure value.
 */
ink.PressureCooker.prototype.getAdjustedPressure = function(pressure) {
  if (pressure < this.pressureRecentMin_) {
    this.pressureRecentMin_ = pressure;
  }
  if (pressure > this.pressureRecentMax_) {
    this.pressureRecentMax_ = pressure;
  }

  this.maybeUpdatePressureBounds_();

  var pmax = this.pressureMax_;
  var pmin = this.pressureMin_;
  var pressureNorm = (pressure - pmin) / (pmax - pmin);

  // If max == min then this will be Infinity or NaN. In particular if a touch
  // device reports a constant pressure reading then this will eventually
  // happen, just report medium pressure in this case.
  if (isNaN(pressureNorm) || pressureNorm == Infinity) {
    pressureNorm = 0.5;
  }

  // The Android client's PressureCooker doesn't clamp here but it does clamp
  // the size of the radius later in the pipeline. The web client is using the
  // pressure values more naively so we clamp the values here.
  // There are pressure values upwards of 1.2 range commonly on both the N7 and
  // Chromebook Pixel, which leads to weird behavior without a clamp here. It is
  // simpler to have this class guarantee a [0,1] range since it exists to
  // smooth out the hardware readings not actually properly being in the [0, 1]
  // range.
  return goog.math.clamp(pressureNorm, 0, 1);
};


/**
 * Updates pressureMin and pressureMax based on the recentMin and recentMax.
 * @private
 */
ink.PressureCooker.prototype.maybeUpdatePressureBounds_ = function() {
  // The update countdown has finished! Update the new bounds based the recent
  // data points.
  if (--this.updateCountdown_ > 0) {
    return;
  }

  var decay = ink.PressureCooker.PRESSURE_UPDATE_DECAY;
  this.pressureMin_ =
      goog.math.lerp(this.pressureMin_, this.pressureRecentMin_, decay);
  this.pressureMax_ =
      goog.math.lerp(this.pressureMax_, this.pressureRecentMax_, decay);

  // Increase the number of steps between each pressure estimate update up to a
  // fixed maximum number of steps.
  this.countdownStart_ = Math.min(
      Math.floor(this.countdownStart_ * 1.5),
      ink.PressureCooker.UPDATE_STEPS_MAX);

  // Upside-down values that will be replaced in the next step.
  this.pressureRecentMin_ = 1;
  this.pressureRecentMax_ = 0;

  this.updateCountdown_ = this.countdownStart_;
};
