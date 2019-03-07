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
 * @fileoverview Unit tests for ink.util.getImageBytes().
 */
goog.module('ink.UtilImageTest');
goog.setTestOnly();

const SafeUrl = goog.require('goog.html.SafeUrl');
const Size = goog.require('goog.math.Size');
const inkUtil = goog.require('ink.util');



/**
 * @const {string}
 */
const RED_DOT_PNG_BASE64 = 'iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAA' +
    'HElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg==';


/**
 * @return {!Uint8Array}
 */
function redDotPngBytes() {
  const data = atob(RED_DOT_PNG_BASE64);
  const array = new Uint8Array(data.length);
  for (let i = 0; i < data.length; i++) {
    array[i] = data.charCodeAt(i);
  }
  return array;
}


describe('getImageBytes', () => {
  it('should work with a data URL', async () => {
    const url = 'data:image/png;base64,' + RED_DOT_PNG_BASE64;
    const {size} = await inkUtil.getImageBytes(url);
    expect(size.width).toBe(5);
    expect(size.height).toBe(5);
  });

  it('should work with an object URL', async () => {
    const blob = new Blob([redDotPngBytes()], {type: 'image/png'});
    const url = SafeUrl.unwrap(SafeUrl.fromBlob(blob));
    const {size} = await inkUtil.getImageBytes(url);
    URL.revokeObjectURL(url);
    expect(size.width).toBe(5);
    expect(size.height).toBe(5);
  });

  it('should work with a Blob', async () => {
    const blob = new Blob([redDotPngBytes()], {type: 'image/png'});
    const {size} = await inkUtil.getImageBytes(blob);
    expect(size.width).toBe(5);
    expect(size.height).toBe(5);
  });

  it('should work with decoded image data', async () => {
    const blob = new Blob([redDotPngBytes()], {type: 'image/png'});
    const decoded = await inkUtil.getImageBytes(blob);
    const {size} = await inkUtil.getImageBytes(decoded);
    expect(size.width).toBe(5);
    expect(size.height).toBe(5);
  });

  it('should fail with an invalid URL', async () => {
    const url = 'wharrgarbl';
    let errorMessage = '';
    try {
      await inkUtil.getImageBytes(url);
    } catch (error) {
      errorMessage = error.message;
    }
    expect(errorMessage).toBe('getImageBytes failed on wharrgarbl');
  });

  it('should fail with an invalid Blob', async () => {
    const blob = new Blob(['wharrgarbl'], {type: 'image/png'});
    let errorMessage = '';
    try {
      await inkUtil.getImageBytes(blob);
    } catch (error) {
      errorMessage = error.message;
    }
    expect(errorMessage).toContain('getImageBytes failed on blob:');
  });

  it('should fail with invalid decoded image data', async () => {
    const decoded = {data: new Uint8ClampedArray(13), size: new Size(4, 3)};
    let errorMessage = '';
    try {
      await inkUtil.getImageBytes(decoded);
    } catch (error) {
      errorMessage = error.message;
    }
    expect(errorMessage)
        .toBe('Image RGBA data length is 13, but expected 48 for a 4x3 image');
  });
});
