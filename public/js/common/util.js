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

goog.provide('ink.util');

goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.math.Size');
goog.require('ink.proto.ElementState');
goog.require('ink.proto.MutationPacket');
goog.require('protos.research.ink.InkEvent');

/**
 * @typedef {{
 *   getModel: (function():!ink.util.RealtimeModel)
 * }}
 */
ink.util.RealtimeDocument;

/**
 * @typedef {{
 *  getRoot: (function():!ink.util.RealtimeRoot)
 * }}
 */
ink.util.RealtimeModel;


/**
 * @typedef {{
 *   get: (function(string):!ink.util.RealtimePages)
 * }}
 */
ink.util.RealtimeRoot;


/**
 * @typedef {{
 *   get: (function(number):?ink.util.RealtimePage),
 *   xhigh: number,
 *   xlow: number,
 *   yhigh: number,
 *   ylow: number
 * }}
 */
ink.util.RealtimePages;


/**
 * @typedef {{
 *   get: (function(string):!ink.util.RealtimeElements)
 * }}
 */
ink.util.RealtimePage;


/**
 * @typedef {{
 *   asArray: (function():!Array.<!ink.util.RealtimeElement>)
 * }}
 */
ink.util.RealtimeElements;


/**
 * @typedef {{
 *   get: (function(string):string)
 * }}
 */
ink.util.RealtimeElement;


/**
 * @param {!goog.events.EventTarget} child The child to get the root parent
 *     EventTarget for.
 * @return {!goog.events.EventTarget} The root parent.
 */
ink.util.getRootParentComponent = function(child) {
  var current = child;
  var parent = current.getParentEventTarget();
  while (parent) {
    current = parent;
    parent = current.getParentEventTarget();
  }
  return current;
};


/**
 * Downloads an image, renders it to a canvas, returns the raw bytes.
 * @param {string} imgSrc
 * @param {function(!Uint8ClampedArray, !goog.math.Size)} callback
 */
ink.util.getImageBytes = function(imgSrc, callback) {
  var canvasElement = goog.dom.createElement(goog.dom.TagName.CANVAS);
  var imgElement = goog.dom.createElement(goog.dom.TagName.IMG);
  imgElement.setAttribute(
      'style',
      'position:absolute;visibility:hidden;top:-1000px;left:-1000px;');
  imgElement.crossOrigin = 'Anonymous';

  goog.events.listenOnce(imgElement, 'load', function() {
    var width = imgElement.naturalWidth || imgElement.width;
    var height = imgElement.naturalHeight || imgElement.height;
    canvasElement.width = width;
    canvasElement.height = height;
    var ctx = goog.dom.getCanvasContext2D(canvasElement);
    ctx.drawImage(imgElement, 0, 0);
    var data = ctx.getImageData(0, 0, width, height);
    goog.asserts.assert(data.data);

    document.body.removeChild(imgElement);

    callback(data.data, new goog.math.Size(width, height));
  });

  imgElement.setAttribute('src', imgSrc);
  document.body.appendChild(imgElement);
};


/**
 * Creates document events.
 * @param {!protos.research.ink.InkEvent.Host} host
 * @param {!protos.research.ink.InkEvent.DocumentEvent.DocumentEventType} type
 * @return {!protos.research.ink.InkEvent}
 */
ink.util.createDocumentEvent = function(host, type) {
  var eventProto = new protos.research.ink.InkEvent();
  eventProto.setHost(host);
  eventProto.setEventType(
      protos.research.ink.InkEvent.EventType.DOCUMENT_EVENT);
  var documentEvent = new protos.research.ink.InkEvent.DocumentEvent();
  documentEvent.setEventType(type);
  eventProto.setDocumentEvent(documentEvent);
  return eventProto;
};


/**
 * @const
 */
ink.util.SHAPE_TO_LOG_TOOLTYPE = {
  'CALLIGRAPHY': protos.research.ink.InkEvent.ToolbarEvent.ToolType.CALLIGRAPHY,
  'EDIT': protos.research.ink.InkEvent.ToolbarEvent.ToolType.EDIT_TOOL,
  'HIGHLIGHTER': protos.research.ink.InkEvent.ToolbarEvent.ToolType.HIGHLIGHTER,
  'MAGIC_ERASE': protos.research.ink.InkEvent.ToolbarEvent.ToolType.MAGIC_ERASER,
  'MARKER': protos.research.ink.InkEvent.ToolbarEvent.ToolType.MARKER
};


/**
 * Creates toolbar events.
 * @param {!protos.research.ink.InkEvent.Host} host
 * @param {!protos.research.ink.InkEvent.ToolbarEvent.ToolEventType} type
 * @param {string} toolType
 * @param {string} color
 * @return {!protos.research.ink.InkEvent}
 */
ink.util.createToolbarEvent = function(host, type, toolType, color) {
  var eventProto = new protos.research.ink.InkEvent();
  eventProto.setHost(host);
  eventProto.setEventType(protos.research.ink.InkEvent.EventType.TOOLBAR_EVENT);
  var toolbarEvent = new protos.research.ink.InkEvent.ToolbarEvent();
  // Alpha is always 0xFF. This does #RRGGBB -> #AARRGGBB -> 0xAARRGGBB.
  var colorAsNumber = parseInt('ff' + color.substring(1, 7), 16);
  toolbarEvent.setColor(colorAsNumber);
  toolbarEvent.setToolType(
      ink.util.SHAPE_TO_LOG_TOOLTYPE[toolType] ||
      protos.research.ink.InkEvent.ToolbarEvent.ToolType.UNKNOWN_TOOL_TYPE);
  toolbarEvent.setToolEventType(type);
  eventProto.setToolbarEvent(toolbarEvent);
  return eventProto;
};


// Note: These helpers are included here because we do not use the closure
// browser event wrappers to avoid additional GC pauses.  Logic forked from
// javascript/closure/events/browserevent.js?l=337

/**
 * "Action button" is  MouseEvent.button equal to 0 (main button) and no
 * control-key for Mac right-click action.  The button property is the one
 * that was responsible for triggering a mousedown or mouseup event.
 *
 * @param {!MouseEvent} evt
 * @return {boolean}
 */
ink.util.isMouseActionButton = function(evt) {
  return evt.button == 0 &&
         !(goog.userAgent.WEBKIT && goog.userAgent.MAC && evt.ctrlKey);
};


/**
 * MouseEvent.buttons has 1 (main button) held down, and no control-key
 * for Mac right-click drag.  This is for which button is currently held down,
 * e.g. during a mousemove event.
 *
 * @param {!MouseEvent} evt
 * @return {boolean}
 */
ink.util.hasMouseActionButton = function(evt) {
  return (evt.buttons & 1) == 1 &&
         !(goog.userAgent.WEBKIT && goog.userAgent.MAC && evt.ctrlKey);
};


/**
 * Checks MouseEvent.buttons has 2 (secondary button) held down, or 1 (main
 * button) with control key for Mac right-click drag.  This is for which
 * button is currently held down, e.g. during a mousemove event.
 *
 * @param {!MouseEvent} evt
 * @return {boolean}
 */
ink.util.hasMouseSecondaryButton = function(evt) {
  return (evt.buttons & 2) == 2 ||
         ((evt.buttons & 1) == 1 && goog.userAgent.WEBKIT &&
          goog.userAgent.MAC && evt.ctrlKey);
};


/**
 * Extracts pending mutations from the given Snapshot and returns them unless
 * the given Snapshot is in an unrecoverable inconsistent state, in which case
 * null is returned. If the given Snapshot has no pending mutations, returns
 * null.
 *
 * This is a js implementation of ExtractMutationPacket:
 * ink/public/mutations/mutations.cc
 *
 * @param {!ink.proto.Snapshot} snapshot
 * @return {?ink.proto.MutationPacket}
 */
ink.util.extractMutationPacket = function(snapshot) {
  function addElementToMutationPacket(uuid, elementMap, ret) {
    if (!elementMap.has(uuid)) {
      return false;
    }
    ret.addElement(elementMap.get(uuid));
    return true;
  }

  var ret = new ink.proto.MutationPacket();

  if (snapshot.hasPageProperties()) {
    ret.setPageProperties(snapshot.getPagePropertiesOrDefault());
  }

  if (!snapshot.undoActionCount()) {
    return ret;
  }

  var elementMap = new Map();
  var elements = snapshot.elementArray();
  for (var i = 0; i < elements.length; i++) {
    var element = elements[i];
    elementMap.set(element.getUuid(), element);
  }

  elements = snapshot.deadElementArray();
  for (i = 0; i < elements.length; i++) {
    element = elements[i];
    elementMap.set(element.getUuid(), element);
  }

  var actions = snapshot.undoActionArray();
  for (i = 0; i < actions.length; i++) {
    var action = actions[i];
    ret.addMutation(action);
    if (action.hasAddAction()) {
      var uuid = action.getAddAction().getUuid();
      if (!addElementToMutationPacket(uuid, elementMap, ret)) {
        console.log(`Couldn't add element to mutation packet ${uuid}`);
        return null;
      }
    } else if (action.hasAddMultipleAction()) {
      var uuidList = action.getAddMultipleAction().uuidArray();
      for (var j = 0; j < uuidList.length; j++) {
        var uuid = uuidList[j];
        if (!addElementToMutationPacket(uuid, elementMap, ret)) {
          console.log(`Couldn't add element to mutation packet ${uuid}`);
          return null;
        }
      }
    }
  }
  return ret;
};


/**
 * Returns true if the given snapshot has a pending mutation packet.
 *
 * @param {!ink.proto.Snapshot} snapshot
 * @return {boolean}
 */
ink.util.snapshotHasPendingMutationPacket = function(snapshot) {
  return snapshot.undoActionCount() > 0;
};


/**
 * Given a source Snapshot, clears the target and populates it with the source
 * Snapshot, minus any pending mutations. The target Snapshot will have no
 * undo/redo stack.
 *
 * @param {!ink.proto.Snapshot} snapshot
 */
ink.util.clearPendingMutationPacket = function(snapshot) {
  snapshot.clearDeadElement();
  snapshot.clearUndoAction();
  snapshot.clearRedoAction();
  snapshot.clearElementStateIndex();
  for (var i = 0; i < snapshot.elementCount(); i++) {
    snapshot.addElementStateIndex(ink.proto.ElementState.ALIVE);
  }
};

