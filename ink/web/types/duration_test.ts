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

describe('Duration32 Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('constructs Duration32 from seconds and milliseconds', () => {
    const dSec = ink.Duration32.seconds(1.5);
    expect(dSec.isFinite()).toBeTrue();
    expect(dSec.toSeconds()).toBeCloseTo(1.5, 4);
    expect(dSec.toMillis()).toBeCloseTo(1500.0, 2);
    dSec.delete();

    const dMs = ink.Duration32.millis(250.0);
    expect(dMs.isFinite()).toBeTrue();
    expect(dMs.toMillis()).toBeCloseTo(250.0, 3);
    expect(dMs.toSeconds()).toBeCloseTo(0.25, 4);
    dMs.delete();

    const dInf = ink.Duration32.infinite();
    expect(dInf.isFinite()).toBeFalse();
    dInf.delete();
  });
});
