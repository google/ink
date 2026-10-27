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

describe('InProgressStroke Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('starts, enqueues inputs, updates shape, and copies to Stroke', () => {
    const family = ink.StockBrushes.marker();
    const color = ink.Color.fromRgba(1.0, 0.0, 0.0, 1.0);
    const brush = ink.Brush.create(family, color, 10.0, 0.1);

    const realInputs = new ink.StrokeInputBatch();
    const p0 = new ink.Point(20.0, 20.0);
    const t0 = ink.Duration32.seconds(0.0);
    const in0 = new ink.StrokeInput(p0, t0);
    const p1 = new ink.Point(80.0, 80.0);
    const t1 = ink.Duration32.seconds(0.1);
    const in1 = new ink.StrokeInput(p1, t1);
    realInputs.append(in0);
    realInputs.append(in1);

    const predictedInputs = new ink.StrokeInputBatch();

    const inProgress = new ink.InProgressStroke();
    inProgress.start(brush);
    inProgress.enqueueInputs(realInputs, predictedInputs);
    inProgress.finishInputs();
    inProgress.updateShape(t1);

    expect(inProgress.brushCoatCount()).toBe(1);
    expect(inProgress.inputCount()).toBe(2);
    expect(inProgress.meshVertexCount(0)).toBeGreaterThan(0);
    expect(inProgress.meshTriangleCount(0)).toBeGreaterThan(0);

    const finishedStroke = inProgress.copyToStroke();
    expect(finishedStroke.renderGroupCount()).toBe(1);

    finishedStroke.delete();
    inProgress.delete();
    predictedInputs.delete();
    in1.delete();
    t1.delete();
    p1.delete();
    in0.delete();
    t0.delete();
    p0.delete();
    realInputs.delete();
    brush.delete();
    color.delete();
    family.delete();
  });
});
