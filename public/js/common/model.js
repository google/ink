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

goog.provide('ink.Model');

goog.require('goog.events.EventTarget');
goog.require('ink.util');



/**
 * A generic model base class. Each models here is a singleton per EventTarget
 * hierarchy tree.
 *
 * @extends {goog.events.EventTarget}
 * @constructor
 * @struct
 */
ink.Model = function() {
  ink.Model.base(this, 'constructor');
};
goog.inherits(ink.Model, goog.events.EventTarget);


/**
 * The models are attached to the root parent EventTargets. To have the models
 * be automatically gc properly the relevant models need to actually be
 * properties of those EventTargets. This property is initialized to avoid
 * collisions with other JavaScript on the same page, similarly to the property
 * that goog.getUid uses.
 * @private {string}
 */
ink.Model.MODEL_INSTANCES_PROPERTY_ = 'ink_model_instances_' + Math.random();


/**
 * Gets the relevant model for the provided viewer. The viewer should be a
 * goog.ui.Component that has entered the document or a goog.events.EventTarget
 * that has already had its parentEventTarget set.
 *
 * Note: This currently assumes that the provided models are singletons per
 * EventTarget hierarchy tree. A more flexible design for deciding what level
 * to have models should be added here if usage demands it.
 *
 * @param {function(new:ink.Model, !goog.events.EventTarget)} modelCtor
 * @param {!goog.events.EventTarget} observer
 * @return {!ink.Model}
 */
ink.Model.get = function(modelCtor, observer) {
  var root = ink.util.getRootParentComponent(observer);
  var models = root[ink.Model.MODEL_INSTANCES_PROPERTY_];
  if (!models) {
    root[ink.Model.MODEL_INSTANCES_PROPERTY_] = models = {};
  }
  var key = goog.getUid(modelCtor);
  var oldInstance = models[key];
  if (oldInstance) {
    return oldInstance;
  }
  var newInstance = new modelCtor(root);
  models[key] = newInstance;
  return newInstance;
};
