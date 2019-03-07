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
 * @fileoverview Implementation of Sketchology HostController interface for web.
 * @suppress {const}
 */
goog.provide('ink.HostController');

goog.require('ink.AsmJsWrapper');
goog.require('ink.ElementListener');
goog.require('ink.proto.Cursor');
goog.require('ink.proto.ToolEvent');
goog.require('ink.proto.scene_change.SceneChangeEvent');
goog.require('net.proto2.contrib.WireSerializer');

/**
 * Creates a JS object that implements the C++ Host abstract class.
 *
 * @export
 * @constructor
 * @param {!ink.ElementListener} elementListener
 * @param {function(number)} setFps
 * @param {function():number} getFps
 * @param {function(number, number, !Uint8ClampedArray)} imageExportComplete
 * callback handler for image export, the provided Uint8ClampedArray is only
 * valid for the lifetime of the callback.  The backing data will be collected
 * after the call stack completes.
 * @param {function(number, boolean)} onFlagChanged
 * @param {function(boolean, boolean)} onUndoRedoStateChanged
 * @param {function(number)} onSequencePointReached
 * @param {function(string)} requestImage
 * @param {function(!ink.proto.Cursor)} onSetCursor
 * @param {function(!Uint8Array)} onMutation
 * @param {function(!ink.proto.scene_change.SceneChangeEvent)} onSceneChanged
 * @param {function(!ink.proto.ToolEvent)} onToolEvent
 * @param {function(boolean)} onCameraMovementStateChanged
 * @param {function(boolean)} onBlockingStateChanged
 *
 * @return {!ink.HostController}
 */
ink.HostController = function(
    elementListener, setFps, getFps, imageExportComplete, onFlagChanged,
    onUndoRedoStateChanged, onSequencePointReached, requestImage, onSetCursor,
    onMutation, onSceneChanged, onToolEvent, onCameraMovementStateChanged,
    onBlockingStateChanged) {
  /** @private {!ink.ElementListener} */
  this.elementListener_;

  /** @private {function(number)} */
  this.setFps_;

  /** @private {function():number} */
  this.getFps_;

  /** @private {function(number, number, !Uint8ClampedArray)} */
  this.imageExportComplete_;

  /** @private {function(number, boolean)} */
  this.onFlagChanged_;

  /** @private {function(boolean, boolean)} */
  this.onUndoRedoStateChanged_;

  /** @private {function(number)} */
  this.onSequencePointReached_;

  /** @private {function(string)} */
  this.requestImage_;

  /** @private {function(!ink.proto.Cursor)} */
  this.onSetCursor_;

  /** @private {function(!Uint8Array)} */
  this.onMutation_;

  /** @private {function(!ink.proto.scene_change.SceneChangeEvent)} */
  this.onSceneChanged_;

  /** @private {function(!ink.proto.ToolEvent)} */
  this.onToolEvent_;

  /** @private {function(boolean)} */
  this.onCameraMovementStateChanged_;

  /** @private {function(boolean)} */
  this.onBlockingStateChanged_;

  /**
   * Protocol Buffer wire format serializer.
   * @type {!net.proto2.contrib.WireSerializer}
   * @private
   */
  this.wireSerializer_;

  // The actual HostController constructor depends on first initializing the
  // emscripten code with the C++ bindings.  So we delay assigning the
  // constructor until first use of HostController.
  var constructor = ink.AsmJsWrapper.getInstance().extendClass(
      'Host', ink.HostController.prototype);
  if (constructor) {
    ink.HostController = constructor;
    return /** @type {!ink.HostController} */ (new ink.HostController(
        elementListener, setFps, getFps, imageExportComplete, onFlagChanged,
        onUndoRedoStateChanged, onSequencePointReached, requestImage,
        onSetCursor, onMutation, onSceneChanged, onToolEvent,
        onCameraMovementStateChanged, onBlockingStateChanged));
  } else {
    // Emscripten bindings not initialized.
    throw new Error('Bindings are uninitialized.');
  }
};

/**
 * Override of C++ constructor for HostController sub-class.
 *
 * @export
 * @param {!ink.ElementListener} elementListener
 * @param {function(number)} setFps
 * @param {function():number} getFps
 * @param {function(number, number, !Uint8ClampedArray)} imageExportComplete
 * @param {function(number, boolean)} onFlagChanged
 * @param {function(boolean, boolean)} onUndoRedoStateChanged
 * @param {function(number)} onSequencePointReached
 * @param {function(string)} requestImage
 * @param {function(!ink.proto.Cursor)} onSetCursor
 * @param {function(!Uint8Array)} onMutation
 * @param {function(!ink.proto.scene_change.SceneChangeEvent)} onSceneChanged
 * @param {function(!ink.proto.ToolEvent)} onToolEvent
 * @param {function(boolean)} onCameraMovementStateChanged
 * @param {function(boolean)} onBlockingStateChanged
 */
ink.HostController.prototype.__construct = function(
    elementListener, setFps, getFps, imageExportComplete, onFlagChanged,
    onUndoRedoStateChanged, onSequencePointReached, requestImage, onSetCursor,
    onMutation, onSceneChanged, onToolEvent, onCameraMovementStateChanged,
    onBlockingStateChanged) {
  this['__parent']['__construct'].call(this);
  this.elementListener_ = elementListener;
  this.setFps_ = setFps;
  this.getFps_ = getFps;
  this.imageExportComplete_ = imageExportComplete;
  this.onFlagChanged_ = onFlagChanged;
  this.onUndoRedoStateChanged_ = onUndoRedoStateChanged;
  this.onSequencePointReached_ = onSequencePointReached;
  this.requestImage_ = requestImage;
  this.onSetCursor_ = onSetCursor;
  this.onMutation_ = onMutation;
  this.onSceneChanged_ = onSceneChanged;
  this.onToolEvent_ = onToolEvent;
  this.onCameraMovementStateChanged_ = onCameraMovementStateChanged;
  this.onBlockingStateChanged_ = onBlockingStateChanged;
  this.wireSerializer_ = new net.proto2.contrib.WireSerializer();
};

/**
 * @export
 * @return {number} current platform FPS.
 */
ink.HostController.prototype.getTargetFPS = function() {
  var self = /** @type {!ink.HostController} */ (this);
  return self.getFps_();
};

/**
 * @export
 * @param {number} fps
 */
ink.HostController.prototype.setTargetFPS = function(fps) {
  var self = /** @type {!ink.HostController} */ (this);
  self.setFps_(fps);
};

/**
 * @export
 */
ink.HostController.prototype.bindScreen = function() {
  var glContext = ink.AsmJsWrapper.getInstance().getGLContext();
  glContext.bindFramebuffer(glContext.FRAMEBUFFER, null);
};

/**
 * @export
 * @param {string} uri
 */
ink.HostController.prototype.requestImage = function(uri) {
  var self = /** @type {!ink.HostController}*/ (this);
  self.requestImage_(uri);
};


/**
 * @export
 * @param {!Uint8Array} serializedCursor
 */
ink.HostController.prototype.setCursor = function(serializedCursor) {
  var self = /** @type {!ink.HostController} */ (this);
  const cursorProto = new ink.proto.Cursor();
  self.wireSerializer_.deserializeTo(cursorProto, serializedCursor);
  self.onSetCursor_(cursorProto);
};


/**
 * @export
 * @param {!Uint8Array} serializedMutation
 */
ink.HostController.prototype.handleMutation = function(serializedMutation) {
  var self = /** @type {!ink.HostController} */ (this);
  self.onMutation_(serializedMutation);
};


/**
 * @export
 * @param {string} uuid
 * @param {string} encodedElement
 * @param {string} encodedTransform
 */
ink.HostController.prototype.handleElementCreated = function(
    uuid, encodedElement, encodedTransform) {
  var self = /** @type {!ink.HostController} */ (this);
  self.elementListener_.onElementCreated(
      uuid, encodedElement, encodedTransform);
};

/**
 * @export
 * @param {!Array.<string>} uuids
 * @param {!Array.<string>} encodedTransforms
 */
ink.HostController.prototype.handleElementsMutated = function(
    uuids, encodedTransforms) {
  var self = /** @type {!ink.HostController} */ (this);
  self.elementListener_.onElementsMutated(uuids, encodedTransforms);
};

/**
 * @export
 * @param {!Array.<string>} uuids
 */
ink.HostController.prototype.handleElementsRemoved = function(uuids) {
  var self = /** @type {!ink.HostController} */ (this);
  self.elementListener_.onElementsRemoved(uuids);
};

/**
 * Called when PNG export is complete.  Start with
 * ink.SketchologyEngineWrapper.startPngExport.
 *
 * @export
 * @param {number} widthPx
 * @param {number} heightPx
 * @param {!Uint8Array} imageData image data backed by emscripten heap
 */
ink.HostController.prototype.onImageExportComplete = function(
    widthPx, heightPx, imageData) {
  var self = /** @type {!ink.HostController} */ (this);
  self.imageExportComplete_(
      widthPx, heightPx,
      new Uint8ClampedArray(
          imageData.buffer, imageData.byteOffset, imageData.byteLength));
};

/**
 * @export
 * @return {string}
 */
ink.HostController.prototype.getPlatformId = function() {
  return 'asmjs/webgl';
};

/**
 * @export
 * @param {number} sequencePointId
 */
ink.HostController.prototype.onSequencePointReached = function(
    sequencePointId) {
  var self = /** @type {!ink.HostController} */ (this);
  self.onSequencePointReached_(sequencePointId);
};

/**
 * @export
 * @param {number} flag the flag enum value
 * @param {boolean} enabled
 */
ink.HostController.prototype.onFlagChanged = function(flag, enabled) {
  var self = /** @type {!ink.HostController} */ (this);
  self.onFlagChanged_(flag, enabled);
};


/**
 * @export
 * @param {boolean} canUndo
 * @param {boolean} canRedo
 */
ink.HostController.prototype.onUndoRedoStateChanged = function(
    canUndo, canRedo) {
  var self = /** @type {!ink.HostController} */ (this);
  self.onUndoRedoStateChanged_(canUndo, canRedo);
};


/**
 * @export
 * @param {!Uint8Array} sceneChangedEvent
 */
ink.HostController.prototype.onSceneChanged = function(sceneChangedEvent) {
  var self = /** @type {!ink.HostController} */ (this);
  const sceneChangedProto = new ink.proto.scene_change.SceneChangeEvent();
  self.wireSerializer_.deserializeTo(sceneChangedProto, sceneChangedEvent);
  self.onSceneChanged_(sceneChangedProto);
};


/**
 * @export
 * @param {!Uint8Array} toolEvent
 */
ink.HostController.prototype.onToolEvent = function(toolEvent) {
  var self = /** @type {!ink.HostController} */ (this);
  const toolProto = new ink.proto.ToolEvent();
  self.wireSerializer_.deserializeTo(toolProto, toolEvent);
  self.onToolEvent_(toolProto);
};


/**
 * @export
 * @param {boolean} isMoving
 */
ink.HostController.prototype.onCameraMovementStateChanged = function(isMoving) {
  var self = /** @type {!ink.HostController} */ (this);
  self.onCameraMovementStateChanged_(isMoving);
};

/**
 * @export
 * @param {boolean} isBlocked
 */
ink.HostController.prototype.onBlockingStateChanged = function(isBlocked) {
  var self = /** @type {!ink.HostController} */ (this);
  self.onBlockingStateChanged_(isBlocked);
};

goog.exportSymbol('ink.HostController', ink.HostController);
goog.exportProperty(
    ink.HostController.prototype, '__construct',
    ink.HostController.prototype.__construct);
goog.exportProperty(
    ink.HostController.prototype, 'setTargetFPS',
    ink.HostController.prototype.setTargetFPS);
goog.exportProperty(
    ink.HostController.prototype, 'getTargetFPS',
    ink.HostController.prototype.getTargetFPS);
goog.exportProperty(
    ink.HostController.prototype, 'bindScreen',
    ink.HostController.prototype.bindScreen);
goog.exportProperty(
    ink.HostController.prototype, 'requestImage',
    ink.HostController.prototype.requestImage);
goog.exportProperty(
    ink.HostController.prototype, 'setCursor',
    ink.HostController.prototype.setCursor);
goog.exportProperty(
    ink.HostController.prototype, 'handleElementCreated',
    ink.HostController.prototype.handleElementCreated);
goog.exportProperty(
    ink.HostController.prototype, 'handleElementsMutated',
    ink.HostController.prototype.handleElementsMutated);
goog.exportProperty(
    ink.HostController.prototype, 'handleElementsRemoved',
    ink.HostController.prototype.handleElementsRemoved);
goog.exportProperty(
    ink.HostController.prototype, 'handleMutation',
    ink.HostController.prototype.handleMutation);
goog.exportProperty(
    ink.HostController.prototype, 'onImageExportComplete',
    ink.HostController.prototype.onImageExportComplete);
goog.exportProperty(
    ink.HostController.prototype, 'getPlatformId',
    ink.HostController.prototype.getPlatformId);
goog.exportProperty(
    ink.HostController.prototype, 'onSequencePointReached',
    ink.HostController.prototype.onSequencePointReached);
goog.exportProperty(
    ink.HostController.prototype, 'onFlagChanged',
    ink.HostController.prototype.onFlagChanged);
goog.exportProperty(
    ink.HostController.prototype, 'onUndoRedoStateChanged',
    ink.HostController.prototype.onUndoRedoStateChanged);
goog.exportProperty(
    ink.HostController.prototype, 'onSceneChanged',
    ink.HostController.prototype.onSceneChanged);
goog.exportProperty(
    ink.HostController.prototype, 'onToolEvent',
    ink.HostController.prototype.onToolEvent);
goog.exportProperty(
    ink.HostController.prototype, 'onCameraMovementStateChanged',
    ink.HostController.prototype.onCameraMovementStateChanged);
goog.exportProperty(
    ink.HostController.prototype, 'onBlockingStateChanged',
    ink.HostController.prototype.onBlockingStateChanged);
