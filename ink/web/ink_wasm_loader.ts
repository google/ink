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

import {trustedResourceUrl} from 'safevalues';
import {setScriptSrc} from 'safevalues/dom';

import {InkWasmModule} from './ink_wasm';

export * from './ink_wasm';

const LOADER_SCRIPT_URL =
    trustedResourceUrl`/base/third_party/ink/web/ink_wasm/ink_wasm_bin.js`;
const WASM_BINARY_PATH = '/base/third_party/ink/web/ink_wasm/ink_wasm_bin.wasm';

type CreateInkModuleFn = (
    options: Record<string, unknown>,
    ) => Promise<InkWasmModule>;

let cachedModulePromise: Promise<InkWasmModule>|null = null;

function loadLoaderScript(): Promise<void> {
  return new Promise((resolve, reject) => {
    const win = window as unknown as {createInkModule?: CreateInkModuleFn};
    if (typeof win.createInkModule === 'function') {
      resolve();
      return;
    }
    const script = document.createElement('script');
    setScriptSrc(script, LOADER_SCRIPT_URL);
    script.onload = () => {
      resolve();
    };
    script.onerror = (err) => {
      reject(new Error(`Failed to load WASM loader script: ${err}`));
    };
    document.head.appendChild(script);
  });
}

/**
 * Initializes the WebGPU device (when available) and loads the Ink WASM module.
 */
export function loadInkWasm(): Promise<InkWasmModule> {
  if (cachedModulePromise !== null) {
    return cachedModulePromise;
  }

  cachedModulePromise = (async () => {
    let device: GPUDevice|undefined;
    if (typeof navigator !== 'undefined' && navigator.gpu) {
      let adapter = await navigator.gpu.requestAdapter();
      if (!adapter) {
        adapter = await navigator.gpu.requestAdapter({
          forceFallbackAdapter: true,
        });
      }
      if (!adapter) {
        throw new Error('Failed to request WebGPU adapter.');
      }
      device = await adapter.requestDevice();
    }

    await loadLoaderScript();
    const win = window as unknown as {createInkModule?: CreateInkModuleFn};
    if (typeof win.createInkModule !== 'function') {
      throw new Error(
          'createInkModule was not defined by ink_wasm_bin.js loader.',
      );
    }

    const module = await win.createInkModule({
      preinitializedWebGPUDevice: device,
      locateFile: (path: string) =>
          path.endsWith('.wasm') ? WASM_BINARY_PATH : path,
    });
    if (device !== undefined) {
      module.preinitializedWebGPUDevice = device;
    }
    return module;
  })();

  return cachedModulePromise;
}
