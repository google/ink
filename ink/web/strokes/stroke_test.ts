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

describe('Stroke Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('constructs Stroke from Brush and StrokeInputBatch', () => {
    const family = ink.StockBrushes.marker();
    const color = ink.Color.fromRgba(1.0, 0.0, 0.0, 1.0);
    const brush = ink.Brush.create(family, color, 10.0, 0.1);

    const batch = new ink.StrokeInputBatch();
    const p0 = new ink.Point(20.0, 20.0);
    const t0 = ink.Duration32.seconds(0.0);
    const in0 = new ink.StrokeInput(p0, t0);
    const p1 = new ink.Point(80.0, 80.0);
    const t1 = ink.Duration32.seconds(0.1);
    const in1 = new ink.StrokeInput(p1, t1);
    batch.append(in0);
    batch.append(in1);

    const stroke = new ink.Stroke(brush, batch);
    expect(stroke.renderGroupCount()).toBe(1);

    const inputs = stroke.getInputs();
    expect(inputs.size()).toBe(2);

    inputs.delete();
    stroke.delete();
    in1.delete();
    t1.delete();
    p1.delete();
    in0.delete();
    t0.delete();
    p0.delete();
    batch.delete();
    brush.delete();
    color.delete();
    family.delete();
  });
});
