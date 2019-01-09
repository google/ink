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
 * @fileoverview Wrapper to call the Sketchology engine.
 */

goog.provide('ink.SketchologyEngineWrapper');

goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.EventType');
goog.require('goog.events.WheelEvent');
goog.require('goog.labs.userAgent.browser');
goog.require('goog.labs.userAgent.platform');
goog.require('goog.math.Coordinate');
goog.require('goog.math.Size');
goog.require('goog.soy');
goog.require('goog.style');
goog.require('goog.ui.Component');
goog.require('ink.AsmJsWrapper');
goog.require('ink.Box');
goog.require('ink.BrushModel');
goog.require('ink.ClientBitmap');
goog.require('ink.Color');
goog.require('ink.DrawTimer');
goog.require('ink.ElementListener');
goog.require('ink.ElementSizeWatcher');
goog.require('ink.HostController');
goog.require('ink.PressureCooker');
goog.require('ink.UndoStateChangeEvent');
goog.require('ink.proto.Border');
goog.require('ink.proto.ElementBundle');
goog.require('ink.proto.Flag');
goog.require('ink.proto.ImageInfo');
goog.require('ink.proto.LayerState');
goog.require('ink.proto.PageProperties');
goog.require('ink.proto.Rect');
goog.require('ink.proto.SetCallbackFlags');
goog.require('ink.proto.Snapshot');
goog.require('ink.proto.ToolParams');
goog.require('ink.soy.emscripten');
goog.require('ink.util');
goog.require('net.proto2.contrib.WireSerializer');

// Preserve the serialization methods since we need to access them in
// the Engine.
goog.exportProperty(
    net.proto2.contrib.WireSerializer.prototype, 'serialize',
    net.proto2.contrib.WireSerializer.prototype.serialize);
goog.exportProperty(
    net.proto2.contrib.WireSerializer.prototype, 'deserializeTo',
    net.proto2.contrib.WireSerializer.prototype.deserializeTo);


// BEGIN-GOOGLE-INTERNAL
// END-GOOGLE-INTERNAL
/**
 * @param {?string} engineUrl Unused in emscripten engine
 * @param {boolean} useAlpha Set alpha attributes on canvas context
 * @param {!ink.ElementListener} elementListener
 * @param {function(number, number, !Uint8ClampedArray)} onImageExportComplete
 * @param {function(string)} requestImage
 * @param {function(!Uint8Array)} onMutation
 * @param {function(!ink.proto.scene_change.SceneChangeEvent)} onSceneChanged
 * @param {(function(!Event):number)=} eventTimeProvider
 * @struct
 * @constructor
 * @extends {goog.ui.Component}
 */
ink.SketchologyEngineWrapper = function(
    engineUrl, useAlpha, elementListener, onImageExportComplete, requestImage,
    onMutation, onSceneChanged, eventTimeProvider) {
  ink.SketchologyEngineWrapper.base(this, 'constructor');

  /** @private {?ink.AsmJsWrapper} */
  this.asmJs_ = null;

  /** @private {boolean} */
  this.useAlpha_ = useAlpha;

  /**
   * Protocol Buffer wire format serializer.
   * @type {!net.proto2.contrib.WireSerializer}
   * @private
   */
  this.wireSerializer_ = new net.proto2.contrib.WireSerializer();

  /** @private {!ink.ElementListener} */
  this.elementListener_ = elementListener;

  /** @private {function(number, number, !Uint8ClampedArray)} */
  this.onImageExportComplete_ = onImageExportComplete;

  /** @private {function(!Uint8Array)} */
  this.onMutation_ = onMutation;

  /** @private {function(!ink.proto.scene_change.SceneChangeEvent)} */
  this.onSceneChanged_ = onSceneChanged;

  /** @private {function(string)} */
  this.requestImage_ = requestImage;

  // default document bounds
  this.pageLeft_ = 0;
  this.pageTop_ = ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_HEIGHT;
  this.pageRight_ = ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_WIDTH;
  this.pageBottom_ = 0;

  /**
   * Background counter
   * @type {number}
   * @private
   */
  this.bgCount_ = 0;

  /**
   * Device pixel ratio, translates CSS pixels to screen pixels, range: [1-3].
   * @private {number}
   */
  this.pixelRatio_ = goog.dom.getPixelRatio();

  /** @private {function(!Event):number}  */
  this.eventTimeProvider_ = eventTimeProvider || ((ev) => ev.timeStamp / 1000);

  /** @private {?Module.MouseIds} */
  this.pressedMouseButton_ = null;

  /** @private {?Object} */
  this.lastMousePosition_ = null;

  /** @private {boolean} */
  this.penDown_ = false;

  /** @private {!ink.PressureCooker} */
  this.pressureCooker_ = new ink.PressureCooker();

  /** @private {?ink.BrushModel} */
  this.brushModel_ = null;

  /** @private {!ink.DrawTimer} */
  this.drawTimer_ = new ink.DrawTimer();
  this.drawTimer_.setParentEventTarget(this);

  /** @private {!Object<number, !Function>} */
  this.sequencePointCallbacks_ = {};

  /** @private {number} */
  this.nextSequencePointId_ = 0;

  /**
   * Image uri counter.
   * @type {number}
   * @private
   */
  this.uriCounter_ = 0;

  /**
   * True if single-buffered rendering is enabled during GL initialization.
   *
   * This determines whether we need to handle rotation in the app or can allow
   * the compositor to handle it for us.
   *
   * @private {boolean}
   * */
  this.isSingleBuffered_ = false;

  /**
   * WebGL fence for querying command queue status.
   * @type {?WebGLSync}
   * @private
   */
  this.fence_ = null;
};
goog.inherits(ink.SketchologyEngineWrapper, goog.ui.Component);
// BEGIN-GOOGLE-INTERNAL
// END-GOOGLE-INTERNAL


/** @const */
ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_WIDTH = 800;
/** @const */
ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_HEIGHT = 600;


/**
 * See https://developer.mozilla.org/en-US/docs/Web/API/Pointer_events
 * @const
 */
ink.SketchologyEngineWrapper.ERASER_BUTTON_FLAG = 32;
/** @const */
ink.SketchologyEngineWrapper.BARREL_BUTTON_FLAG = 2;


/** @override */
ink.SketchologyEngineWrapper.prototype.createDom = function() {
  const elem = goog.soy.renderAsElement(ink.soy.emscripten.canvasHTML);
  this.setElementInternal(elem);
};


/** @override */
ink.SketchologyEngineWrapper.prototype.enterDocument = function() {
  var done = goog.bind(function() {
    setTimeout(goog.bind(this.initGl, this), 0);
  }, this);
  if (goog.global['asmjsLoadingDone']) {
    done();
  } else {
    goog.global['Module'] = goog.global['Module'] || {};
    goog.global['Module']['postRun'] = goog.global['Module']['postRun'] || [];
    goog.global['Module']['postRun'].push(done);
  }

  this.brushModel_ = ink.BrushModel.getInstance(this);

  this.getHandler().listen(
      this.getCanvas_(), 'webglcontextlost', this.handleGLContextLost_);
};


/**
 * @private
 * @return {!HTMLCanvasElement}
 */
ink.SketchologyEngineWrapper.prototype.getCanvas_ = function() {
  return /** @type {!HTMLCanvasElement} */ (
      this.getElementStrict().querySelector('#ink-engine'));
};


/** @enum {string} */
ink.SketchologyEngineWrapper.EventType = {
  CANVAS_INITIALIZED: goog.events.getUniqueId('gl_canvas_initialized'),
  CANVAS_FATAL_ERROR: goog.events.getUniqueId('fatal_error'),
  PEN_MODE_ENABLED: goog.events.getUniqueId('pen_mode_enabled')
};


/**
 * An event fired when pen mode is enabled or disabled.
 *
 * @param {boolean} enabled
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.SketchologyEngineWrapper.PenModeEnabled = function(enabled) {
  ink.SketchologyEngineWrapper.PenModeEnabled.base(
      this, 'constructor',
      ink.SketchologyEngineWrapper.EventType.PEN_MODE_ENABLED);

  /** @type {boolean} */
  this.enabled = enabled;
};
goog.inherits(ink.SketchologyEngineWrapper.PenModeEnabled, goog.events.Event);


/** Poke the engine to wake up and start drawing */
ink.SketchologyEngineWrapper.prototype.poke = function() {
  this.drawTimer_.poke();
};


/**
 * Creates and loads a new, empty document.
 */
ink.SketchologyEngineWrapper.prototype.reset = function() {
  Module['reset'](this.engine_);
  // The default state includes scene bounds.
  this.setPageBounds(
      0, ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_HEIGHT,
      ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_WIDTH, 0);
  // Try to look way outside the scene; let the camera constraints do something
  // reasonable.
  this.setCamera(new ink.Box({
    'left': -100,
    'top': ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_HEIGHT + 100,
    'right': ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_WIDTH + 100,
    'bottom': -100
  }));
};


/**
 * Exits the current instance, by default dispatches fatal error and removes the
 * engine reference and stops the draw loop.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.exit_ = function() {
  this.dispatchEvent(ink.SketchologyEngineWrapper.EventType.CANVAS_FATAL_ERROR);
  this.drawTimer_.stop();
  this.engine_ = null;
};

/** @private */
ink.SketchologyEngineWrapper.prototype.handleGLContextLost_ = function() {
  this.dispatchEvent(ink.SketchologyEngineWrapper.EventType.CANVAS_FATAL_ERROR);
};


/**
 * Initializes WebGL in Emscripten and creates the SEngine object.
 */
ink.SketchologyEngineWrapper.prototype.initGl = function() {
  this.asmJs_ = ink.AsmJsWrapper.getInstance();

  // Catch fatal errors and dispatch an event to embedders.
  goog.global['Module']['onAbort'] = this.exit_.bind(this);

  var canvas = this.getCanvas_();

  this.resizeCanvasToMatchDisplaySize_();

  goog.global['Module']['canvas'] = canvas;
  const gl = /** @type {?WebGLRenderingContext} */ (
      goog.global['Browser']['createContext'](
          canvas, true /* useWebGL */, true /* setInModule */, {
            'alpha': this.useAlpha_,
            'antialias': true,
            // Only enable lowLatency for ChromeOS 71+.  The origin trial
            // activates on Chrome 70 but doesn't work: crbug/908664.  Low
            // latency rendering isn't supported on non-ChromeOS platforms.
            'lowLatency': goog.labs.userAgent.platform.isChromeOS() &&
                goog.labs.userAgent.browser.isVersionOrHigher('71'),
          }));
  if (!gl) {
    this.handleGLContextLost_();
    return;
  }

  // lowLatency: true implies WebGL2 as the lowLatency attribute is only
  // supported in Chrome 70 and newer, which also all support WebGL2.
  this.isSingleBuffered_ = !!gl.getContextAttributes()['lowLatency'];
  this.drawTimer_.listen(
      ink.DrawTimer.EventType.DRAW,
      goog.bind(
          this.isSingleBuffered_ ? this.checkFenceAndDraw_ : this.draw_, this));

  gl.clearColor(1.0, 1.0, 1.0, 1.0);
  gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, true);

  var hostController = new ink.HostController(
      this.elementListener_,  // To delegate Brix methods
      goog.bind(this.drawTimer_.setFps, this.drawTimer_),
      goog.bind(this.drawTimer_.getRequestedFps, this.drawTimer_),
      this.onImageExportComplete_,  // Provided in constructor
      (flag, enabled) => {
        if (flag == ink.proto.Flag.ENABLE_PEN_MODE) {
          this.dispatchEvent(
              new ink.SketchologyEngineWrapper.PenModeEnabled(enabled));
        }
      },  // We only support the pen mode enabled flag change handler.
      goog.bind(this.onUndoRedoStateChanged_, this),
      goog.bind(this.onSequencePointReached_, this),
      this.requestImage_,   // Provided in constructor
      this.onMutation_,     // Provided in constructor
      this.onSceneChanged_  // Provided in constructor
  );

  // 52 bit random number to use as a seed. Note that only ~52 bit integers can
  // be represented exactly in JS so we can't actually generate a random integer
  // for all possible 64 bit numbers here, but 52 bits should be sufficient
  // here.
  var randomSeed = Math.floor(Math.random() * Math.pow(2, 52));

  var viewport = this.makeViewport_();
  try {
    /**
     * @type {?Object}
     * @private
     */
    this.engine_ = Module['makeSEngine'](hostController, viewport, randomSeed);
  } finally {
    viewport['delete']();
  }

  this.setPageBounds(
      this.pageLeft_, this.pageTop_, this.pageRight_, this.pageBottom_);

  ink.ElementSizeWatcher.getInstance().watchElement(
      this.getElementStrict(), goog.bind(this.handleResize_, this));

  if (this.isSingleBuffered_) {
    const rotationHandler = () => {
      const reverse = 360 - screen.orientation.angle;
      canvas.style.transform = `rotateZ(${reverse}deg)`;
      this.handleResize_(
          this.getElementStrict(), goog.style.getSize(this.getElementStrict()),
          goog.dom.getPixelRatio());
    };
    screen.orientation.addEventListener('change', rotationHandler);
    rotationHandler();
  }

  var inputEvents;
  if (goog.global['PointerEvent']) {
    inputEvents = [
      goog.events.EventType.POINTERDOWN,
      goog.events.EventType.POINTERUP,
      goog.events.EventType.POINTERCANCEL,
      goog.events.EventType.POINTERMOVE,
      goog.events.EventType.WHEEL,
    ];
  } else {
    inputEvents = [
      goog.events.EventType.MOUSEDOWN,
      goog.events.EventType.MOUSEUP,
      goog.events.EventType.MOUSEMOVE,
      goog.events.EventType.WHEEL,
      goog.events.EventType.TOUCHSTART,
      goog.events.EventType.TOUCHEND,
      goog.events.EventType.TOUCHMOVE,
      goog.events.EventType.TOUCHCANCEL,
    ];
  }
  var inputHandler = goog.bind(this.dispatch, this);
  for (var i = 0; i < inputEvents.length; i++) {
    canvas['addEventListener'](
        inputEvents[i], inputHandler, {'passive': false});
  }
  // Attach backup event listeners to window for mouse/pointer up in case the
  // mouse is moved off the canvas before releasing.  Otherwise we won't get the
  // mouseup events and have to do something freaky.
  goog.global['window']['addEventListener'](
      goog.events.EventType.MOUSEUP, inputHandler);
  if (goog.global['PointerEvent']) {
    goog.global['window']['addEventListener'](
        goog.events.EventType.POINTERUP, (e) => {
          if (e.pointerType == 'mouse') {
            this.dispatch(e);
          }
        });
  }
  goog.events.listen(canvas, goog.events.EventType.CONTEXTMENU, function(evt) {
    evt.preventDefault();
  });
  this.drawTimer_.start();
  this.dispatchEvent(ink.SketchologyEngineWrapper.EventType.CANVAS_INITIALIZED);
};


/**
 * @return {!Object} C++ viewport proto object.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.makeViewport_ = function() {
  const viewport =
      new (ink.AsmJsWrapper.getInstance().cFactory('ViewportProto'))();
  const canvas = this.getCanvas_();
  let size = {width: canvas.width, height: canvas.height};

  if (this.isSingleBuffered_ && screen.orientation.angle % 180 == 90) {
    // Width and height are swapped when we are handling rotation, in order to
    // make a rotated canvas fit into the DOM element.
    size.width = canvas.height;
    size.height = canvas.width;
  }
  // Never use a 0x0 viewport, always have at least some default size.
  viewport['width'] = size.width ||
      (ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_WIDTH * this.pixelRatio_);
  viewport['height'] = size.height ||
      (ink.SketchologyEngineWrapper.DEFAULT_DOCUMENT_HEIGHT * this.pixelRatio_);
  // CSS pixels are always 96 per inch.
  viewport['ppi'] = 96 * this.pixelRatio_;
  // Reverse the rotation applied in the rotation handler, if the handler is
  // active.
  viewport['screen_rotation'] =
      this.isSingleBuffered_ ? 360 - screen.orientation.angle : 0;
  return /** @type {!Object} */ (viewport);
};


/**
 * @param {!Element} element The element that was changed.
 * @param {!goog.math.Size} size The new size.
 * @param {number} devicePixelRatio The new device pixel ratio
 * @private
 */
ink.SketchologyEngineWrapper.prototype.handleResize_ = function(
    element, size, devicePixelRatio) {
  this.pixelRatio_ = devicePixelRatio;
  const canvas = this.getCanvas_();
  if (this.isSingleBuffered_ && screen.orientation.angle % 180 == 90) {
    // Width and height are swapped when we are handling rotation, in order to
    // make a rotated canvas fit into the DOM element.
    canvas.style.width = size.height + 'px';
    canvas.style.height = size.width + 'px';
    const offset = Math.round((size.height - size.width) / 2);
    canvas.style.left = -offset + 'px';
    canvas.style.top = offset + 'px';
  } else {
    canvas.style.width = size.width + 'px';
    canvas.style.height = size.height + 'px';
    canvas.style.left = '0px';
    canvas.style.top = '0px';
  }
  this.resizeCanvasToMatchDisplaySize_();
  if (size.isEmpty()) {
    return;  // Don't ever set 0x0 viewport on the engine.
  }
  const viewport = this.makeViewport_();
  try {
    this.engine_['setViewport'](viewport);
  } finally {
    viewport['delete']();
  }
};


/**
 * Sets the html width/height of the canvas to match the device pixel size that
 * corresponds to the canvas css size. Otherwise the rendering surface will be
 * scaled to match the css size.
 *
 * @see https://www.khronos.org/webgl/wiki/HandlingHighDPI#Resizing_the_Canvas
 * @param {!goog.math.Size=} opt_size If provided, use this as the current size
 *   instead of measuring the canvas.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.resizeCanvasToMatchDisplaySize_ =
    function(opt_size) {
  const canvas = this.getCanvas_();
  const size = opt_size || goog.style.getSize(canvas);
  canvas.width = size.width * this.pixelRatio_;
  canvas.height = size.height * this.pixelRatio_;
};


/**
 * Adds the given texture image data to the engine.  Once this is called, any
 * textured meshes with that URI in the engine will begin displaying the
 * texture.
 *
 * e.g. background image, border, sticker, grid
 *
 * @param {!Uint8ClampedArray} data
 * @param {!goog.math.Size} size
 * @param {string} uri
 * @param {!ink.proto.ImageInfo.AssetType} assetType
 */
ink.SketchologyEngineWrapper.prototype.addImageData = function(
    data, size, uri, assetType) {
  try {
    var clientBitmap = new ink.ClientBitmap(
        data, size,
        Module.ImageFormat['RGBA_8888']);  // Web is always RGBA 8888.

    var imageInfoCProto = new (this.asmJs_.cFactory('ImageInfoProto'))();
    imageInfoCProto['uri'] = uri;
    imageInfoCProto['asset_type'] = Module.AssetType['values'][assetType];
    this.engine_['addImageData'](imageInfoCProto, clientBitmap);
  } finally {
    if (clientBitmap) {
      clientBitmap['delete']();
    }
    if (imageInfoCProto) {
      imageInfoCProto['delete']();
    }
  }
};


/**
 * Sets the border image.
 * @param {!Uint8ClampedArray} data
 * @param {!goog.math.Size} size
 * @param {string} uri
 * @param {!ink.proto.Border} borderImageProto
 */
ink.SketchologyEngineWrapper.prototype.setBorderImage = function(
    data, size, uri, borderImageProto) {
  var borderImageCProto;
  try {
    this.addImageData(data, size, uri, ink.proto.ImageInfo.AssetType.BORDER);

    borderImageCProto = new (this.asmJs_.cFactory('BorderProto'))();
    borderImageCProto['initFromJs'](borderImageProto, this.wireSerializer_);
    this.engine_['document']()['SetPageBorder'](borderImageCProto);
  } finally {
    if (borderImageCProto) {
      borderImageCProto['delete']();
    }
  }
};


/**
 * Sets the background image from a data URI.
 *
 * @param {!Uint8ClampedArray} data
 * @param {!goog.math.Size} size
 * @param {string} uri
 * @param {!ink.proto.BackgroundImageInfo} bgImageProto
 */
ink.SketchologyEngineWrapper.prototype.setBackgroundImage = function(
    data, size, uri, bgImageProto) {
  var bgImageCProto;
  try {
    this.addImageData(data, size, uri, ink.proto.ImageInfo.AssetType.DEFAULT);

    bgImageCProto = new (this.asmJs_.cFactory('BackgroundImageInfoProto'))();
    bgImageCProto['initFromJs'](bgImageProto, this.wireSerializer_);
    this.engine_['document']()['SetBackgroundImage'](bgImageCProto);
  } finally {
    if (bgImageCProto) {
      bgImageCProto['delete']();
    }
  }
};


/**
 * Set the background color
 * @param {!ink.Color} color
 */
ink.SketchologyEngineWrapper.prototype.setBackgroundColor = function(color) {
  var bgColorCProto;
  try {
    bgColorCProto = new (this.asmJs_.cFactory('BackgroundColorProto'));
    bgColorCProto['rgba'] = color.getRgbaUint32()[0];
    this.engine_['document']()['SetBackgroundColor'](bgColorCProto);
  } finally {
    if (bgColorCProto) {
      bgColorCProto['delete']();
    }
  }
};

/**
 * Sets the out of bounds color.
 * @param {!ink.Color} outOfBoundsColor The out of bounds color.
 */
ink.SketchologyEngineWrapper.prototype.setOutOfBoundsColor = function(
    outOfBoundsColor) {
  let outOfBoundsColorCProto;
  try {
    outOfBoundsColorCProto =
        new (this.asmJs_.cFactory('OutOfBoundsColorProto'))();
    outOfBoundsColorCProto['rgba'] = outOfBoundsColor.getRgbaUint32()[0];
    this.engine_['setOutOfBoundsColor'](outOfBoundsColorCProto);
    this.poke();
  } finally {
    if (outOfBoundsColorCProto) {
      outOfBoundsColorCProto['delete']();
    }
  }
};

/**
 * Set the grid
 *
 * @param {!Uint8ClampedArray} data
 * @param {!goog.math.Size} size
 * @param {string} uri
 * @param {!ink.proto.GridInfo} gridInfoProto
 */
ink.SketchologyEngineWrapper.prototype.setGrid = function(
    data, size, uri, gridInfoProto) {
  var gridInfoCProto;
  try {
    this.addImageData(data, size, uri, ink.proto.ImageInfo.AssetType.GRID);

    gridInfoCProto = new (this.asmJs_.cFactory('GridInfoProto'))();
    gridInfoCProto['initFromJs'](gridInfoProto, this.wireSerializer_);
    this.engine_['setGrid'](gridInfoCProto);
  } finally {
    if (gridInfoCProto) {
      gridInfoCProto['delete']();
    }
  }
};


/**
 * Clear the grid
 */
ink.SketchologyEngineWrapper.prototype.clearGrid = function() {
  this.engine_['clearGrid']();
};


/**
 * Check if the WebGLSync fence has been signaled before drawing.
 *
 * Note this only works with a WebGL2 rendering context.  It's only useful if
 * rendering is single-buffered, and it's use with the draw timer is guarded by
 * a check for the lowLatency: true canvas attribute, see the initGl method.
 * That attribute is only supported in browsers that also support WebGL2.
 *
 * @private
 */
ink.SketchologyEngineWrapper.prototype.checkFenceAndDraw_ = function() {
  const gl = /** @type {!WebGL2RenderingContext} */ (Module['ctx']);
  if (this.fence_ &&
      gl.getSyncParameter(this.fence_, gl.SYNC_STATUS) == gl.UNSIGNALED) {
    // WebGL command queue hasn't caught up with the last draw, skip a frame.
    return;
  } else {
    this.fence_ = null;
  }

  this.draw_();
  this.fence_ = gl.fenceSync(gl.SYNC_GPU_COMMANDS_COMPLETE, 0);
};


/**
 * Triggers a render of the canvas.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.draw_ = function() {
  this.engine_['draw']();
};


/**
 * Rotate inputs to account for screen rotation if single-buffered rendering is
 * in use.
 *
 * See rotationHandler in the constructor.
 *
 * @private
 *
 * @param {number} x offset X of the event
 * @param {number} y offset Y of the event
 * @return {!goog.math.Coordinate} X and Y adjusted for rotation and scale.
 */
ink.SketchologyEngineWrapper.prototype.fixPoint_ = function(x, y) {
  const pos =
      new goog.math.Coordinate(x * this.pixelRatio_, y * this.pixelRatio_);
  if (this.isSingleBuffered_) {
    const angle = screen.orientation.angle;
    const canvas = this.getCanvas_();
    if (angle == 90 || angle == 180) {
      pos.x = canvas.width - pos.x;
    }
    if (angle == 180 || angle == 270) {
      pos.y = canvas.height - pos.y;
    }
    if (angle % 180 == 90) {
      [pos.x, pos.y] = [pos.y, pos.x];
    }
  }
  return pos;
};


/**
 * Dispatches input to the SEngine.
 *
 * @param {!UIEvent} browserEvt
 */
ink.SketchologyEngineWrapper.prototype.dispatch = function(browserEvt) {
  const flags =
      ink.SketchologyEngineWrapper.getFlagForBrowserEvent_(browserEvt.type);

  if (goog.global['PointerEvent'] && browserEvt instanceof PointerEvent) {
    const pointerEvt = /** @type {!PointerEvent} */ (browserEvt);

    if (PointerEvent.prototype.getCoalescedEvents) {
      let didProcessCoalescedEvts = false;
      const coalescedEvts = pointerEvt.getCoalescedEvents();
      if (coalescedEvts.length > 1) {
        for (const evt of coalescedEvts) {
          // Workaround for Firefox bug
          // https://bugzilla.mozilla.org/show_bug.cgi?id=1457859
          if (evt.timeStamp == 0) {
            break;
          }
          didProcessCoalescedEvts = true;
          this.dispatch(evt);
        }
        if (didProcessCoalescedEvts) return;
      }
    }

    if (pointerEvt.pointerType == 'pen') {
      this.performPenDispatch_(flags, pointerEvt);
    } else if (pointerEvt.pointerType == 'touch') {
      this.performTouchDispatch_(
          flags, pointerEvt.pointerId, pointerEvt.offsetX, pointerEvt.offsetY,
          this.eventTimeProvider_(pointerEvt));
    } else if (pointerEvt.pointerType == 'mouse') {
      switch (pointerEvt.type) {
        case goog.events.EventType.POINTERDOWN:
          this.performMouseDownDispatch_(flags, pointerEvt);
          break;
        case goog.events.EventType.POINTERUP:
          this.performMouseUpDispatch_(flags, pointerEvt);
          break;
        case goog.events.EventType.POINTERMOVE:
          this.performMouseMoveDispatch_(flags, pointerEvt);
      }
    }
  } else if (goog.global['TouchEvent'] && browserEvt instanceof TouchEvent) {
    const touchEvt = /** @type {!TouchEvent} */ (browserEvt);
    const timestamp = this.eventTimeProvider_(touchEvt);

    // Touch doesn't have offsetX/Y so calculate them.
    // The coordinate calculations generally don't change, and
    // goog.style.getPageOffset hits the DOM a bunch, so we cache them.
    let previousTarget = null;
    let previousOffset = null;

    for (let idx = 0; idx < touchEvt.changedTouches.length; idx++) {
      const touch = touchEvt.changedTouches[idx];
      const target = /** @type {!Element} */ (touch.target);
      const pageOffset = previousTarget === target ?
          previousOffset :
          goog.style.getPageOffset(target);

      if (previousTarget != target) {
        previousTarget = target;
        previousOffset = pageOffset;
      }

      // Safari reports touch IDs < 0.  Spec says it's a long.
      const touchId = Math.abs(touch.identifier);
      this.performTouchDispatch_(
          flags, touchId, touch.pageX - pageOffset.x,
          touch.pageY - pageOffset.y, timestamp);
    }

    touchEvt.preventDefault();

  } else if (browserEvt instanceof WheelEvent) {
    this.performWheelDispatch_(flags, /** @type {!WheelEvent} */ (browserEvt));
  } else if (browserEvt instanceof MouseEvent) {
    const mouseEvt = /** @type {!MouseEvent} */ (browserEvt);
    switch (mouseEvt.type) {
      case goog.events.EventType.MOUSEDOWN:
        this.performMouseDownDispatch_(flags, mouseEvt);
        break;
      case goog.events.EventType.MOUSEUP:
        this.performMouseUpDispatch_(flags, mouseEvt);
        break;
      case goog.events.EventType.MOUSEMOVE:
        this.performMouseMoveDispatch_(flags, mouseEvt);
    }
  }
};


/**
 * Returns the appropriate engine flags for corresponding browser events.
 * @param {string} evt event type
 * @return {!Array.<!Module.InputFlag|number>}
 * @private
 */
ink.SketchologyEngineWrapper.getFlagForBrowserEvent_ = function(evt) {
  switch (evt) {
    case goog.events.EventType.POINTERDOWN:
    case goog.events.EventType.MOUSEDOWN:
    case goog.events.EventType.TOUCHSTART:
      return [Module.InputFlag['TDOWN'], Module.InputFlag['INCONTACT']];
      break;
    case goog.events.EventType.POINTERUP:
    case goog.events.EventType.MOUSEUP:
    case goog.events.EventType.TOUCHEND:
      return [Module.InputFlag['TUP']];
      break;
    case goog.events.EventType.POINTERMOVE:
    case goog.events.EventType.MOUSEMOVE:
    case goog.events.EventType.TOUCHMOVE:
      return [Module.InputFlag['INCONTACT']];
      break;
    case goog.events.EventType.POINTERCANCEL:
    case goog.events.EventType.TOUCHCANCEL:
      return [Module.InputFlag['TUP'], Module.InputFlag['CANCEL']];
      break;
  }
  return [0];
};


/**
 * @param {!Array.<!Module.InputFlag>} flags The flags to convert to bitfield
 * @return {number} bitfield of flags
 * @private
 */
ink.SketchologyEngineWrapper.getFlagBits_ = function(flags) {
  var flagBits = 0;
  goog.array.forEach(flags, function(f) {
    flagBits |= f.value;
  });
  return flagBits;
};


/**
 * @return {number} Milliseconds since some arbitrary time (not necessarily
 *     unix epoch).
 * @private
 */
ink.SketchologyEngineWrapper.now_ = function() {
  return Math.floor(window.performance.now());
};


/**
 * @param {!Array.<!Module.InputFlag>} flags The flags associated with the
 *     event.
 * @param {number} touchId touch identifier
 * @param {number} offsetX
 * @param {number} offsetY
 * @param {number} timestamp
 * @private
 */
ink.SketchologyEngineWrapper.prototype.performTouchDispatch_ = function(
    flags, touchId, offsetX, offsetY, timestamp) {
  const flagBits = ink.SketchologyEngineWrapper.getFlagBits_(flags);
  const pos = this.fixPoint_(offsetX, offsetY);
  this.engine_['dispatchInput'](
      Module.InputType['TOUCH'], touchId, flagBits, timestamp, pos.x, pos.y);
};


/**
 * Append keyboard modifiers present in the given event to the given flags.
 *
 * @param {!MouseEvent} evt The browser event.
 * @param {!Array.<!Module.InputFlag>} flags The flags associated with the
 *     event.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.extractKeyboardModifiers_ = function(
    evt, flags) {
  if (evt.altKey) {
    flags.push(Module.InputFlag['ALT']);
  }
  if (evt.ctrlKey) {
    flags.push(Module.InputFlag['CONTROL']);
  }
  if (evt.metaKey) {
    flags.push(Module.InputFlag['META']);
  }
  if (evt.shiftKey) {
    flags.push(Module.InputFlag['SHIFT']);
  }
};


/**
 * @param {!Array.<!Module.InputFlag>} flags The flags associated with the
 *     event.
 * @param {!WheelEvent} evt The browser event for the event.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.performWheelDispatch_ = function(
    flags, evt) {
  let deltaX;
  if (evt.wheelDeltaX) {
    deltaX = evt.wheelDeltaX;
  } else if (evt.deltaMode === goog.events.WheelEvent.DeltaMode.LINE) {
    // if the deltaMode is line scrolling, assume a line to be 40 pixels.
    deltaX = 40 * evt.deltaX;
  } else {
    deltaX = evt.deltaX;
  }

  let deltaY;
  if (evt.wheelDeltaY) {
    deltaY = evt.wheelDeltaY;
  } else if (evt.deltaMode === goog.events.WheelEvent.DeltaMode.LINE) {
    // if the deltaMode is line scrolling, assume a line to be 40 pixels.
    deltaY = -40 * evt.deltaY;
  } else {
    deltaY = -evt.deltaY;
  }

  // Discard all wheel events with 0 delta.
  if (deltaX == 0 && deltaY == 0) {
    return;
  }

  const id = Module.MouseIds['MOUSEWHEEL'];
  flags.push(Module.InputFlag['WHEEL']);
  this.extractKeyboardModifiers_(evt, flags);

  const timestamp = this.eventTimeProvider_(evt);
  const flagBits = ink.SketchologyEngineWrapper.getFlagBits_(flags);
  const pos = this.fixPoint_(evt.offsetX, evt.offsetY);
  this.engine_['dispatchInputFull'](
      Module.InputType['MOUSE'], id.value, flagBits, timestamp, pos.x, pos.y,
      deltaX, deltaY, -1 /* no pressure for wheel */, 0 /* no tilt */,
      0 /* no orientation */);

  evt.preventDefault();
};


/**
 * @param {!Array.<!Module.InputFlag>} flags The flags associated with the
 *     event.
 * @param {!MouseEvent} evt The browser event for the event.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.performMouseDownDispatch_ = function(
    flags, evt) {
  // Never send another down when a mouse button is already down.
  if (this.pressedMouseButton_ != null) {
    return;
  }

  let id = Module.MouseIds['MOUSEHOVER'];
  // Use MouseEvent.button for mousedown/up
  if (ink.util.isMouseActionButton(evt)) {
    id = Module.MouseIds['MOUSELEFT'];
    flags.push(Module.InputFlag['LEFT']);
  } else {
    id = Module.MouseIds['MOUSERIGHT'];
    flags.push(Module.InputFlag['RIGHT']);
  }
  this.extractKeyboardModifiers_(evt, flags);

  this.pressedMouseButton_ = id;

  const timestamp = this.eventTimeProvider_(evt);
  const flagBits = ink.SketchologyEngineWrapper.getFlagBits_(flags);
  const pos = this.fixPoint_(evt.offsetX, evt.offsetY);
  this.engine_['dispatchInput'](
      Module.InputType['MOUSE'], id.value, flagBits, timestamp, pos.x, pos.y);

  if (!goog.global['PointerEvent']) {
    evt.preventDefault();
  }
};


/**
 * @param {!Array.<!Module.InputFlag>} flags The flags associated with the
 *     event.
 * @param {!MouseEvent} evt The browser event for the event.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.performMouseUpDispatch_ = function(
    flags, evt) {
  let id = Module.MouseIds['MOUSEHOVER'];
  // Use MouseEvent.button for mousedown/up
  if (ink.util.isMouseActionButton(evt)) {
    id = Module.MouseIds['MOUSELEFT'];
    flags.push(Module.InputFlag['LEFT']);
  } else {
    id = Module.MouseIds['MOUSERIGHT'];
    flags.push(Module.InputFlag['RIGHT']);
  }

  // Don't send an up if it's not for the currently down button or if we don't
  // think a button is down.  This can happen if we get mouseup from window
  // instead of the canvas.
  if (this.pressedMouseButton_ == null || this.pressedMouseButton_ !== id) {
    return;
  }
  this.pressedMouseButton_ = null;

  this.extractKeyboardModifiers_(evt, flags);

  const timestamp = this.eventTimeProvider_(evt);
  const flagBits = ink.SketchologyEngineWrapper.getFlagBits_(flags);
  const pos = this.fixPoint_(evt.offsetX, evt.offsetY);
  this.engine_['dispatchInput'](
      Module.InputType['MOUSE'], id.value, flagBits, timestamp, pos.x, pos.y);

  if (!goog.global['PointerEvent']) {
    evt.preventDefault();
  }
};


/**
 * @param {!Array.<!Module.InputFlag>} flags The flags associated with the
 *     event.
 * @param {!MouseEvent} evt The browser event for the event.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.performMouseMoveDispatch_ = function(
    flags, evt) {
  // If no button is down, ignore mousemove events.
  if (this.pressedMouseButton_ == null) {
    return;
  }

  let id = Module.MouseIds['MOUSEHOVER'];
  if (ink.util.hasMouseActionButton(evt)) {
    id = Module.MouseIds['MOUSELEFT'];
    flags.push(Module.InputFlag['LEFT']);
  } else if (ink.util.hasMouseSecondaryButton(evt)) {
    id = Module.MouseIds['MOUSERIGHT'];
    flags.push(Module.InputFlag['RIGHT']);
  }
  this.extractKeyboardModifiers_(evt, flags);

  // If we think button is down, but the event is for a different button,
  // ignore mousemove events.
  if (this.pressedMouseButton_ !== id) {
    return;
  }

  const timestamp = this.eventTimeProvider_(evt);
  const flagBits = ink.SketchologyEngineWrapper.getFlagBits_(flags);
  const pos = this.fixPoint_(evt.offsetX, evt.offsetY);
  this.engine_['dispatchInput'](
      Module.InputType['MOUSE'], id.value, flagBits, timestamp, pos.x, pos.y);

  if (!goog.global['PointerEvent']) {
    evt.preventDefault();
  }
};


/**
 * @param {!Array.<!Module.InputFlag>} flags The flags associated with the
 *     event.
 * @param {!PointerEvent} evt The browser event for the event.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.performPenDispatch_ = function(
    flags, evt) {
  if (evt.type != goog.events.EventType.POINTERDOWN && !this.penDown_) {
    return;  // No pen in contact, ignore hover events.
  }
  if (evt.buttons & ink.SketchologyEngineWrapper.BARREL_BUTTON_FLAG) {
    return;  // Ignore barrel button
  }
  if (evt.type == goog.events.EventType.POINTERDOWN) {
    this.penDown_ = true;
  }
  if (evt.type == goog.events.EventType.POINTERUP) {
    this.penDown_ = false;
  }
  if (evt.buttons & ink.SketchologyEngineWrapper.ERASER_BUTTON_FLAG) {
    flags.push(Module.InputFlag['ERASER']);
  }
  const timestamp = this.eventTimeProvider_(evt);
  const flagBits = ink.SketchologyEngineWrapper.getFlagBits_(flags);
  const pos = this.fixPoint_(evt.offsetX, evt.offsetY);
  this.engine_['dispatchInputFull'](
      Module.InputType['PEN'], evt.pointerId, flagBits, timestamp, pos.x, pos.y,
      0 /* no wheel_delta_x for pen */, 0 /* no wheel_delta_y for pen */,
      evt.pressure, 0 /* tilt */, 0 /* orientation */);
};


/**
 * Sets the camera position to the given world coordinates box. If the aspect
 * ratio of the box doesn't match that of the viewport, the new camera position
 * will be the smallest box containing the given position.
 *
 * @param {!ink.Box} cameraBox The camera box.
 */
ink.SketchologyEngineWrapper.prototype.setCamera = function(cameraBox) {
  try {
    var rect = new (this.asmJs_.cFactory('SketchologyRect'))(
        cameraBox.left, cameraBox.top, cameraBox.right, cameraBox.bottom);
    this.engine_['setCameraPositionRect'](rect);
  } finally {
    rect['delete']();
  }
  this.engine_['draw']();
};

/**
 * Causes the camera to center on the page at the given page index, if any.
 *
 * @param {number} pageIndex The 0-based index of the page to focus on.
 */
ink.SketchologyEngineWrapper.prototype.focusOnPage = function(pageIndex) {
  this.engine_['focusOnPage'](pageIndex);
};

/**
 * Sets the page layout of the current multi-page document, if any, to a
 * vertical linear layout, with the given inter-page spacing.
 *
 * @param {number} pageSpacingWorld The inter-page spacing, in world units.
 */
ink.SketchologyEngineWrapper.prototype.setVerticalPageLayout = function(
    pageSpacingWorld) {
  try {
    var layoutSpecCProto = new (this.asmJs_.cFactory('LayoutSpecProto'))();
    layoutSpecCProto['strategy'] = Module['LayoutStrategy']['VERTICAL'];
    layoutSpecCProto['spacing_world'] = pageSpacingWorld;
    this.engine_['setPageLayout'](layoutSpecCProto);
  } finally {
    if (layoutSpecCProto) {
      layoutSpecCProto['delete']();
    }
  }
};


/**
 * Returns an Array of ink.Box representing the world-space coordinates of all
 * currently-edited pages. If the engine is not currently editing a multi-page
 * document, the returned array will be empty.
 *
 * @return {!Array<!ink.Box>}
 */
ink.SketchologyEngineWrapper.prototype.getPageLocations = function() {
  let pages;
  const result = [];
  try {
    pages = this.engine_['getPageLocations']();
    for (let i = 0; i < pages.length; i++) {
      const page = pages['rectAt'](i);
      result.push(new ink.Box({
        'top': page['yhigh'],
        'right': page['xhigh'],
        'bottom': page['ylow'],
        'left': page['xlow']
      }));
    }
  } finally {
    if (pages) {
      pages['delete']();
    }
  }
  return result;
};


/**
 * Returns the inter-page spacing, in world coordinates, if the engine is
 * currently editing a multi-page document, and if the current layout is linear
 * (and therefore has a well-definined inter-page spacing). Returns 0 if there
 * is no well-defined inter-page spacing.
 *
 * @return {number}
 */
ink.SketchologyEngineWrapper.prototype.getPageSpacingWorld = function() {
  return this.engine_['getPageSpacingWorld']();
};


/**
 * Sets the brush parameters.
 *
 * @param {!Uint32Array} color rgba 32-bit unsigned color
 * @param {number} strokeWidth The brush's stroke width. [0-1] if
 *   strokeSizeType is PERCENT_WORLD; otherwise some positive number with
 *   different semantics based on strokeSizeType. See the definition of
 *   LineSize.SizeType in sengine.proto for more details.
 * @param {!ink.proto.LineSize.SizeType} strokeSizeType Units used to specify
 *   stroke width.
 * @param {!ink.proto.ToolParams.ToolType} toolType
 * @param {!ink.proto.BrushType} brushType
 */
ink.SketchologyEngineWrapper.prototype.brushUpdate = function(
    color, strokeWidth, strokeSizeType, toolType, brushType) {
  try {
    var toolCProto = new (this.asmJs_.cFactory('ToolParamsProto'))();
    toolCProto['tool'] = Module.ToolType['values'][toolType];
    // LINE tools need special handling
    if (toolType == ink.proto.ToolParams.ToolType.LINE ||
        toolType == ink.proto.ToolParams.ToolType.SMART_HIGHLIGHTER_TOOL) {
      toolCProto['brush_type'] = Module['BrushType']['values'][brushType];
      toolCProto['rgba'] = color[0];
      toolCProto['mutable_line_size']()['stroke_width'] = strokeWidth;
      toolCProto['mutable_line_size']()['use_web_sizes'] = true;
      toolCProto['mutable_line_size']()['units'] =
          Module['BrushSizeType']['values'][strokeSizeType];
    } else if (
        toolType == ink.proto.ToolParams.ToolType.TEXT_HIGHLIGHTER_TOOL) {
      toolCProto['rgba'] = color[0];
    } else if (
        toolType == ink.proto.ToolParams.ToolType.STROKE_EDITING_ERASER) {
      toolCProto['mutable_line_size']()['stroke_width'] = strokeWidth;
      toolCProto['mutable_line_size']()['use_web_sizes'] = true;
      toolCProto['mutable_line_size']()['units'] =
          Module['BrushSizeType']['values'][strokeSizeType];
    }
    this.engine_['setToolParams'](toolCProto);
  } finally {
    if (toolCProto) {
      toolCProto['delete']();
    }
  }
};


/**
 * Clears the canvas.
 * The ONLY legitimate use for this function is receiving a "clear all elements"
 * message from a Brix connection. In other words, there is *no* legitimate use
 * for this function. Avoid it.
 */
ink.SketchologyEngineWrapper.prototype.clear = function() {
  this.engine_['clear']();
};


/** Removes all user-editable elements from the document. */
ink.SketchologyEngineWrapper.prototype.removeAll = function() {
  this.engine_['removeAllElements']();
};


/**
 * Sets or unsets readOnly on the canvas.
 * @param {boolean} readOnly
 */
ink.SketchologyEngineWrapper.prototype.setReadOnly = function(readOnly) {
  this.assignFlag(ink.proto.Flag.READ_ONLY_MODE, !!readOnly);
};


/**
 * Assign a flag on the canvas
 * @param {!ink.proto.Flag} flag
 * @param {boolean} enable
 */
ink.SketchologyEngineWrapper.prototype.assignFlag = function(flag, enable) {
  this.engine_['assignFlag'](Module.Flag['values'][flag], !!enable);
};


/**
 * Set callback flags for what data is attached to element callbacks.
 * @param {!ink.proto.SetCallbackFlags} setCallbackFlags
 */
ink.SketchologyEngineWrapper.prototype.setCallbackFlags = function(
    setCallbackFlags) {
  var setCallbackFlagsCProto;
  try {
    setCallbackFlagsCProto =
        new (this.asmJs_.cFactory('SetCallbackFlagsProto'))();
    setCallbackFlagsCProto['initFromJs'](
        setCallbackFlags, this.wireSerializer_);
    this.engine_['setCallbackFlags'](setCallbackFlagsCProto);
  } finally {
    setCallbackFlagsCProto['delete']();
  }
};


/**
 * Sets the size of the page.
 * @param {number} left
 * @param {number} top
 * @param {number} right
 * @param {number} bottom
 */
ink.SketchologyEngineWrapper.prototype.setPageBounds = function(
    left, top, right, bottom) {
  this.pageLeft_ = left;
  this.pageTop_ = top;
  this.pageRight_ = right;
  this.pageBottom_ = bottom;
  var pageBounds = new (this.asmJs_.cFactory('RectProto'));
  try {
    pageBounds['xlow'] = this.pageLeft_;
    pageBounds['ylow'] = this.pageBottom_;
    pageBounds['xhigh'] = this.pageRight_;
    pageBounds['yhigh'] = this.pageTop_;
    this.engine_['document']()['SetPageBounds'](pageBounds);
  } finally {
    pageBounds['delete']();
  }
};


/**
 * Deselects anything selected with the edit tool.
 */
ink.SketchologyEngineWrapper.prototype.deselectAll = function() {
  this.engine_['deselectAll']();
};


/**
 * Start the PNG export process.
 *
 * @param {!ink.proto.ImageExport} exportProto
 */
ink.SketchologyEngineWrapper.prototype.exportPng = function(exportProto) {
  var imageExportProto;
  try {
    imageExportProto = new (this.asmJs_.cFactory('ImageExportProto'))();
    imageExportProto['initFromJs'](exportProto, this.wireSerializer_);
    this.engine_['startImageExport'](imageExportProto);
  } finally {
    if (imageExportProto) {
      imageExportProto['delete']();
    }
  }
};


/**
 * Simple undo.
 */
ink.SketchologyEngineWrapper.prototype.undo = function() {
  this.engine_['document']()['Undo']();
};


/**
 * Simple redo.
 */
ink.SketchologyEngineWrapper.prototype.redo = function() {
  this.engine_['document']()['Redo']();
};


/**
 * Called when there's a change in the undo / redo state.
 * @param {boolean} canUndo
 * @param {boolean} canRedo
 * @private
 */
ink.SketchologyEngineWrapper.prototype.onUndoRedoStateChanged_ = function(
    canUndo, canRedo) {
  this.dispatchEvent(new ink.UndoStateChangeEvent(canUndo, canRedo));
};


/**
 * Set whether changes to the document are captured in its undo / redo stack.
 * @param {boolean} enabled
 */
ink.SketchologyEngineWrapper.prototype.setUndoEnabled = function(enabled) {
  this.engine_['document']()['SetUndoEnabled'](enabled);
};


/**
 * Returns the current snapshot as a js proto. This current requires two copies
 * of the proto bytes.
 * @param {function(!ink.proto.Snapshot)} callback
 * @param {boolean} includeUndoStack
 */
ink.SketchologyEngineWrapper.prototype.getSnapshot = function(
    callback, includeUndoStack) {
  try {
    var undoStack =
        includeUndoStack ? 'INCLUDE_UNDO_STACK' : 'DO_NOT_INCLUDE_UNDO_STACK';
    var cSnapshotProto = this.engine_['document']()['GetSnapshot'](
        Module['SnapshotQuery'][undoStack]);
    var snapshotProto = new ink.proto.Snapshot();
    cSnapshotProto['copyToJs'](snapshotProto, this.wireSerializer_);
    setTimeout(() => {
      callback(snapshotProto);
    });
  } finally {
    if (cSnapshotProto) {
      cSnapshotProto['delete']();
    }
  }
};


/**
 * Returns the current snapshot as bytes. This requires one copy of the bytes
 * from C++ to JS.
 * @param {function(!ArrayBuffer)} callback
 * @param {boolean} includeUndoStack
 */
ink.SketchologyEngineWrapper.prototype.getSnapshotBytes = function(
    callback, includeUndoStack) {
  try {
    var undoStack =
        includeUndoStack ? 'INCLUDE_UNDO_STACK' : 'DO_NOT_INCLUDE_UNDO_STACK';
    var cSnapshotProto = this.engine_['document']()['GetSnapshot'](
        Module['SnapshotQuery'][undoStack]);
    cSnapshotProto['copyToJs'](undefined, {
      'deserializeTo': (message, buffer) => {
        var ret = buffer.slice(0).buffer;
        setTimeout(() => {
          callback(ret);
        });
      }
    });
  } finally {
    if (cSnapshotProto) {
      cSnapshotProto['delete']();
    }
  }
};


/**
 * Loads a document from a snapshot.
 *
 * @param {!ink.proto.Snapshot} snapshotProto
 */
ink.SketchologyEngineWrapper.prototype.loadFromSnapshot = function(
    snapshotProto) {
  var cSnapshotProto;
  try {
    cSnapshotProto = new (this.asmJs_.cFactory('SnapshotProto'))();
    cSnapshotProto['initFromJs'](snapshotProto, this.wireSerializer_);
    Module['loadFromSnapshot'](this.engine_, cSnapshotProto);
  } finally {
    if (cSnapshotProto) {
      cSnapshotProto['delete']();
    }
  }
};


/**
 * Loads a document from an array of bytes.
 *
 * @param {!Uint8Array} bytes
 */
ink.SketchologyEngineWrapper.prototype.loadFromSnapshotBytes = function(bytes) {
  var cSnapshotProto;
  try {
    cSnapshotProto = new (this.asmJs_.cFactory('SnapshotProto'))();
    cSnapshotProto['initFromJs'](bytes, {
      'serialize': () => {
        return bytes;
      }
    });
    Module['loadFromSnapshot'](this.engine_, cSnapshotProto);
  } finally {
    if (cSnapshotProto) {
      cSnapshotProto['delete']();
    }
  }
};


/**
 * Loads the engine treating the given serialized snapshot as a baseline scene
 * state, and applying the given serialized mutations to that baseline.
 *
 * @param {!Uint8Array} serializedSnapshot
 * @param {!Array<!Uint8Array>} serializedMutations
 */
ink.SketchologyEngineWrapper.prototype.loadFromSerializedSnapshotAndMutations =
    function(serializedSnapshot, serializedMutations) {
  Module['loadFromSerializedSnapshotAndMutations'](
      this.engine_, serializedSnapshot, serializedMutations);
};


/**
 * Loads the engine by applying the given serialized mutations to an empty
 * scene.
 *
 * @param {!Array<!Uint8Array>} serializedMutations
 */
ink.SketchologyEngineWrapper.prototype.loadFromSerializedMutations = function(
    serializedMutations) {
  Module['loadFromSerializedMutations'](this.engine_, serializedMutations);
};


/**
 * Allows the user to execute arbitrary commands on the engine.
 * @param {!ink.proto.Command} command
 */
ink.SketchologyEngineWrapper.prototype.handleCommand = function(command) {
  var cCommandProto;
  try {
    cCommandProto = new (this.asmJs_.cFactory('CommandProto'))();
    cCommandProto['initFromJs'](command, this.wireSerializer_);
    this.engine_['handleCommand'](cCommandProto);
  } finally {
    if (cCommandProto) {
      cCommandProto['delete']();
    }
  }
};


/**
 * Gets the raw engine object. Do not use this.
 * @return {?Object}
 */
ink.SketchologyEngineWrapper.prototype.getRawEngineObject = function() {
  return this.engine_;
};


/**
 * Generates a snapshot based on a brix document.
 * @param {!ink.util.RealtimeDocument} brixDoc
 * @param {function(!ink.proto.Snapshot)} callback
 */
ink.SketchologyEngineWrapper.prototype.convertBrixDocumentToSnapshot = function(
    brixDoc, callback) {
  var snapshot = new ink.proto.Snapshot();
  var model = brixDoc.getModel();
  var root = model.getRoot();
  var pages = root.get('pages');
  var page = pages.get(0);
  if (!page) {
    throw Error('unable to get page from brix document.');
  }
  var elements = page.get('elements').asArray();
  for (var i = 0; i < elements.length; i++) {
    try {
      var cElementBundleProto =
          new (this.asmJs_.cFactory('ElementBundleProto'));
      var element = elements[i];
      if (!Module['brixElementToElementBundle'](
              element.get('id'), element.get('proto'), element.get('transform'),
              cElementBundleProto)) {
        throw Error('Unable to convert brix element to element bundle.');
      }
      var jsElementBundleProto = new ink.proto.ElementBundle();
      cElementBundleProto['copyToJs'](
          jsElementBundleProto, this.wireSerializer_);
      snapshot.addElement(jsElementBundleProto);
    } finally {
      cElementBundleProto['delete']();
    }
  }
  var pageProperties = new ink.proto.PageProperties();
  var rect = new ink.proto.Rect();
  var brixBounds = root.get('bounds');
  rect.setXhigh(brixBounds.xhigh || 0);
  rect.setXlow(brixBounds.xlow || 0);
  rect.setYhigh(brixBounds.yhigh || 0);
  rect.setYlow(brixBounds.ylow || 0);
  pageProperties.setBounds(rect);
  snapshot.setPageProperties(pageProperties);
  try {
    var cSnapshotProto = new (this.asmJs_.cFactory('SnapshotProto'))();
    cSnapshotProto['initFromJs'](snapshot, this.wireSerializer_);
    Module['SetFingerprint'](cSnapshotProto);
    snapshot = new ink.proto.Snapshot();
    cSnapshotProto['copyToJs'](snapshot, this.wireSerializer_);
  } finally {
    cSnapshotProto['delete']();
  }
  setTimeout(() => {
    callback(snapshot);
  });
};


/**
 * Calls the given callback once all previous asynchronous engine operations
 * have been applied.
 * @param {!Function} callback
 */
ink.SketchologyEngineWrapper.prototype.flush = function(callback) {
  this.sequencePointCallbacks_[this.nextSequencePointId_] = callback;
  try {
    var cSequencePointProto = new (this.asmJs_.cFactory('SequencePointProto'));
    cSequencePointProto['id'] = this.nextSequencePointId_++;
    this.engine_['addSequencePoint'](cSequencePointProto);
  } finally {
    cSequencePointProto['delete']();
  }
};


/**
 * Handles sequence point callbacks.
 * @param {number} id
 * @private
 */
ink.SketchologyEngineWrapper.prototype.onSequencePointReached_ = function(id) {
  var cb = this.sequencePointCallbacks_[id];
  delete this.sequencePointCallbacks_[id];
  cb();
};


/**
 * Adds a sticker.
 * @param {!Uint8ClampedArray} data
 * @param {!goog.math.Size} size
 * @param {!ink.Box} box
 */
ink.SketchologyEngineWrapper.prototype.addSticker = function(data, size, box) {
  try {
    var uri = 'sketchology://sticker_' + this.uriCounter_;
    this.uriCounter_++;
    this.addImageData(data, size, uri, ink.proto.ImageInfo.AssetType.STICKER);

    var imageRectCProto = new (this.asmJs_.cFactory('ImageRectProto'))();
    var rect = imageRectCProto['mutable_rect']();
    rect['xlow'] = box.left;
    rect['xhigh'] = box.right;
    rect['ylow'] = box.bottom;
    rect['yhigh'] = box.top;
    imageRectCProto['bitmap_uri'] = uri;
    imageRectCProto['mutable_attributes']()['is_sticker'] = true;
    this.engine_['addImageRect'](imageRectCProto);
  } finally {
    if (imageRectCProto) {
      imageRectCProto['delete']();
    }
  }
};


/**
 * True if PDF support is enabled.
 * @return {boolean}
 */
ink.SketchologyEngineWrapper.prototype.isPDFSupported = function() {
  return !!Module['loadPdfForAnnotation'];
};


/**
 * Adds a PDF as the background and loads any Ink strokes therein.
 * @param {!ArrayBuffer} pdf
 */
ink.SketchologyEngineWrapper.prototype.setPDF = function(pdf) {
  if (!this.isPDFSupported()) {
    throw Error('pdf support disabled.');
  }
  Module['loadPdfForAnnotation'](this.engine_, pdf);
};


/**
 * Returns pdf bytes.
 * @param {function(!Uint8Array)} callback
 */
ink.SketchologyEngineWrapper.prototype.getPDF = function(callback) {
  if (!this.isPDFSupported()) {
    throw Error('pdf support disabled.');
  }
  var pdfBytes = Module['getAnnotatedPdf'](this.engine_);
  setTimeout(() => callback(pdfBytes), 0);
};

/**
 * Returns pdf bytes, overwriting currently edited PDF in the process. Only use
 * this function if you're closing or reloading the Ink editor.
 * @return {!Uint8Array} bytes of annotated PDF
 */
ink.SketchologyEngineWrapper.prototype.getPDFDestructive = function() {
  if (!this.isPDFSupported()) {
    throw Error('pdf support disabled.');
  }
  return Module['getAnnotatedPdfDestructive'](this.engine_);
};


/**
 * Adds a new layer to the engine.
 *
 * @param {function(boolean)} callback Returns true if the call succeeded.
 */
ink.SketchologyEngineWrapper.prototype.addLayer = function(callback) {
  setTimeout(() => {
    callback(!!this.engine_['addLayer']());
  });
};


/**
 * Sets the visibility of the layer at index.
 *
 * @param {number} index
 * @param {boolean} visible
 * @param {function(boolean)} callback Returns true if the call succeeded.
 */
ink.SketchologyEngineWrapper.prototype.setLayerVisibility = function(
    index, visible, callback) {
  setTimeout(() => {
    callback(this.engine_['setLayerVisibility'](index, visible));
  });
};


/**
 * Sets the opacity of the layer at index.
 *
 * @param {number} index
 * @param {number} opacity
 * @param {function(boolean)} callback Returns true if the call succeeded.
 */
ink.SketchologyEngineWrapper.prototype.setLayerOpacity = function(
    index, opacity, callback) {
  setTimeout(() => {
    callback(this.engine_['setLayerOpacity'](index, opacity));
  });
};


/**
 * Makes the layer at index the active layer. All input will be
 * added to this layer.
 *
 * @param {number} index
 */
ink.SketchologyEngineWrapper.prototype.setActiveLayer = function(index) {
  this.engine_['setActiveLayer'](index);
};


/**
 * Move the layer at fromIndex to toIndex.
 *
 * @param {number} fromIndex
 * @param {number} toIndex
 * @param {function(boolean)} callback Returns true if the call succeeded.
 */
ink.SketchologyEngineWrapper.prototype.moveLayer = function(
    fromIndex, toIndex, callback) {
  setTimeout(() => {
    callback(this.engine_['moveLayer'](fromIndex, toIndex));
  });
};


/**
 * Remove the layer at index.
 *
 * @param {number} index
 * @param {function(boolean)} callback Returns true if the call succeeded.
 */
ink.SketchologyEngineWrapper.prototype.removeLayer = function(index, callback) {
  setTimeout(() => {
    callback(this.engine_['removeLayer'](index));
  });
};


/**
 * Returns the camera.
 *
 * @param {function(?ink.Box)} callback
 */
ink.SketchologyEngineWrapper.prototype.getCamera = function(callback) {
  try {
    var pos = this.engine_['getCameraPosition']();
    var center_x = pos['mutable_world_center']()['x'];
    var center_y = pos['mutable_world_center']()['y'];
    var width = pos['world_width'];
    var height = pos['world_height'];
    var box = new ink.Box({
      'top': center_y + .5 * height,
      'right': center_x + .5 * width,
      'bottom': center_y - .5 * height,
      'left': center_x - .5 * width
    });
    setTimeout(() => {
      callback(box);
    });
  } catch (e) {
    setTimeout(() => {
      callback(null);
    });
  } finally {
    if (pos) {
      pos.delete();
    }
  }
};


/**
 * Returns the page bounds in world coordinates.
 *
 * @param {function(?ink.Box)} callback
 */
ink.SketchologyEngineWrapper.prototype.getPageBounds = function(callback) {
  try {
    var pageProperties = this.engine_['document']()['GetPageProperties']();
    var bounds = pageProperties['bounds'];
    var box = new ink.Box({
      'top': bounds['yhigh'],
      'right': bounds['xhigh'],
      'bottom': bounds['ylow'],
      'left': bounds['xlow']
    });
    setTimeout(() => {
      callback(box);
    });
  } catch (e) {
    setTimeout(() => {
      callback(null);
    });
  } finally {
    if (pageProperties) {
      pageProperties['delete']();
    }
  }
};


/**
 * Returns the minimum bounding rect of strokes.
 *
 * @param {function(?ink.Box)} callback
 * @param {number=} opt_layerNumber
 */
ink.SketchologyEngineWrapper.prototype.getMinimumBoundingRect = function(
    callback, opt_layerNumber) {
  try {
    var rect;
    if (opt_layerNumber != null) {
      rect = this.engine_['getMinimumBoundingRectForLayer'](opt_layerNumber);
    } else {
      rect = this.engine_['getMinimumBoundingRect']();
    }
    var box = new ink.Box({
      'top': rect['Top'](),
      'right': rect['Right'](),
      'bottom': rect['Bottom'](),
      'left': rect['Left']()
    });
    setTimeout(() => {
      callback(box);
    });
  } catch (e) {
    setTimeout(() => {
      callback(null);
    });
  } finally {
    if (rect) {
      rect['delete']();
    }
  }
};


/**
 * Sets the effect that the mouse wheel has when scrolled.
 *
 * @param {!ink.proto.MouseWheelBehavior<number>} behavior  value from
 *     ink.proto.MouseWheelBehavior.*
 */
ink.SketchologyEngineWrapper.prototype.setMouseWheelBehavior = function(
    behavior) {
  this.engine_['setMouseWheelBehavior'](
      Module.MouseWheelBehavior['values'][behavior]);
};


/**
 * Does nothing.
 *
 * @param {!Function} callback
 */
ink.SketchologyEngineWrapper.prototype.waitForNaClLoad = function(callback) {
  setTimeout(callback);
};

/**
 * Get the state of the scene's layers.
 * @param {function(?ink.proto.LayerState)} callback
 */
ink.SketchologyEngineWrapper.prototype.getLayerState = function(callback) {
  const cLayerState = this.engine_['getLayerState']();
  try {
    const layerState = new ink.proto.LayerState();
    cLayerState['copyToJs'](layerState, this.wireSerializer_);
    setTimeout(() => {
      callback(layerState);
    });
  } catch (e) {
    setTimeout(() => {
      callback(null);
    });
  } finally {
    if (cLayerState) {
      cLayerState['delete']();
    }
  }
};


/**
 * Returns the current requested frame rate.
 * @return {number} requested FPS
 */
ink.SketchologyEngineWrapper.prototype.getRequestedFps = function() {
  return this.drawTimer_.getRequestedFps();
};


/**
 * Switches the current live renderer to the direct renderer.
 */
ink.SketchologyEngineWrapper.prototype.useDirectRenderer = function() {
  Module['useDirectRenderer'](this.engine_);
};


/**
 * Switches the current live renderer to the buffered renderer.
 */
ink.SketchologyEngineWrapper.prototype.useBufferedRenderer = function() {
  Module['useBufferedRenderer'](this.engine_);
};

/**
 * Turns the persistence of element outlines on or off.
 * @param {boolean} enabled True to include element outlines in Snapshots; false
 *     to not include them.
 */
ink.SketchologyEngineWrapper.prototype.setOutlineExportEnabled = function(
    enabled) {
  this.engine_['setOutlineExportEnabled'](enabled);
};

/**
 * Turns the persistence of handwriting-relevant stroke data on or off.
 * @param {boolean} enabled True to include x,y,t data in Snapshots; false to
 *     not include it.
 */
ink.SketchologyEngineWrapper.prototype.setHandwritingDataEnabled = function(
    enabled) {
  this.engine_['setHandwritingDataEnabled'](enabled);
};
