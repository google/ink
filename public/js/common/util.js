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
goog.require('goog.html.SafeUrl');
goog.require('goog.math.Size');
goog.require('ink.proto.ElementState');
goog.require('ink.proto.ImageInfo');
goog.require('ink.proto.MutationPacket');
goog.require('protos.research.ink.InkEvent');



/**
 * Decoded RGBA image data and size.  Must have data.length === 4 * size.width *
 * size.height.
 *
 * @typedef {{
 *   data: !Uint8ClampedArray,
 *   size: !goog.math.Size
 * }}
 */
ink.util.DecodedImage;


/**
 * An image URL, or encoded image data (as a Blob), or decoded image data.  If a
 * Blob is used, it must have an appropriate MIME type set in the Blob options.
 *
 * @typedef {string|!Blob|!ink.util.DecodedImage}
 */
ink.util.ImageSource;


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
 * @param {!ink.util.ImageSource} imgSrc
 * @param {function(!Uint8ClampedArray, !goog.math.Size)=} callback Deprecated.
 *     Prefer using the Promise.
 * @return {!Promise<!ink.util.DecodedImage>}
 */
ink.util.getImageBytes = function(imgSrc, callback) {
  let shouldRevokeUrl = false;
  let url;
  if (typeof imgSrc === 'string') {
    url = imgSrc;
  } else if (imgSrc instanceof Blob) {
    url = goog.html.SafeUrl.unwrap(goog.html.SafeUrl.fromBlob(imgSrc));
    shouldRevokeUrl = true;
  } else {
    const expectedLength = 4 * imgSrc.size.width * imgSrc.size.height;
    if (imgSrc.data.length !== expectedLength) {
      return Promise.reject(new Error(
          'Image RGBA data length is ' + imgSrc.data.length +
          ', but expected ' + expectedLength + ' for a ' + imgSrc.size.width +
          'x' + imgSrc.size.height + ' image'));
    }
    return Promise.resolve(imgSrc);
  }

  const canvasElement = goog.dom.createElement(goog.dom.TagName.CANVAS);
  const imgElement = goog.dom.createElement(goog.dom.TagName.IMG);
  imgElement.setAttribute(
      'style',
      'position:absolute;visibility:hidden;top:-1000px;left:-1000px;');
  imgElement.crossOrigin = 'Anonymous';

  let promise = new Promise((resolve, reject) => {
    goog.events.listenOnce(imgElement, 'load', () => {
      const width = imgElement.naturalWidth || imgElement.width;
      const height = imgElement.naturalHeight || imgElement.height;
      canvasElement.width = width;
      canvasElement.height = height;
      const ctx = goog.dom.getCanvasContext2D(canvasElement);
      ctx.drawImage(imgElement, 0, 0);
      const data = ctx.getImageData(0, 0, width, height);
      goog.asserts.assert(data.data);

      document.body.removeChild(imgElement);
      resolve({
        data: data.data,
        size: new goog.math.Size(width, height),
      });
    });

    goog.events.listenOnce(imgElement, 'error', () => {
      document.body.removeChild(imgElement);
      reject(new Error('getImageBytes failed on ' + url));
    });

    imgElement.setAttribute('src', url);
    document.body.appendChild(imgElement);
  });
  if (shouldRevokeUrl) {
    promise = promise.finally(() => {
      URL.revokeObjectURL(url);
    });
  }
  if (callback) {
    promise = promise.then((image) => {
      callback(image.data, image.size);
      return image;
    });
  }
  return promise;
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



/** @private @const {string} */
ink.util.HOST_ASSET_URI_PREFIX_ = 'host:';


/** @private @const {!RegExp} */
ink.util.HOST_ASSET_URI_REGEXP_ =
    new RegExp('^' + ink.util.HOST_ASSET_URI_PREFIX_ + '([A-Z_]+):(.*)$');


/**
 * @param {!ink.proto.ImageInfo.AssetType} assetType
 * @param {string} assetId
 * @return {string}
 */
ink.util.buildHostUri = function(assetType, assetId) {
  const assetTypeName = ink.util.assetTypeName(assetType);
  return `${ink.util.HOST_ASSET_URI_PREFIX_}${assetTypeName}:${assetId}`;
};


/**
 * @param {string} uri
 * @return {?{assetType:!ink.proto.ImageInfo.AssetType, assetId:string}}
 */
ink.util.parseHostUri = function(uri) {
  const match = uri.match(ink.util.HOST_ASSET_URI_REGEXP_);
  if (!match) {
    return null;
  }
  const assetTypeName = match[1];
  const assetId = match[2];
  const assetType = ink.util.parseAssetType(assetTypeName);
  if (assetType === null) {
    return null;
  }
  return {assetType, assetId};
};


/** @private @enum {string} */
ink.util.AssetTypeName_ = {
  DEFAULT: 'DEFAULT',
  BORDER: 'BORDER',
  STICKER: 'STICKER',
  GRID: 'GRID',
  TILED_TEXTURE: 'TILED_TEXTURE',
};


/**
 * @param {!ink.proto.ImageInfo.AssetType} assetType
 * @return {string}
 */
ink.util.assetTypeName = function(assetType) {
  switch (assetType) {
    case ink.proto.ImageInfo.AssetType.DEFAULT:
      return ink.util.AssetTypeName_.DEFAULT;
    case ink.proto.ImageInfo.AssetType.BORDER:
      return ink.util.AssetTypeName_.BORDER;
    case ink.proto.ImageInfo.AssetType.STICKER:
      return ink.util.AssetTypeName_.STICKER;
    case ink.proto.ImageInfo.AssetType.GRID:
      return ink.util.AssetTypeName_.GRID;
    case ink.proto.ImageInfo.AssetType.TILED_TEXTURE:
      return ink.util.AssetTypeName_.TILED_TEXTURE;
  }
  return `UNKNOWN(${assetType})`;
};


/**
 * @param {string} name
 * @return {?ink.proto.ImageInfo.AssetType}
 */
ink.util.parseAssetType = function(name) {
  switch (name) {
    case ink.util.AssetTypeName_.DEFAULT:
      return ink.proto.ImageInfo.AssetType.DEFAULT;
    case ink.util.AssetTypeName_.BORDER:
      return ink.proto.ImageInfo.AssetType.BORDER;
    case ink.util.AssetTypeName_.STICKER:
      return ink.proto.ImageInfo.AssetType.STICKER;
    case ink.util.AssetTypeName_.GRID:
      return ink.proto.ImageInfo.AssetType.GRID;
    case ink.util.AssetTypeName_.TILED_TEXTURE:
      return ink.proto.ImageInfo.AssetType.TILED_TEXTURE;
    default:
      return null;
  }
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
      const uuid = action.getAddAction().getUuid();
      if (!addElementToMutationPacket(uuid, elementMap, ret)) {
        console.log(`Couldn't add element to mutation packet ${uuid}`);
        return null;
      }
    } else if (action.hasAddMultipleAction()) {
      const uuidList = action.getAddMultipleAction().uuidArray();
      for (const uuid of uuidList) {
        if (!addElementToMutationPacket(uuid, elementMap, ret)) {
          console.log(`Couldn't add element to mutation packet ${uuid}`);
          return null;
        }
      }
    } else if (action.hasReplaceAction()) {
      const uuidList = action.getReplaceAction().addedUuidArray();
      for (const uuid of uuidList) {
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

