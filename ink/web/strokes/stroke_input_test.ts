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

describe('StrokeInput Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('constructs StrokeInput with position and elapsedTime', () => {
    const pos = new ink.Point(10.0, 20.0);
    const time = ink.Duration32.seconds(0.5);
    const input = new ink.StrokeInput(pos, time);

    expect(input.position.x).toBeCloseTo(10.0, 5);
    expect(input.position.y).toBeCloseTo(20.0, 5);
    expect(input.elapsedTime.toSeconds()).toBeCloseTo(0.5, 4);
    expect(input.toolType).toBe(ink.ToolType.UNKNOWN);
    expect(input.hasPressure()).toBeFalse();

    input.delete();
    time.delete();
    pos.delete();
  });
});
