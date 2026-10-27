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

describe('Brush Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('creates Brush with family, color, size, and epsilon', () => {
    const family = ink.StockBrushes.marker();
    const color = ink.Color.fromRgba(1.0, 0.0, 0.0, 1.0);
    const brush = ink.Brush.create(family, color, 10.0, 0.1);

    expect(brush.getSize()).toBeCloseTo(10.0, 5);
    expect(brush.getEpsilon()).toBeCloseTo(0.1, 5);
    expect(brush.coatCount()).toBe(1);

    const brushColor = brush.getColor();
    expect(brushColor.getRed()).toBeCloseTo(1.0, 5);
    expect(brushColor.getGreen()).toBeCloseTo(0.0, 5);
    expect(brushColor.getBlue()).toBeCloseTo(0.0, 5);
    expect(brushColor.getAlpha()).toBeCloseTo(1.0, 5);

    brushColor.delete();
    brush.delete();
    color.delete();
    family.delete();
  });
});
