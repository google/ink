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

describe('Angle Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('converts between degrees and radians', () => {
    const angleDeg = ink.Angle.degrees(180.0);
    expect(angleDeg.valueInDegrees()).toBeCloseTo(180.0, 4);
    expect(angleDeg.valueInRadians()).toBeCloseTo(Math.PI, 4);
    angleDeg.delete();

    const angleRad = ink.Angle.radians(Math.PI / 2);
    expect(angleRad.valueInRadians()).toBeCloseTo(Math.PI / 2, 4);
    expect(angleRad.valueInDegrees()).toBeCloseTo(90.0, 4);
    angleRad.delete();
  });
});
