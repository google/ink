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

describe('PhysicalDistance Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('converts between centimeters, meters, and inches', () => {
    const dist = ink.PhysicalDistance.centimeters(2.54);
    expect(dist.isFinite()).toBeTrue();
    expect(dist.toCentimeters()).toBeCloseTo(2.54, 4);
    expect(dist.toMeters()).toBeCloseTo(0.0254, 5);
    expect(dist.toInches()).toBeCloseTo(1.0, 4);
    dist.delete();
  });
});
