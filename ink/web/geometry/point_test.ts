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

describe('Point Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('constructs default Point at (0, 0)', () => {
    const p = new ink.Point();
    expect(p.x).toBe(0);
    expect(p.y).toBe(0);
    p.delete();
  });

  it('constructs Point with x and y and mutates coordinates', () => {
    const p = new ink.Point(3.5, -7.25);
    expect(p.x).toBeCloseTo(3.5, 5);
    expect(p.y).toBeCloseTo(-7.25, 5);
    p.x = 12.0;
    p.y = 24.0;
    expect(p.x).toBeCloseTo(12.0, 5);
    expect(p.y).toBeCloseTo(24.0, 5);
    p.delete();
  });
});
