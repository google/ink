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

goog.provide('ink.UndoStateChangeEvent');

goog.require('goog.events');
goog.require('goog.events.Event');



/**
 * @param {boolean} canUndo Whether there is undo state available.
 * @param {boolean} canRedo Whether there is redo state available.
 * @constructor
 * @struct
 * @extends {goog.events.Event}
 */
ink.UndoStateChangeEvent = function(canUndo, canRedo) {
  ink.UndoStateChangeEvent.base(
      this, 'constructor',ink.UndoStateChangeEvent.EVENT_TYPE);
  this.canUndo = canUndo;
  this.canRedo = canRedo;
};
goog.inherits(ink.UndoStateChangeEvent, goog.events.Event);


/** @type {string} */
ink.UndoStateChangeEvent.EVENT_TYPE = goog.events.getUniqueId('undo-state');


/**
 * Getter necessary for easier exporting of these properties to non-google3
 *   users.
 * @return {boolean}
 */
ink.UndoStateChangeEvent.prototype.getCanUndo = function() {
  return this.canUndo;
};


/** @return {boolean} */
ink.UndoStateChangeEvent.prototype.getCanRedo = function() {
  return this.canRedo;
};
