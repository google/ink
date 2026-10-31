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

describe('Color Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('constructs Color from RGBA floats and reads back channels', () => {
    const color = ink.Color.fromRgba(0.25, 0.5, 0.75, 0.8);
    expect(color.getRed()).toBeCloseTo(0.25, 4);
    expect(color.getGreen()).toBeCloseTo(0.5, 4);
    expect(color.getBlue()).toBeCloseTo(0.75, 4);
    expect(color.getAlpha()).toBeCloseTo(0.8, 4);
    color.delete();
  });

  it('provides named color factories', () => {
    const red = ink.Color.red();
    expect(red.getRed()).toBeCloseTo(1.0, 5);
    expect(red.getGreen()).toBeCloseTo(0.0, 5);
    expect(red.getBlue()).toBeCloseTo(0.0, 5);
    expect(red.getAlpha()).toBeCloseTo(1.0, 5);
    red.delete();
  });
});
