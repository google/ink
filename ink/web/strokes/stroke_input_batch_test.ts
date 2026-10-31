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

describe('StrokeInputBatch Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('appends and retrieves StrokeInputs in order', () => {
    const batch = new ink.StrokeInputBatch();
    expect(batch.isEmpty()).toBeTrue();
    expect(batch.size()).toBe(0);

    const p0 = new ink.Point(10.0, 20.0);
    const t0 = ink.Duration32.seconds(0.0);
    const in0 = new ink.StrokeInput(p0, t0);

    const p1 = new ink.Point(30.0, 40.0);
    const t1 = ink.Duration32.seconds(0.1);
    const in1 = new ink.StrokeInput(p1, t1);

    batch.append(in0);
    batch.append(in1);

    expect(batch.isEmpty()).toBeFalse();
    expect(batch.size()).toBe(2);

    const out0 = batch.get(0);
    expect(out0.position.x).toBeCloseTo(10.0, 5);
    expect(out0.position.y).toBeCloseTo(20.0, 5);

    const out1 = batch.get(1);
    expect(out1.position.x).toBeCloseTo(30.0, 5);
    expect(out1.position.y).toBeCloseTo(40.0, 5);

    out1.delete();
    out0.delete();
    in1.delete();
    t1.delete();
    p1.delete();
    in0.delete();
    t0.delete();
    p0.delete();
    batch.delete();
  });
});
