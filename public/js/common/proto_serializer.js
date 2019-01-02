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

goog.provide('ink.ProtoSerializer');


goog.require('goog.crypt.base64');
goog.require('goog.proto2.ObjectSerializer');  // for debugging
goog.require('net.proto2.contrib.WireSerializer');



/**
 * A proto serializer / deserializer to and from base-64 encoded wire format.
 * @constructor
 * @struct
 */
ink.ProtoSerializer = function() {
  /** @private {!net.proto2.contrib.WireSerializer} */
  this.wireSerializer_ = new net.proto2.contrib.WireSerializer();
};


/**
 * @param {!goog.proto2.Message} e proto to serialize
 * @return {string} The serialized proto as a base 64 encoded string.
 */
ink.ProtoSerializer.prototype.serializeToBase64 = function(e) {
  var buf = this.wireSerializer_.serialize(e);
  return goog.crypt.base64.encodeByteArray(buf);
};


/**
 * Deserializes the given opaque serialized object to a jspb object.
 *
 * @param {string} item serialized object as base64 text
 * @param {!goog.proto2.Message} proto Proto to deserialize into
 * @return {!goog.proto2.Message}
 */
ink.ProtoSerializer.prototype.safeDeserialize = function(item, proto) {
  var buf = goog.crypt.base64.decodeStringToByteArray(item);
  this.wireSerializer_.deserializeTo(proto, new Uint8Array(buf));
  return proto;
};


/**
 * @param {!ink.proto.Element} p
 * @return {boolean} Whether the provided Element appears valid.
 * @private
 */
ink.ProtoSerializer.prototype.isValid_ = function(p) {
  if (p == null) {
    return false;
  }

  if (!p.hasStroke()) {
    return false;
  }

  return true;
};


/**
 * Returns a human-readable representation of a proto.
 *
 * @param {!goog.proto2.Message} p The proto to debug.
 * @return {string} A nice string to ponder.
 * @private
 */
ink.ProtoSerializer.prototype.debugProto_ = function(p) {
  var obj = new goog.proto2.ObjectSerializer(
      goog.proto2.ObjectSerializer.KeyOption.NAME).serialize(p);
  return JSON.stringify(obj, null, '  ');
};
