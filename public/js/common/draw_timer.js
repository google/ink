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

goog.module('ink.DrawTimer');
goog.module.declareLegacyNamespace();

const GoogEventTarget = goog.require('goog.events.EventTarget');
const InkEventType = goog.require('ink.embed.events.EventType');


/**
 * Invokes engine.draw in response to a requested framerate.
 *
 * The frame rate is set by the engine, and managed with a requestAnimationFrame
 * callback loop.
 *
 * All times are specified in milliseconds.  Accuracy of the time values varies
 * by browser, from 5 microseconds (Chrome) to 2 milliseconds (Firefox with
 * privacy.reduceTimerPrecision enabled by default).
 */
class DrawTimer extends GoogEventTarget {
  constructor() {
    super();

    /**
     * Draw function to call in the loop.
     * @private {?function():undefined}
     */
    this.draw_ = null;

    /**
     * Callback ID used to stop the draw loop.
     * @private {?number}
     */
    this.requestAnimationId_ = null;

    /**
     * Time that the last time this.draw_ was called.
     * @private {number}
     */
    this.lastDrawTimeMillis_ = 0;

    /**
     * An array of recent time deltas, used for FPS calculations.
     * @private {!Array<number>}
     */
    this.recentDrawTimeDeltasMillis_ = [];

    /**
     * Time that the last time requestAnimationFrame was called.
     * @private {number}
     */
    this.lastFrameTimeMillis_ = 0;

    /**
     * An array of recent time deltas, used for FPS calculations.
     * @private {!Array<number>}
     */
    this.recentFrameTimeDeltasMillis_ = [];

    /**
     * Count of frames dropped due to WebGLSync check.
     * @private {number}
     */
    this.droppedFrames_ = 0;

    /**
     * Requested FPS from engine, usually 0, 30, or 60.
     * @private {number}
     */
    this.requestedFps_ = 60;

    /**
     * If non-null, use the context to check a WebGLSync fence before draw.
     *
     * @private {?WebGL2RenderingContext}
     */
    this.glContext_ = null;

    /**
     * Two WebGL fences for querying command queue status.
     *
     * We use two fences to avoid allowing the GPU command queue to fully drain,
     * which on slow devices can cause us to drop frames we should otherwise be
     * able to draw.
     * @private {!Array<?WebGLSync>}
     */
    this.fences_ = [null, null];

    /**
     * Counter for the current fence to check.
     * @private {number}
     */
    this.currentFence_ = 0;
  }

  /**
   * Set the draw function to use in the loop.
   *
   * If start is called before the draw function is set, loop will not start.
   *
   * @param {function():undefined} drawFunc
   */
  setDrawFunc(drawFunc) {
    this.draw_ = drawFunc;
    if (this.glContext_) {
      this.enableFenceCheckBeforeDraw(this.glContext_);
    }
  }

  /**
   * Use a WebGLSync fence to check the state of the GPU GL command queue before
   * drawing.
   *
   * This is useful in single-buffered rendering when the compositor is not
   * adding backpressure.  If we overrun the GPU queue, then the display will
   * fall further and further behind the state of the engine.
   *
   * @param {!WebGL2RenderingContext} glContext GL context needed for fence
   */
  enableFenceCheckBeforeDraw(glContext) {
    if (this.glContext_) return;
    this.glContext_ = glContext;

    if (!this.draw_) return;

    const drawFunc = this.draw_;
    this.draw_ = () => this.checkFenceAndDraw_(drawFunc);
  }

  /**
   * Starts the draw loop.
   */
  start() {
    if (!this.draw_) return;
    if (!this.requestAnimationId_) {
      this.requestAnimationId_ = requestAnimationFrame((millis) => {
        this.handleAnimationFrame_(millis);
      });
    }
  }

  /**
   * Stops the draw loop.
   */
  stop() {
    if (this.requestAnimationId_ !== null) {
      cancelAnimationFrame(this.requestAnimationId_);
      this.requestAnimationId_ = null;
      this.dispatchEvent(InkEventType.FPS_ZERO);
      if (goog.global['SHOW_FPS']) {
        this.logAndResetFrameCounts_();
      }
    }
  }


  /**
   * Set the desired frame rate.  e.g. 0 or 60.
   * @param {number} fps
   */
  setFps(fps) {
    const oldFps = this.requestedFps_;
    this.requestedFps_ = fps;
    if (oldFps == 0 && fps > 0) {
      // Start a new rAF loop if we were previously stopped.
      this.start();
    }
  }

  /**
   * Get the requested frame rate.
   * @return {number} requested FPS
   */
  getRequestedFps() {
    return this.requestedFps_;
  }


  /**
   * Draw at least one frame, and allow the engine to set the FPS if needed.
   *
   * If not needed, the engine will immediately set the FPS back to zero.
   */
  poke() {
    if (this.requestedFps_ == 0) {
      this.setFps(60);
    }
  }


  /**
   * requestAnimationFrame callback.
   *
   * Returns immediately if requestedFps is 0, otherwise schedules a new frame,
   * calculates frame rate statistics, and calls the draw function.
   *
   * @param {number} millis current time when animation callbacks were fired
   * @private
   */
  handleAnimationFrame_(millis) {
    // Stop if requested FPS is zero.
    if (this.requestedFps_ == 0) {
      this.requestAnimationId_ = null;
      this.dispatchEvent(InkEventType.FPS_ZERO);
      if (goog.global['SHOW_FPS']) {
        this.logAndResetFrameCounts_();
      }
      return;
    }

    // Continue handling animation frames until a stop() is called.
    this.requestAnimationId_ = requestAnimationFrame((millis) => {
      this.handleAnimationFrame_(millis);
    });

    const drawTimeDeltaMillis = millis - this.lastDrawTimeMillis_;

    if (goog.global['SHOW_FPS']) {
      if (this.lastFrameTimeMillis_ != 0) {
        this.recentFrameTimeDeltasMillis_.push(
            millis - this.lastFrameTimeMillis_);
        this.recentDrawTimeDeltasMillis_.push(drawTimeDeltaMillis);
        if (this.recentFrameTimeDeltasMillis_.length == 60) {
          this.logAndResetFrameCounts_();
        }
      }
      this.lastFrameTimeMillis_ = millis;
    }

    // If the requested frame rate is less than 60, throttle actual drawing to
    // that rate, otherwise draw at the speed requested by the browser.
    //
    // We have to throttle the actual drawing, because the browser controls the
    // timing of the requestAnimationFrame callbacks.
    if (this.requestedFps_ < 60) {
      const drawTimeInterval = 1 / (this.requestedFps_ * 1000);
      // Account for floating point precision issues and a small amount of
      // timing jank.
      if (drawTimeDeltaMillis - drawTimeInterval < 1) {
        return;
      }
    }

    this.draw_();

    this.lastDrawTimeMillis_ = millis;
  }

  /**
   * Skip a frame if the WebGLSync fence from the prior frame has not been
   * signaled before drawing.
   *
   * See documentation of enableFenceCheckBeforeDraw for details.
   *
   * @param {function():undefined} drawFunc
   * @private
   */
  checkFenceAndDraw_(drawFunc) {
    const gl = this.glContext_;
    const fence = this.fences_[this.currentFence_];
    if (fence && gl.getSyncParameter(fence, gl.SYNC_STATUS) == gl.UNSIGNALED) {
      // WebGL command queue hasn't caught up with the last draw, skip a frame.
      this.droppedFrames_++;
      return;
    } else {
      this.fences_[this.currentFence_] = null;
    }

    drawFunc();
    this.fences_[this.currentFence_] =
        gl.fenceSync(gl.SYNC_GPU_COMMANDS_COMPLETE, 0);
    this.currentFence_ = (this.currentFence_ + 1) % 2;
  }

  /**
   * Log and reset frame rate info.
   *
   * Only logs once a second or when the draw loop goes idle. This prevents
   * logging from killing our framerate.  Enable with window.SHOW_FPS = true.
   *
   * @private
   */
  logAndResetFrameCounts_() {
    let maxFrameInterval = 0;
    let averageFrameInterval = 0;
    for (const interval of this.recentFrameTimeDeltasMillis_) {
      if (interval > maxFrameInterval) maxFrameInterval = interval;
      averageFrameInterval += interval;
    }
    averageFrameInterval /= this.recentFrameTimeDeltasMillis_.length;
    let maxDrawInterval = 0;
    let averageDrawInterval = 0;
    for (const interval of this.recentDrawTimeDeltasMillis_) {
      if (interval > maxDrawInterval) maxDrawInterval = interval;
      averageDrawInterval += interval;
    }
    averageDrawInterval /= this.recentDrawTimeDeltasMillis_.length;

    console.log(`Frame rate statistics:
                 Average draw time: ${averageDrawInterval}
                 Max draw time: ${maxDrawInterval}
                 Average frame time: ${averageFrameInterval}
                 Max frame time: ${maxFrameInterval}
                 Dropped frames: ${this.droppedFrames_}`);

    this.recentFrameTimeDeltasMillis_.length = 0;
    this.recentDrawTimeDeltasMillis_.length = 0;
    this.droppedFrames_ = 0;
    this.lastFrameTimeMillis_ = 0;
  }
}

exports = DrawTimer;
