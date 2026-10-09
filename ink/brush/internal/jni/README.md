# Brush JNI and C-Interop Bindings

Provides C ABI and JNI bindings for Ink's brush types (`Brush`, `BrushFamily`,
`BrushCoat`, `BrushTip`, `BrushPaint`, `BrushBehavior`, `BrushBehavior::Node`,
`ColorFunction`, `EasingFunction`, `StrokeInput::ToolType`, `SelfOverlap`, and
`stock_brushes`):

*   `*_native.*`: Pure C (`extern "C"`) functions that manage heap-allocated C++
    brush objects via opaque `int64_t` handles. Shared between JNI (`*_jni.cc`)
    and Kotlin/Native `cinterop` (`//third_party/ink/kmp`).
*   `*_jni.cc`: Thin JNI wrappers that marshal JVM/Android arrays and strings
    and delegate to `*_native` functions, propagating `absl::Status` errors as
    Java exceptions via `//third_party/ink/jni/internal`.
*   `brush_native_helper.*`: Shared C++ helpers for converting between opaque
    `int64_t` pointers and C++ brush types; also used by
    `../../strokes/internal/jni/` and `../../storage/internal/jni/`.
