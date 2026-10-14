# Ink Brush

This directory defines Ink's brush data model, which specifies how raw stroke
inputs are modeled, extruded into stroke meshes (`Stroke` and
`InProgressStroke`), and shaded by renderers.

## Core Types and Files

*   `Brush`: Pairs a `BrushFamily` with a base `Color`, `size`, and visual
    fidelity (`epsilon`).
*   `BrushFamily`: Combines one or more `BrushCoat`s with an `InputModel` for
    smoothing and upsampling raw inputs, plus client `Metadata`.
*   `BrushCoat`: Represents a single coat of ink, draw with one `BrushTip` and a
    prioritized list of `BrushPaint` preferences.
*   `BrushTip`: Specifies the base tip shape, particle emission gaps, and a list
    of dynamic `BrushBehavior`s.
*   `BrushBehavior`: A graph of expression nodes that dynamically modifies tip
    shape, offset, paint animation progress, or per-vertex HCLA color shift from
    modeled stroke inputs.
*   `EasingFunction`: Defines response curves used by
    `BrushBehavior::ResponseNode`. The availble types of curves are mostly based
    on CSS easing functions.
*   `BrushPaint`: Specifies zero or more `TextureLayer`s (which can be combined
    with Porter-Duff `BlendMode`s), as well as `ColorFunction`s to modify the
    base brush color, and the `SelfOverlap` mode to use for translucent strokes.
*   `ColorFunction`: Transforms the base `Brush` color for a single brush coat.
*   `Version`: Descripts the minimum required serialization format version for a
    given brush.
*   `stock_brushes.*`: Versioned factory functions for built-in `BrushFamily`
    presets (e.g. `PressurePen`, `Highlighter`, etc.).
*   `fuzz_domains.*`, `type_matchers.*`, `stock_brushes_test_params.*`: FuzzTest
    domains, GoogleTest matchers, and parameterized test inputs.
