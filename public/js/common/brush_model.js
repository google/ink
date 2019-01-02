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

goog.provide('ink.BrushModel');

goog.require('goog.asserts');
goog.require('goog.events');
goog.require('goog.events.EventTarget');
goog.require('ink.Model');
goog.require('ink.proto.BrushType');
goog.require('ink.proto.LineSize.SizeType');
goog.require('ink.proto.ToolParams.ToolType');


/**
 * Holds the state of the ink brush. Toolbar widgets update BrushModel, which in
 * turn dispatches CHANGE events to update the toolbar components' states.
 * @constructor
 * @extends {ink.Model}
 * @param {!goog.events.EventTarget} root Unused
 */
ink.BrushModel = function(root) {
  ink.BrushModel.base(this, 'constructor');
  /**
   * Last brush color (not including erase color).
   * @private {string}
   */
  this.color_ = ink.BrushModel.DEFAULT_DRAW_COLOR;

  /**
   * The active stroke width of the brush (includes erase size).
   * @private {number}
   */
  this.strokeWidth_ = ink.BrushModel.DEFAULT_DRAW_SIZE;

  /**
   * The units used for the active stroke width of the brush.
   * @private {!ink.proto.LineSize.SizeType}
   */
  this.strokeSizeType_ = ink.proto.LineSize.SizeType.PERCENT_WORLD;

  /**
   * @private {!ink.proto.ToolParams.ToolType}
   */
  this.toolType_ = ink.proto.ToolParams.ToolType.LINE;

  /**
   * @private {!ink.proto.BrushType}
   */
  this.brushType_ = ink.proto.BrushType.CALLIGRAPHY;

  /**
   * @private {string}
   */
  this.shape_ = 'CALLIGRAPHY';
};
goog.inherits(ink.BrushModel, ink.Model);

/**
 * @param {!goog.events.EventTarget} observer
 * @return {!ink.BrushModel}
 */
ink.BrushModel.getInstance = function(observer) {
  goog.asserts.assertObject(observer);
  return /** @type {!ink.BrushModel} */(
      ink.Model.get(ink.BrushModel, observer));
};


/**
 * @const {string}
 */
ink.BrushModel.DEFAULT_DRAW_COLOR = '#000000';


/**
 * @const {number}
 */
ink.BrushModel.DEFAULT_DRAW_SIZE = 0.6;


/**
 * @const {string}
 */
ink.BrushModel.DEFAULT_ERASE_COLOR = '#FAFAFA';


/**
 * The events fired by the BrushModel.
 * @enum {string} The event types for the BrushModel.
 */
ink.BrushModel.EventType = {
  /**
   * Fired when the BrushModel is changed.
   */
  CHANGE: goog.events.getUniqueId('change')
};


/**
 * @const {!Object}
 */
ink.BrushModel.SHAPE_TO_TOOLTYPE = {
  'CALLIGRAPHY': ink.proto.ToolParams.ToolType.LINE,
  'EDIT': ink.proto.ToolParams.ToolType.EDIT,
  'ERASER': ink.proto.ToolParams.ToolType.LINE,
  'HIGHLIGHTER': ink.proto.ToolParams.ToolType.LINE,
  'INKPEN': ink.proto.ToolParams.ToolType.LINE,
  'MAGIC_ERASE': ink.proto.ToolParams.ToolType.MAGIC_ERASE,
  'MARKER': ink.proto.ToolParams.ToolType.LINE,
  'PENCIL': ink.proto.ToolParams.ToolType.LINE,
  'PUSHER': ink.proto.ToolParams.ToolType.PUSHER,
  'CHARCOAL': ink.proto.ToolParams.ToolType.LINE,
  'BALLPOINT': ink.proto.ToolParams.ToolType.LINE,
  'BALLPOINT_IN_PEN_MODE_ELSE_MARKER': ink.proto.ToolParams.ToolType.LINE,
  'QUERY': ink.proto.ToolParams.ToolType.QUERY,
  'TEXT_HIGHLIGHTER_TOOL': ink.proto.ToolParams.ToolType.TEXT_HIGHLIGHTER_TOOL,
  'SMART_HIGHLIGHTER_TOOL':
      ink.proto.ToolParams.ToolType.SMART_HIGHLIGHTER_TOOL,
  'STROKE_EDITING_ERASER': ink.proto.ToolParams.ToolType.STROKE_EDITING_ERASER,
};


/**
 * @const {!Object}
 */
ink.BrushModel.SHAPE_TO_BRUSHTYPE = {
  'CALLIGRAPHY': ink.proto.BrushType.CALLIGRAPHY,
  'ERASER': ink.proto.BrushType.ERASER,
  'HIGHLIGHTER': ink.proto.BrushType.HIGHLIGHTER,
  'SMART_HIGHLIGHTER_TOOL': ink.proto.BrushType.HIGHLIGHTER,
  'INKPEN': ink.proto.BrushType.INKPEN,
  'MARKER': ink.proto.BrushType.MARKER,
  'PENCIL': ink.proto.BrushType.PENCIL,
  'CHARCOAL': ink.proto.BrushType.CHARCOAL,
  'BALLPOINT': ink.proto.BrushType.BALLPOINT,
  'BALLPOINT_IN_PEN_MODE_ELSE_MARKER':
      ink.proto.BrushType.BALLPOINT_IN_PEN_MODE_ELSE_MARKER,
};


/**
 * @param {string} color The color in hex, including a leading hash (e.g.,
 *   "#FF0000"). This can be formatted as either #AARRGGBB or #RRGGBB. If no
 *   alpha is specified, it will be set to 0xFF.
 */
ink.BrushModel.prototype.setColor = function(color) {
  this.color_ = color;
  this.dispatchEvent(ink.BrushModel.EventType.CHANGE);
};


/**
 * @param {number} strokeWidth The brush's stroke width. [0-1] if
 *   strokeSizeType is PERCENT_WORLD; otherwise some positive number with
 *   different semantics based on strokeSizeType. See the definition of
 *   LineSize.SizeType in sengine.proto for more details.
 * @param {!ink.proto.LineSize.SizeType=} strokeSizeType Units used to specify
 *   stroke width.
 */
ink.BrushModel.prototype.setStrokeWidth = function(
    strokeWidth, strokeSizeType = ink.proto.LineSize.SizeType.PERCENT_WORLD) {
  this.strokeWidth_ = strokeWidth;
  this.strokeSizeType_ = strokeSizeType;
  this.dispatchEvent(ink.BrushModel.EventType.CHANGE);
};


/**
 * @param {string} shape The brush shape, which is either a brush type or a tool
 * type.  If it's a brush type, implies tool type LINE.
 */
ink.BrushModel.prototype.setShape = function(shape) {
  this.toolType_ = ink.BrushModel.SHAPE_TO_TOOLTYPE[shape];
  this.brushType_ = ink.BrushModel.SHAPE_TO_BRUSHTYPE[shape] !== undefined ?
      ink.BrushModel.SHAPE_TO_BRUSHTYPE[shape] :
      this.brushType_;
  this.shape_ = shape;
  this.dispatchEvent(ink.BrushModel.EventType.CHANGE);
};


/**
 * @return {string} The last used shape.
 */
ink.BrushModel.prototype.getShape = function() {
  return this.shape_;
};


/**
 * @return {string} The last draw color in hex (excluding erase color).
 * Includes a leading hash (e.g. "#FF0000").
 */
ink.BrushModel.prototype.getColor = function() {
  return this.color_;
};


/**
 * Gets the current color being drawn on the screen (including erase color).
 * @return {string} The brush color in hex. Includes a leading hash
 * (e.g. "#FF0000").
 */
ink.BrushModel.prototype.getActiveColor = function() {
  if (this.isErasing()) {
    return ink.BrushModel.DEFAULT_ERASE_COLOR;
  } else {
    return this.color_;
  }
};


/**
 * Wraps getActiveColor() by returning the numeric rgb of the color.
 * @return {number} The brush color in numeric rbg.
 */
ink.BrushModel.prototype.getActiveColorNumericRbg = function() {
  return parseInt(this.getActiveColor().substring(1), 16);
};


/**
 * @return {number} Size for stroke width. Range depends on
 *   getStrokeSizeType(). [0-1] if strokeSizeType is PERCENT_WORLD; otherwise
 *   some positive number with different semantics based on strokeSizeType.
 *   See the definition of LineSize.SizeType in sengine.proto for more details.
 */
ink.BrushModel.prototype.getStrokeWidth = function() {
  return this.strokeWidth_;
};


/**
 * @return {!ink.proto.LineSize.SizeType} Units used for stroke width.
 *
 * See sengine.proto SizeType
 */
ink.BrushModel.prototype.getStrokeSizeType = function() {
  return this.strokeSizeType_;
};


/**
 * @return {!ink.proto.BrushType} The brush type for line
 * tool.
 */
ink.BrushModel.prototype.getBrushType = function() {
  return this.brushType_;
};


/**
 * @return {!ink.proto.ToolParams.ToolType} The tool type.
 */
ink.BrushModel.prototype.getToolType = function() {
  return this.toolType_;
};


/**
 * @return {boolean} True if current tool type is an eraser type.
 */
ink.BrushModel.prototype.isErasing = function() {
  return this.toolType_ == ink.proto.ToolParams.ToolType.MAGIC_ERASE ||
      this.toolType_ == ink.proto.ToolParams.ToolType.STROKE_EDITING_ERASER ||
      (this.toolType_ == ink.proto.ToolParams.ToolType.LINE &&
       this.brushType_ == ink.proto.BrushType.ERASER);
};
