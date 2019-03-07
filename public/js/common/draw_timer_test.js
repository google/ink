// Copyright 2019 Google LLC
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
 * @fileoverview Unit tests for ink.DrawTimer.
 */
goog.module('ink.DrawTimerTest');
goog.setTestOnly();

const DrawTimer = goog.require('ink.DrawTimer');
const InkEventType = goog.require('ink.embed.events.EventType');
const MockClock = goog.require('goog.testing.MockClock');

/**
 * Create a canvas and attempt to get a WebGL2RenderingContext.
 *
 * @return {?WebGL2RenderingContext}
 */
const getGLContext = () => {
  const canvas =
      /** @type {!HTMLCanvasElement} */ (document.createElement('canvas'));
  document.body.appendChild(canvas);
  return /** @type {?WebGL2RenderingContext} */ (canvas.getContext('webgl2'));
};

describe('DrawTimer tests', () => {
  /** @type {?MockClock} */
  let clock;

  /**
   * MockClock animation frame timeout is 20 msec.
   * @const {number}
   */
  const FRAME_TIME = 20;

  beforeAll(() => {
    clock = new MockClock();
    clock.install();
  });

  afterAll(() => {
    clock.uninstall();
  });

  it('should draw at 60 FPS until stopped', () => {
    const drawFunc = jasmine.createSpy('draw function');
    const timer = new DrawTimer();
    timer.setDrawFunc(/** @type {!Function} */ (drawFunc));
    timer.start();
    clock.tick(FRAME_TIME * 60);
    timer.stop();
    expect(drawFunc).toHaveBeenCalledTimes(60);
    drawFunc.calls.reset();
    clock.tick(FRAME_TIME);
    expect(drawFunc).toHaveBeenCalledTimes(0);
  });

  it('should not draw with no draw function', () => {
    spyOn(window, 'requestAnimationFrame').and.callThrough();
    const timer = new DrawTimer();
    timer.start();
    clock.tick(FRAME_TIME);
    expect(window.requestAnimationFrame).toHaveBeenCalledTimes(0);
  });

  it('should not draw if the requested FPS is 0', () => {
    const drawFunc = jasmine.createSpy('draw function');
    const timer = new DrawTimer();
    timer.setDrawFunc(/** @type {!Function} */ (drawFunc));
    timer.setFps(0);
    timer.start();
    clock.tick(FRAME_TIME);
    expect(drawFunc).toHaveBeenCalledTimes(0);
  });

  it('should draw at 30 FPS when requested', () => {
    const drawFunc = jasmine.createSpy('draw function');
    const timer = new DrawTimer();
    timer.setDrawFunc(/** @type {!Function} */ (drawFunc));
    timer.setFps(30);
    timer.start();
    clock.tick(FRAME_TIME * 60);
    timer.stop();
    expect(drawFunc).toHaveBeenCalledTimes(30);
  });

  it('should draw at 60 FPS when poked', () => {
    const drawFunc = jasmine.createSpy('draw function');
    const timer = new DrawTimer();
    timer.setDrawFunc(/** @type {!Function} */ (drawFunc));
    timer.setFps(0);
    timer.start();
    timer.poke();
    clock.tick(FRAME_TIME * 60);
    timer.stop();
    expect(drawFunc).toHaveBeenCalledTimes(60);
  });

  it('should fire FPS_ZERO event when FPS is set to zero', () => {
    const timer = new DrawTimer();
    const listener = jasmine.createSpy('zero FPS listener');
    timer.listen(InkEventType.FPS_ZERO, /** @type {!Function} */ (listener));
    timer.setDrawFunc(() => {});
    timer.start();
    clock.tick(FRAME_TIME);
    timer.setFps(0);
    clock.tick(FRAME_TIME);
    expect(listener).toHaveBeenCalled();
  });

  it('should fire FPS_ZERO event when stopped', () => {
    const timer = new DrawTimer();
    const listener = jasmine.createSpy('zero FPS listener');
    timer.listen(InkEventType.FPS_ZERO, /** @type {!Function} */ (listener));
    timer.setDrawFunc(() => {});
    timer.start();
    clock.tick(FRAME_TIME);
    timer.stop();
    clock.tick(FRAME_TIME);
    expect(listener).toHaveBeenCalled();
  });

  it('should check GL fences when requested', () => {
    const drawFunc = jasmine.createSpy('draw function');
    const gl = getGLContext();
    expect(gl).not.toBeNull();
    spyOn(gl, 'fenceSync').and.callThrough();
    const timer = new DrawTimer();
    timer.setDrawFunc(/** @type {!Function} */ (drawFunc));
    if (gl) {
      timer.enableFenceCheckBeforeDraw(gl);
    }
    timer.start();
    clock.tick(FRAME_TIME);
    timer.stop();
    expect(drawFunc).toHaveBeenCalledTimes(1);
    expect(gl.fenceSync).toHaveBeenCalledTimes(1);
  });

  it('should check ignore second calls to enable fence checks', () => {
    const drawFunc = jasmine.createSpy('draw function');
    const gl = getGLContext();
    expect(gl).not.toBeNull();
    spyOn(gl, 'fenceSync').and.callThrough();
    const timer = new DrawTimer();
    timer.setDrawFunc(/** @type {!Function} */ (drawFunc));
    if (gl) {
      timer.enableFenceCheckBeforeDraw(gl);
      timer.enableFenceCheckBeforeDraw(gl);
    }
    timer.start();
    clock.tick(FRAME_TIME);
    timer.stop();
    expect(drawFunc).toHaveBeenCalledTimes(1);
    expect(gl.fenceSync).toHaveBeenCalledTimes(1);
  });

  it('should log when window.SHOW_FPS is true', () => {
    window['SHOW_FPS'] = true;
    try {
      spyOn(console, 'log');
      const timer = new DrawTimer();
      timer.setDrawFunc(() => {});
      timer.start();
      clock.tick(FRAME_TIME);
      timer.setFps(0);
      clock.tick(FRAME_TIME);
      expect(console.log).toHaveBeenCalled();
    } finally {
      window['SHOW_FPS'] = false;
    }
  });
});
