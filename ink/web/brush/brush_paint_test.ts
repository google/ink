/**
 * @license
 * Copyright 2026 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import 'jasmine';

import {InkWasmModule, loadInkWasm} from '../ink_wasm_loader';

describe('BrushPaint Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('constructs TilingTexture and BrushPaint and reads back fields', () => {
    const size = new ink.Vec(30.0, 30.0);
    const offset = new ink.Vec(0.25, 0.5);
    const rotation = ink.Angle.degrees(45.0);
    const layer = new ink.TilingTexture(
        'checkerboard-4x4',
        ink.TextureOrigin.STROKE_SPACE_ORIGIN,
        ink.TextureSizeUnit.STROKE_COORDINATES,
        ink.TextureWrap.REPEAT,
        ink.TextureWrap.MIRROR,
        size,
        offset,
        rotation,
        ink.BlendMode.MODULATE,
    );

    expect(layer.clientTextureId).toBe('checkerboard-4x4');
    expect(layer.origin).toBe(ink.TextureOrigin.STROKE_SPACE_ORIGIN);
    expect(layer.sizeUnit).toBe(ink.TextureSizeUnit.STROKE_COORDINATES);
    expect(layer.wrapX).toBe(ink.TextureWrap.REPEAT);
    expect(layer.wrapY).toBe(ink.TextureWrap.MIRROR);
    expect(layer.size.x).toBeCloseTo(30.0, 5);
    expect(layer.size.y).toBeCloseTo(30.0, 5);
    expect(layer.offset.x).toBeCloseTo(0.25, 5);
    expect(layer.offset.y).toBeCloseTo(0.5, 5);
    expect(layer.rotation.valueInDegrees()).toBeCloseTo(45.0, 4);
    expect(layer.blendMode).toBe(ink.BlendMode.MODULATE);

    const paint = ink.BrushPaint.withTilingTexture(
        layer,
        ink.SelfOverlap.DISCARD,
    );
    expect(paint.getTextureLayerCount()).toBe(1);
    expect(paint.selfOverlap).toBe(ink.SelfOverlap.DISCARD);

    const retrievedLayer = paint.getTilingTextureLayer(0);
    expect(retrievedLayer.clientTextureId).toBe('checkerboard-4x4');

    retrievedLayer.delete();
    paint.delete();
    layer.delete();
    rotation.delete();
    offset.delete();
    size.delete();
  });
});
