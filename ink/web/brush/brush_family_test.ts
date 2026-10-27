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

describe('BrushFamily Embind Wrapper', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  it('creates BrushFamily from single coat and multiple coats', () => {
    const paint = new ink.BrushPaint();
    const coat = new ink.BrushCoat(paint);

    const familySingle = ink.BrushFamily.fromCoat(coat);
    expect(familySingle.getCoatCount()).toBe(1);

    const familyMulti = ink.BrushFamily.fromCoats([coat, coat]);
    expect(familyMulti.getCoatCount()).toBe(2);

    familyMulti.delete();
    familySingle.delete();
    coat.delete();
    paint.delete();
  });
});
