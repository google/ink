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
 * @fileoverview Unit tests for ink.util functions.
 */
goog.require('goog.testing.proto2');
goog.require('ink.proto.AffineTransform');
goog.require('ink.proto.ElementBundle');
goog.require('ink.proto.ElementState');
goog.require('ink.proto.ImageInfo');
goog.require('ink.proto.MutationPacket');
goog.require('ink.proto.PageProperties');
goog.require('ink.proto.Rect');
goog.require('ink.proto.ReplaceAction');
goog.require('ink.proto.Snapshot');
goog.require('ink.proto.StorageAction');
goog.require('ink.util');

// Proto manipulation helpers

/**
 * Make element with UUID
 * @param {string} uuid
 * @return {!ink.proto.ElementBundle}
 */
function makeElement(uuid) {
  const element = new ink.proto.ElementBundle();
  element.setUuid(uuid);
  return element;
}

/**
 * Make action of type and add or set UUID
 * @param {string} type String name of action proto to create
 * @param {string} uuid
 * @return {!ink.proto.StorageAction}
 */
function makeAction(type, uuid) {
  const action = new ink.proto[type]();
  const verb = type == 'AddAction' ? 'set' : 'add';
  action[verb + 'Uuid'](uuid);
  const storageAction = new ink.proto.StorageAction();
  storageAction['set' + type](action);
  return storageAction;
}

/**
 * Make a replace storage action
 * @param {string} addedUuid
 * @param {string} addedWasBelowUuid
 * @param {string} removedUuid
 * @param {string} removedWasBelowUuid
 * @return {!ink.proto.StorageAction}
 */
function makeReplaceAction(
    addedUuid, addedWasBelowUuid, removedUuid, removedWasBelowUuid) {
  const replaceAction = new ink.proto.ReplaceAction();
  replaceAction.addAddedUuid(addedUuid);
  replaceAction.addAddedWasBelowUuid(addedWasBelowUuid);
  replaceAction.addRemovedUuid(removedUuid);
  replaceAction.addRemovedWasBelowUuid(removedWasBelowUuid);
  const storageAction = new ink.proto.StorageAction();
  storageAction.setReplaceAction(replaceAction);
  return storageAction;
}

/**
 * Unit tests for mutation packet functions
 */
function MutationPacketsTest() {}
registerTestSuite(MutationPacketsTest);

addTest(MutationPacketsTest, function hasPendingMutations() {
  const snapshot = new ink.proto.Snapshot();
  snapshot.addElement(makeElement('a'));
  snapshot.addElement(makeElement('b'));
  snapshot.addElement(makeElement('c'));
  expectFalse(ink.util.snapshotHasPendingMutationPacket(snapshot));

  snapshot.addUndoAction(makeAction('RemoveAction', 'd'));
  expectTrue(ink.util.snapshotHasPendingMutationPacket(snapshot));
});

addTest(MutationPacketsTest, function rejectsBogusSnapshot() {
  const snapshot = new ink.proto.Snapshot();
  snapshot.addElement(makeElement('a'));
  snapshot.addElement(makeElement('c'));
  snapshot.addUndoAction(makeAction('AddAction', 'b'));  // No element!
  snapshot.addUndoAction(makeAction('AddAction', 'c'));
  expectThat(ink.util.extractMutationPacket(snapshot), isNull);
});

addTest(MutationPacketsTest, function knowsThatElementsAreRemoved() {
  const snapshot = new ink.proto.Snapshot();
  snapshot.addElement(makeElement('a'));
  snapshot.addElement(makeElement('c'));
  snapshot.addUndoAction(makeAction('AddAction', 'b'));  // No element!
  snapshot.addUndoAction(makeAction('AddAction', 'c'));
  snapshot.addDeadElement(makeElement('b'));
  snapshot.addUndoAction(makeAction('RemoveAction', 'b'));  // But it was removed
  expectThat(ink.util.extractMutationPacket(snapshot), not(isNull));
});

addTest(MutationPacketsTest, function addsHaveElementPool() {
  const snapshot = new ink.proto.Snapshot();
  snapshot.addElement(makeElement('a'));
  snapshot.addElement(makeElement('b'));
  snapshot.addElement(makeElement('c'));
  snapshot.addUndoAction(makeAction('AddAction', 'b'));
  snapshot.addUndoAction(makeAction('AddAction', 'c'));
  snapshot.addDeadElement(makeElement('d'));
  snapshot.addRedoAction(makeAction('AddAction', 'd'));
  const packet = ink.util.extractMutationPacket(snapshot);
  expectThat(packet, not(isNull));

  const expected = new ink.proto.MutationPacket();
  expected.addElement(makeElement('b'));
  expected.addElement(makeElement('c'));
  expected.addMutation(makeAction('AddAction', 'b'));
  expected.addMutation(makeAction('AddAction', 'c'));
  goog.testing.proto2.assertEquals(expected, packet);
});

addTest(MutationPacketsTest, function addsMultiple() {
  const snapshot = new ink.proto.Snapshot();
  snapshot.addElement(makeElement('a'));
  snapshot.addElement(makeElement('b'));
  snapshot.addElement(makeElement('c'));
  snapshot.addUndoAction(makeAction('AddMultipleAction', 'b'));
  snapshot.addUndoAction(makeAction('AddMultipleAction', 'c'));
  snapshot.addDeadElement(makeElement('d'));
  snapshot.addRedoAction(makeAction('AddMultipleAction', 'd'));
  const packet = ink.util.extractMutationPacket(snapshot);
  expectThat(packet, not(isNull));

  const expected = new ink.proto.MutationPacket();
  expected.addElement(makeElement('b'));
  expected.addElement(makeElement('c'));
  expected.addMutation(makeAction('AddMultipleAction', 'b'));
  expected.addMutation(makeAction('AddMultipleAction', 'c'));
  goog.testing.proto2.assertEquals(expected, packet);
});

addTest(MutationPacketsTest, function replace() {
  const snapshot = new ink.proto.Snapshot();
  snapshot.addElement(makeElement('a'));
  snapshot.addDeadElement(makeElement('b'));
  snapshot.addElement(makeElement('c'));
  snapshot.addElement(makeElement('d'));
  snapshot.addUndoAction(makeReplaceAction('c', 'd', 'b', 'd'));
  const packet = ink.util.extractMutationPacket(snapshot);
  expectThat(packet, not(isNull));

  const expected = new ink.proto.MutationPacket();
  expected.addElement(makeElement('c'));
  expected.addMutation(makeReplaceAction('c', 'd', 'b', 'd'));
  goog.testing.proto2.assertEquals(expected, packet);
});

addTest(MutationPacketsTest, function otherActions() {
  const snapshot = new ink.proto.Snapshot();
  snapshot.addElement(makeElement('a'));
  snapshot.addElement(makeElement('b'));
  snapshot.addElement(makeElement('c'));
  snapshot.addUndoAction(makeAction('RemoveAction', 'd'));
  const setTransform = makeAction('SetTransformAction', 'b');
  const toTransform = new ink.proto.AffineTransform();
  toTransform.setTx(12);
  setTransform.getSetTransformAction().addToTransform(toTransform);
  snapshot.addUndoAction(setTransform);
  const packet = ink.util.extractMutationPacket(snapshot);
  expectThat(packet, not(isNull));

  const expected = new ink.proto.MutationPacket();
  expected.addMutation(makeAction('RemoveAction', 'd'));
  expected.addMutation(setTransform);
  goog.testing.proto2.assertEquals(expected, packet);
});

addTest(MutationPacketsTest, function clearPending() {
  const snapshot = new ink.proto.Snapshot();
  snapshot.addElement(makeElement('a'));
  snapshot.addElement(makeElement('b'));
  snapshot.addElement(makeElement('c'));
  snapshot.addElementStateIndex(ink.proto.ElementState.ALIVE);
  snapshot.addElementStateIndex(ink.proto.ElementState.ALIVE);
  snapshot.addElementStateIndex(ink.proto.ElementState.ALIVE);
  snapshot.addUndoAction(makeAction('AddAction', 'b'));
  snapshot.addUndoAction(makeAction('AddAction', 'c'));
  snapshot.addDeadElement(makeElement('d'));
  snapshot.addElementStateIndex(ink.proto.ElementState.DEAD);
  snapshot.addRedoAction(makeAction('AddAction', 'd'));
  snapshot.setFingerprint('0xabbadabbad00');
  const bounds = new ink.proto.Rect();
  bounds.setXlow(13);
  const pageProperties = new ink.proto.PageProperties();
  pageProperties.setBounds(bounds);
  snapshot.setPageProperties(pageProperties);
  ink.util.clearPendingMutationPacket(snapshot);

  const expected = new ink.proto.Snapshot();
  expected.addElement(makeElement('a'));
  expected.addElement(makeElement('b'));
  expected.addElement(makeElement('c'));
  expected.addElementStateIndex(ink.proto.ElementState.ALIVE);
  expected.addElementStateIndex(ink.proto.ElementState.ALIVE);
  expected.addElementStateIndex(ink.proto.ElementState.ALIVE);
  expected.setFingerprint('0xabbadabbad00');
  expected.setPageProperties(pageProperties);
  goog.testing.proto2.assertEquals(expected, snapshot);
});

addTest(MutationPacketsTest, function copiesPageProperties() {
  const snapshot = new ink.proto.Snapshot();
  const bounds = new ink.proto.Rect();
  bounds.setXlow(13);
  const pageProperties = new ink.proto.PageProperties();
  pageProperties.setBounds(bounds);
  snapshot.setPageProperties(pageProperties);

  const packet = ink.util.extractMutationPacket(snapshot);
  expectThat(packet, not(isNull));
  expectTrue(packet.hasPageProperties());
  expectEq(13, packet.getPageProperties().getBounds().getXlow());
});


/** Unit tests for building/parsing "host:" URIs. */
function HostUriTest() {}
registerTestSuite(HostUriTest);

addTest(HostUriTest, function buildHostUri() {
  expectEq(
      'host:STICKER:static/img/smile.png',
      ink.util.buildHostUri(
          ink.proto.ImageInfo.AssetType.STICKER, 'static/img/smile.png'));
  expectEq(
      'host:TILED_TEXTURE:glitter-pen',
      ink.util.buildHostUri(
          ink.proto.ImageInfo.AssetType.TILED_TEXTURE, 'glitter-pen'));
  // Colons are allowed.
  expectEq(
      'host:DEFAULT:foo:bar:baz',
      ink.util.buildHostUri(
          ink.proto.ImageInfo.AssetType.DEFAULT, 'foo:bar:baz'));
  // Empty asset ID is allowed.
  expectEq(
      'host:BORDER:',
      ink.util.buildHostUri(ink.proto.ImageInfo.AssetType.BORDER, ''));
});

addTest(HostUriTest, function parseHostUri() {
  let hostUri = ink.util.parseHostUri('host:STICKER:static/img/smile.png');
  expectThat(hostUri, not(isNull));
  expectEq(ink.proto.ImageInfo.AssetType.STICKER, hostUri.assetType);
  expectEq('static/img/smile.png', hostUri.assetId);

  hostUri = ink.util.parseHostUri('host:TILED_TEXTURE:glitter-pen');
  expectThat(hostUri, not(isNull));
  expectEq(ink.proto.ImageInfo.AssetType.TILED_TEXTURE, hostUri.assetType);
  expectEq('glitter-pen', hostUri.assetId);

  // Colons are allowed.
  hostUri = ink.util.parseHostUri('host:DEFAULT:foo:bar:baz');
  expectThat(hostUri, not(isNull));
  expectEq(ink.proto.ImageInfo.AssetType.DEFAULT, hostUri.assetType);
  expectEq('foo:bar:baz', hostUri.assetId);

  // Empty asset ID is allowed.
  hostUri = ink.util.parseHostUri('host:BORDER:');
  expectThat(hostUri, not(isNull));
  expectEq(ink.proto.ImageInfo.AssetType.BORDER, hostUri.assetType);
  expectEq('', hostUri.assetId);

  // Non-"host" scheme is not allowed.
  expectThat(ink.util.parseHostUri('sketchology:STICKER:image'), isNull);
  // Invalid asset type is not allowed.
  expectThat(ink.util.parseHostUri('host:AWESOME:image'), isNull);
  // Missing asset type is not allowed.
  expectThat(ink.util.parseHostUri('host:image'), isNull);
});

/** Unit tests for converting AssetTypes to strings and back. */
function AssetTypeNameTest() {}
registerTestSuite(AssetTypeNameTest);

addTest(AssetTypeNameTest, function assetTypeName() {
  expectEq(
      'DEFAULT', ink.util.assetTypeName(ink.proto.ImageInfo.AssetType.DEFAULT));
  expectEq(
      'BORDER', ink.util.assetTypeName(ink.proto.ImageInfo.AssetType.BORDER));
  expectEq(
      'STICKER', ink.util.assetTypeName(ink.proto.ImageInfo.AssetType.STICKER));
  expectEq('GRID', ink.util.assetTypeName(ink.proto.ImageInfo.AssetType.GRID));
  expectEq(
      'TILED_TEXTURE',
      ink.util.assetTypeName(ink.proto.ImageInfo.AssetType.TILED_TEXTURE));
  expectEq(
      'UNKNOWN(foo)',
      ink.util.assetTypeName(
          /** @type {!ink.proto.ImageInfo.AssetType} */ ('foo')));
});

addTest(AssetTypeNameTest, function parseAssetType() {
  expectEq(
      ink.proto.ImageInfo.AssetType.DEFAULT,
      ink.util.parseAssetType('DEFAULT'));
  expectEq(
      ink.proto.ImageInfo.AssetType.BORDER, ink.util.parseAssetType('BORDER'));
  expectEq(
      ink.proto.ImageInfo.AssetType.STICKER,
      ink.util.parseAssetType('STICKER'));
  expectEq(ink.proto.ImageInfo.AssetType.GRID, ink.util.parseAssetType('GRID'));
  expectEq(
      ink.proto.ImageInfo.AssetType.TILED_TEXTURE,
      ink.util.parseAssetType('TILED_TEXTURE'));
  expectThat(ink.util.parseAssetType('AWESOME'), isNull);
});
