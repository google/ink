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

/** Base interface for heap-allocated Embind C++ objects. */
export interface EmbindObject {
  delete(): void;
}

/** Embind wrapper for `ink::Color`. */
export interface Color extends EmbindObject {
  getRed(): number;
  getGreen(): number;
  getBlue(): number;
  getAlpha(): number;
}

/** Constructor and static factory methods for `Color`. */
export interface ColorConstructor {
  new(): Color;
  fromRgba(r: number, g: number, b: number, a: number): Color;
  black(): Color;
  white(): Color;
  red(): Color;
  green(): Color;
  blue(): Color;
  transparent(): Color;
}

/** Embind wrapper for `ink::Point`. */
export interface Point extends EmbindObject {
  x: number;
  y: number;
}

/** Constructor for `Point`. */
export interface PointConstructor {
  new(): Point;
  new(x: number, y: number): Point;
}

/** Embind wrapper for `ink::Vec`. */
export interface Vec extends EmbindObject {
  x: number;
  y: number;
  magnitude(): number;
}

/** Constructor for `Vec`. */
export interface VecConstructor {
  new(): Vec;
  new(x: number, y: number): Vec;
}

/** Embind wrapper for `ink::Angle`. */
export interface Angle extends EmbindObject {
  valueInRadians(): number;
  valueInDegrees(): number;
  normalized(): Angle;
  normalizedAboutZero(): Angle;
}

/** Constructor and static factory methods for `Angle`. */
export interface AngleConstructor {
  new(): Angle;
  radians(radians: number): Angle;
  degrees(degrees: number): Angle;
}

/** Embind wrapper for `ink::Duration32`. */
export interface Duration32 extends EmbindObject {
  isFinite(): boolean;
  toSeconds(): number;
  toMillis(): number;
}

/** Constructor and static factory methods for `Duration32`. */
export interface Duration32Constructor {
  new(): Duration32;
  zero(): Duration32;
  infinite(): Duration32;
  seconds(seconds: number): Duration32;
  millis(millis: number): Duration32;
}

/** Embind wrapper for `ink::PhysicalDistance`. */
export interface PhysicalDistance extends EmbindObject {
  isFinite(): boolean;
  toCentimeters(): number;
  toMeters(): number;
  toInches(): number;
}

/** Constructor and static factory methods for `PhysicalDistance`. */
export interface PhysicalDistanceConstructor {
  new(): PhysicalDistance;
  zero(): PhysicalDistance;
  centimeters(cm: number): PhysicalDistance;
  meters(m: number): PhysicalDistance;
  inches(inches: number): PhysicalDistance;
}

/** Marker interface for Embind enum values. */
export interface EmbindEnumValue {}

/** Embind enum for `ink::BrushPaint::TextureOrigin`. */
export interface TextureOriginEnum {
  readonly STROKE_SPACE_ORIGIN: EmbindEnumValue;
  readonly FIRST_STROKE_INPUT: EmbindEnumValue;
  readonly LAST_STROKE_INPUT: EmbindEnumValue;
}

/** Embind enum for `ink::BrushPaint::TextureSizeUnit`. */
export interface TextureSizeUnitEnum {
  readonly BRUSH_SIZE: EmbindEnumValue;
  readonly STROKE_COORDINATES: EmbindEnumValue;
}

/** Embind enum for `ink::BrushPaint::TextureWrap`. */
export interface TextureWrapEnum {
  readonly REPEAT: EmbindEnumValue;
  readonly MIRROR: EmbindEnumValue;
  readonly CLAMP: EmbindEnumValue;
}

/** Embind enum for `ink::BrushPaint::BlendMode`. */
export interface BlendModeEnum {
  readonly MODULATE: EmbindEnumValue;
  readonly DST_IN: EmbindEnumValue;
  readonly DST_OUT: EmbindEnumValue;
  readonly SRC_ATOP: EmbindEnumValue;
  readonly SRC_IN: EmbindEnumValue;
  readonly SRC_OVER: EmbindEnumValue;
  readonly DST_OVER: EmbindEnumValue;
  readonly SRC: EmbindEnumValue;
  readonly DST: EmbindEnumValue;
  readonly SRC_OUT: EmbindEnumValue;
  readonly DST_ATOP: EmbindEnumValue;
  readonly XOR: EmbindEnumValue;
}

/** Embind enum for `ink::BrushPaint::SelfOverlap`. */
export interface SelfOverlapEnum {
  readonly ANY: EmbindEnumValue;
  readonly ACCUMULATE: EmbindEnumValue;
  readonly DISCARD: EmbindEnumValue;
}

/** Embind wrapper for `ink::BrushPaint::TilingTexture`. */
export interface TilingTexture extends EmbindObject {
  clientTextureId: string;
  origin: EmbindEnumValue;
  sizeUnit: EmbindEnumValue;
  wrapX: EmbindEnumValue;
  wrapY: EmbindEnumValue;
  size: Vec;
  offset: Vec;
  rotation: Angle;
  blendMode: EmbindEnumValue;
}

/** Constructor for `TilingTexture`. */
export interface TilingTextureConstructor {
  new(): TilingTexture;
  new(
      clientTextureId: string,
      origin: EmbindEnumValue,
      sizeUnit: EmbindEnumValue,
      wrapX: EmbindEnumValue,
      wrapY: EmbindEnumValue,
      size: Vec,
      offset: Vec,
      rotation: Angle,
      blendMode: EmbindEnumValue,
      ): TilingTexture;
}

/** Embind wrapper for `ink::BrushPaint`. */
export interface BrushPaint extends EmbindObject {
  selfOverlap: EmbindEnumValue;
  addTilingTextureLayer(layer: TilingTexture): void;
  getTextureLayerCount(): number;
  getTilingTextureLayer(index: number): TilingTexture;
}

/** Constructor and static factory methods for `BrushPaint`. */
export interface BrushPaintConstructor {
  new(): BrushPaint;
  withTilingTexture(
      layer: TilingTexture,
      selfOverlap: EmbindEnumValue,
      ): BrushPaint;
}

/** Embind wrapper for `ink::BrushCoat`. */
export interface BrushCoat extends EmbindObject {
  getPaintPreferenceCount(): number;
  getPaintPreference(index: number): BrushPaint;
  addPaintPreference(paint: BrushPaint): void;
}

/** Constructor for `BrushCoat`. */
export interface BrushCoatConstructor {
  new(): BrushCoat;
  new(paint: BrushPaint): BrushCoat;
}

/** Embind wrapper for `ink::BrushFamily`. */
export interface BrushFamily extends EmbindObject {
  getCoatCount(): number;
  getCoat(index: number): BrushCoat;
}

/** Constructor and static factory methods for `BrushFamily`. */
export interface BrushFamilyConstructor {
  new(): BrushFamily;
  fromCoat(coat: BrushCoat): BrushFamily;
  fromCoats(coats: BrushCoat[]): BrushFamily;
}

/** Static factory methods for `ink::stock_brushes`. */
export interface StockBrushesStatic {
  marker(): BrushFamily;
  pressurePen(): BrushFamily;
  highlighter(): BrushFamily;
  dashedLine(): BrushFamily;
}

/** Embind wrapper for `ink::Brush`. */
export interface Brush extends EmbindObject {
  getFamily(): BrushFamily;
  getColor(): Color;
  getSize(): number;
  getEpsilon(): number;
  coatCount(): number;
}

/** Constructor and static factory methods for `Brush`. */
export interface BrushConstructor {
  new(): Brush;
  create(
      family: BrushFamily,
      color: Color,
      size: number,
      epsilon: number,
      ): Brush;
}

/** Embind enum for `ink::StrokeInput::ToolType`. */
export interface ToolTypeEnum {
  readonly UNKNOWN: EmbindEnumValue;
  readonly MOUSE: EmbindEnumValue;
  readonly TOUCH: EmbindEnumValue;
  readonly STYLUS: EmbindEnumValue;
}

/** Embind wrapper for `ink::StrokeInput`. */
export interface StrokeInput extends EmbindObject {
  toolType: EmbindEnumValue;
  position: Point;
  elapsedTime: Duration32;
  strokeUnitLength: PhysicalDistance;
  pressure: number;
  tilt: Angle;
  orientation: Angle;
  barrelTwist: Angle;
  hasStrokeUnitLength(): boolean;
  hasPressure(): boolean;
  hasTilt(): boolean;
  hasOrientation(): boolean;
  hasBarrelTwist(): boolean;
}

/** Constructor and static factory methods for `StrokeInput`. */
export interface StrokeInputConstructor {
  new(): StrokeInput;
  new(position: Point, elapsedTime: Duration32): StrokeInput;
  new(
      toolType: EmbindEnumValue,
      position: Point,
      elapsedTime: Duration32,
      strokeUnitLength: PhysicalDistance,
      pressure: number,
      tilt: Angle,
      orientation: Angle,
      ): StrokeInput;
  create(position: Point, elapsedTime: Duration32): StrokeInput;
}

/** Embind wrapper for `ink::StrokeInputBatch`. */
export interface StrokeInputBatch extends EmbindObject {
  append(input: StrokeInput): void;
  size(): number;
  isEmpty(): boolean;
  get(index: number): StrokeInput;
  clear(): void;
}

/** Constructor for `StrokeInputBatch`. */
export interface StrokeInputBatchConstructor {
  new(): StrokeInputBatch;
}

/** Embind wrapper for `ink::Stroke`. */
export interface Stroke extends EmbindObject {
  getBrush(): Brush;
  getInputs(): StrokeInputBatch;
  renderGroupCount(): number;
}

/** Constructor for `Stroke`. */
export interface StrokeConstructor {
  new(brush: Brush): Stroke;
  new(brush: Brush, inputs: StrokeInputBatch): Stroke;
}

/** Embind wrapper for `ink::InProgressStroke`. */
export interface InProgressStroke extends EmbindObject {
  start(brush: Brush): void;
  enqueueInputs(
      realInputs: StrokeInputBatch,
      predictedInputs: StrokeInputBatch,
      ): void;
  finishInputs(): void;
  updateShape(currentElapsedTime: Duration32): void;
  copyToStroke(): Stroke;
  clear(): void;
  brushCoatCount(): number;
  inputCount(): number;
  realInputCount(): number;
  predictedInputCount(): number;
  meshVertexCount(coatIndex: number): number;
  meshTriangleCount(coatIndex: number): number;
}

/** Constructor for `InProgressStroke`. */
export interface InProgressStrokeConstructor {
  new(): InProgressStroke;
}

/** Embind wrapper for `ink::rendering::webgpu::WebGpuTextureStore`. */
export interface WebGpuTextureStore extends EmbindObject {
  setRgba8Texture(
      id: string,
      width: number,
      height: number,
      rgbaPixels: Uint8Array,
      ): void;
  hasTexture(id: string): boolean;
}

/** Constructor for `WebGpuTextureStore`. */
export interface WebGpuTextureStoreConstructor {
  new(): WebGpuTextureStore;
}

/** Embind wrapper for `ink::rendering::webgpu::WebGpuRenderer`. */
export interface WebGpuRenderer extends EmbindObject {
  beginRenderPass(width: number, height: number, clearColor: Color): void;
  drawInProgressStroke(
      stroke: InProgressStroke,
      width: number,
      height: number,
      ): void;
  drawStroke(stroke: Stroke, width: number, height: number): void;
  endRenderPass(): GPUTexture;
}

/** Static factory methods for `WebGpuRenderer`. */
export interface WebGpuRendererConstructor {
  create(textureStore: WebGpuTextureStore|null): WebGpuRenderer;
}

/**
 * Combined Emscripten module interface for `//third_party/ink/web:ink_wasm`.
 */
export interface InkWasmModule {
  preinitializedWebGPUDevice: GPUDevice;
  WebGPU: {
    importJsDevice(device: GPUDevice): number;
    importJsTexture(texture: GPUTexture): number;
    getJsObject(ptr: number): unknown;
  };
  Color: ColorConstructor;
  Point: PointConstructor;
  Vec: VecConstructor;
  Angle: AngleConstructor;
  Duration32: Duration32Constructor;
  PhysicalDistance: PhysicalDistanceConstructor;
  TextureOrigin: TextureOriginEnum;
  TextureSizeUnit: TextureSizeUnitEnum;
  TextureWrap: TextureWrapEnum;
  BlendMode: BlendModeEnum;
  SelfOverlap: SelfOverlapEnum;
  TilingTexture: TilingTextureConstructor;
  BrushPaint: BrushPaintConstructor;
  BrushCoat: BrushCoatConstructor;
  BrushFamily: BrushFamilyConstructor;
  StockBrushes: StockBrushesStatic;
  Brush: BrushConstructor;
  ToolType: ToolTypeEnum;
  StrokeInput: StrokeInputConstructor;
  StrokeInputBatch: StrokeInputBatchConstructor;
  Stroke: StrokeConstructor;
  InProgressStroke: InProgressStrokeConstructor;
  WebGpuTextureStore: WebGpuTextureStoreConstructor;
  WebGpuRenderer: WebGpuRendererConstructor;
}
