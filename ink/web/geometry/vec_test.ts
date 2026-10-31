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

describe('Vec Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('constructs default Vec at (0, 0)', () => {
    const v = new ink.Vec();
    expect(v.x).toBe(0);
    expect(v.y).toBe(0);
    expect(v.magnitude()).toBe(0);
    v.delete();
  });

  it('constructs Vec with x and y and computes magnitude', () => {
    const v = new ink.Vec(3.0, 4.0);
    expect(v.x).toBeCloseTo(3.0, 5);
    expect(v.y).toBeCloseTo(4.0, 5);
    expect(v.magnitude()).toBeCloseTo(5.0, 5);
    v.delete();
  });
});
