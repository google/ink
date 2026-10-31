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

import {Brush, Color, InkWasmModule, InProgressStroke, loadInkWasm, Stroke, WebGpuTextureStore} from '../ink_wasm_loader';

interface RgbaColor {
  r: number;
  g: number;
  b: number;
  a: number;
}

describe('WebGpuRendererTests (Port of MetalRendererTests.swift)', () => {
  let ink: InkWasmModule;

  beforeAll(async () => {
    ink = await loadInkWasm();
  });

  function makeBrush(color: Color, size = 10, epsilon = 0.1): Brush {
    const family = ink.StockBrushes.marker();
    const brush = ink.Brush.create(family, color, size, epsilon);
    family.delete();
    return brush;
  }

  function makeCheckerboardBrush(): Brush {
    const size = new ink.Vec(10, 10);
    const offset = new ink.Vec(0, 0);
    const rotation = ink.Angle.radians(0);
    const layer = new ink.TilingTexture(
        'checkerboard',
        ink.TextureOrigin.STROKE_SPACE_ORIGIN,
        ink.TextureSizeUnit.STROKE_COORDINATES,
        ink.TextureWrap.REPEAT,
        ink.TextureWrap.REPEAT,
        size,
        offset,
        rotation,
        ink.BlendMode.MODULATE,
    );
    const paint = ink.BrushPaint.withTilingTexture(
        layer,
        ink.SelfOverlap.ACCUMULATE,
    );
    const coat = new ink.BrushCoat(paint);
    const family = ink.BrushFamily.fromCoat(coat);
    const color = ink.Color.fromRgba(0.75, 0.75, 0.75, 1.0);
    const brush = ink.Brush.create(family, color, 20, 0.1);

    color.delete();
    family.delete();
    coat.delete();
    paint.delete();
    layer.delete();
    rotation.delete();
    offset.delete();
    size.delete();
    return brush;
  }

  function makeCheckerboardTextureStore(): WebGpuTextureStore {
    const width = 8;
    const height = 8;
    const pixels = new Uint8Array(width * height * 4);
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const isWhite = x < width / 2 !== y < height / 2;
        const c = isWhite ? 255 : 0;
        const idx = (y * width + x) * 4;
        pixels[idx] = c;
        pixels[idx + 1] = c;
        pixels[idx + 2] = c;
        pixels[idx + 3] = 255;
      }
    }
    const store = new ink.WebGpuTextureStore();
    store.setRgba8Texture('checkerboard', width, height, pixels);
    return store;
  }

  function makeInProgressStroke(
      brush: Brush,
      points: Array<[number, number]> =
          [
            [20, 20],
            [80, 80],
          ],
      ): InProgressStroke {
    const realInputs = new ink.StrokeInputBatch();
    for (let i = 0; i < points.length; i++) {
      const [x, y] = points[i];
      const pt = new ink.Point(x, y);
      const time = ink.Duration32.seconds(i * 0.1);
      const input = new ink.StrokeInput(pt, time);
      realInputs.append(input);
      input.delete();
      time.delete();
      pt.delete();
    }
    const predictedInputs = new ink.StrokeInputBatch();
    const inProgress = new ink.InProgressStroke();
    inProgress.start(brush);
    inProgress.enqueueInputs(realInputs, predictedInputs);
    inProgress.finishInputs();
    const finalTime = ink.Duration32.seconds(points.length * 0.1);
    inProgress.updateShape(finalTime);

    finalTime.delete();
    predictedInputs.delete();
    realInputs.delete();
    return inProgress;
  }

  async function renderToPixelBuffer(
      drawables: Array<Stroke|InProgressStroke>,
      width = 100,
      height = 100,
      textureStore: WebGpuTextureStore|null = null,
      ): Promise<Uint8Array> {
    const renderer = ink.WebGpuRenderer.create(textureStore);
    const clearColor = ink.Color.fromRgba(1.0, 1.0, 1.0, 1.0);
    renderer.beginRenderPass(width, height, clearColor);

    for (const drawable of drawables) {
      if ('copyToStroke' in drawable) {
        renderer.drawInProgressStroke(drawable, width, height);
      } else {
        renderer.drawStroke(drawable, width, height);
      }
    }

    const gpuTexture = renderer.endRenderPass();
    const device = ink.preinitializedWebGPUDevice;
    const bytesPerRow = Math.ceil((width * 4) / 256) * 256;
    const readbackBuffer = device.createBuffer({
      size: bytesPerRow * height,
      usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ,
    });

    const encoder = device.createCommandEncoder();
    encoder.copyTextureToBuffer(
        {texture: gpuTexture},
        {buffer: readbackBuffer, bytesPerRow, rowsPerImage: height},
        [width, height],
    );
    device.queue.submit([encoder.finish()]);

    await readbackBuffer.mapAsync(GPUMapMode.READ);
    const mapped = new Uint8Array(readbackBuffer.getMappedRange());
    const pixels = new Uint8Array(width * height * 4);
    for (let y = 0; y < height; y++) {
      const srcOffset = y * bytesPerRow;
      const dstOffset = y * width * 4;
      pixels.set(mapped.subarray(srcOffset, srcOffset + width * 4), dstOffset);
    }
    readbackBuffer.unmap();
    readbackBuffer.destroy();
    gpuTexture.destroy();
    clearColor.delete();
    renderer.delete();
    return pixels;
  }

  function pixel(
      buffer: Uint8Array,
      x: number,
      y: number,
      width = 100,
      ): RgbaColor {
    const offset = (y * width + x) * 4;
    return {
      r: buffer[offset],
      g: buffer[offset + 1],
      b: buffer[offset + 2],
      a: buffer[offset + 3],
    };
  }

  it('drawsInProgressStroke', async () => {
    const red = ink.Color.fromRgba(1, 0, 0, 1);
    const brush = makeBrush(red);
    const inProgress = makeInProgressStroke(brush);
    const pixels = await renderToPixelBuffer([inProgress]);

    // The center of the stroke at (50, 50) should be red.
    expect(pixel(pixels, 50, 50)).toEqual({r: 255, g: 0, b: 0, a: 255});
    // A corner far from the stroke at (10, 90) should remain white.
    expect(pixel(pixels, 10, 90)).toEqual({r: 255, g: 255, b: 255, a: 255});

    inProgress.delete();
    brush.delete();
    red.delete();
  });

  it('drawsStroke', async () => {
    const red = ink.Color.fromRgba(1, 0, 0, 1);
    const brush = makeBrush(red);
    const inProgress = makeInProgressStroke(brush);
    const stroke = inProgress.copyToStroke();
    const pixels = await renderToPixelBuffer([stroke]);

    // The center of the stroke at (50, 50) should be red.
    expect(pixel(pixels, 50, 50)).toEqual({r: 255, g: 0, b: 0, a: 255});
    // A corner far from the stroke at (10, 90) should remain white.
    expect(pixel(pixels, 10, 90)).toEqual({r: 255, g: 255, b: 255, a: 255});

    stroke.delete();
    inProgress.delete();
    brush.delete();
    red.delete();
  });

  it('drawsInProgressStrokeWithCheckerboardTexture', async () => {
    const brush = makeCheckerboardBrush();
    const store = makeCheckerboardTextureStore();
    const inProgress = makeInProgressStroke(brush);
    const pixels = await renderToPixelBuffer([inProgress], 100, 100, store);

    let greyPixelCount = 0;
    let blackPixelCount = 0;
    for (let y = 30; y < 70; y++) {
      for (let x = 30; x < 70; x++) {
        if (Math.abs(x - y) <= 6) {
          const p = pixel(pixels, x, y);
          if (p.r >= 80 && p.r <= 200 && p.g >= 80 && p.g <= 200 && p.b >= 80 &&
              p.b <= 200) {
            greyPixelCount++;
          } else if (p.r < 50 && p.g < 50 && p.b < 50) {
            blackPixelCount++;
          }
        }
      }
    }

    expect(greyPixelCount).toBeGreaterThan(0);
    expect(blackPixelCount).toBeGreaterThan(0);
    expect(pixel(pixels, 10, 90)).toEqual({r: 255, g: 255, b: 255, a: 255});

    inProgress.delete();
    store.delete();
    brush.delete();
  });

  it('drawsStrokeWithCheckerboardTexture', async () => {
    const brush = makeCheckerboardBrush();
    const store = makeCheckerboardTextureStore();
    const inProgress = makeInProgressStroke(brush);
    const stroke = inProgress.copyToStroke();
    const pixels = await renderToPixelBuffer([stroke], 100, 100, store);

    let greyPixelCount = 0;
    let blackPixelCount = 0;
    for (let y = 30; y < 70; y++) {
      for (let x = 30; x < 70; x++) {
        if (Math.abs(x - y) <= 6) {
          const p = pixel(pixels, x, y);
          if (p.r >= 80 && p.r <= 200 && p.g >= 80 && p.g <= 200 && p.b >= 80 &&
              p.b <= 200) {
            greyPixelCount++;
          } else if (p.r < 50 && p.g < 50 && p.b < 50) {
            blackPixelCount++;
          }
        }
      }
    }

    expect(greyPixelCount).toBeGreaterThan(0);
    expect(blackPixelCount).toBeGreaterThan(0);
    expect(pixel(pixels, 10, 90)).toEqual({r: 255, g: 255, b: 255, a: 255});

    stroke.delete();
    inProgress.delete();
    store.delete();
    brush.delete();
  });

  it('drawsMixedSequenceOfInProgressAndFinishedStrokes', async () => {
    const red = ink.Color.fromRgba(1, 0, 0, 1);
    const green = ink.Color.fromRgba(0, 1, 0, 1);
    const blue = ink.Color.fromRgba(0, 0, 1, 1);
    const yellow = ink.Color.fromRgba(1, 1, 0, 1);

    const redBrush = makeBrush(red, 8);
    const greenBrush = makeBrush(green, 8);
    const blueBrush = makeBrush(blue, 8);
    const yellowBrush = makeBrush(yellow, 8);

    const stroke1InProgress = makeInProgressStroke(redBrush, [
      [15, 20],
      [85, 20],
    ]);
    const stroke1 = stroke1InProgress.copyToStroke();

    const inProgress1 = makeInProgressStroke(greenBrush, [
      [15, 40],
      [85, 40],
    ]);

    const stroke2InProgress = makeInProgressStroke(blueBrush, [
      [15, 60],
      [85, 60],
    ]);
    const stroke2 = stroke2InProgress.copyToStroke();

    const inProgress2 = makeInProgressStroke(yellowBrush, [
      [15, 80],
      [85, 80],
    ]);

    const pixels = await renderToPixelBuffer([
      stroke1,
      inProgress1,
      stroke2,
      inProgress2,
    ]);

    expect(pixel(pixels, 50, 20)).toEqual({r: 255, g: 0, b: 0, a: 255});
    expect(pixel(pixels, 50, 40)).toEqual({r: 0, g: 255, b: 0, a: 255});
    expect(pixel(pixels, 50, 60)).toEqual({r: 0, g: 0, b: 255, a: 255});
    expect(pixel(pixels, 50, 80)).toEqual({r: 255, g: 255, b: 0, a: 255});
    expect(pixel(pixels, 50, 5)).toEqual({r: 255, g: 255, b: 255, a: 255});

    inProgress2.delete();
    stroke2.delete();
    stroke2InProgress.delete();
    inProgress1.delete();
    stroke1.delete();
    stroke1InProgress.delete();
    yellowBrush.delete();
    blueBrush.delete();
    greenBrush.delete();
    redBrush.delete();
    yellow.delete();
    blue.delete();
    green.delete();
    red.delete();
  });
});
