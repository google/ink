# Ink

[![BazelTest](https://github.com/google/ink/actions/workflows/bazel-test.yaml/badge.svg)](https://github.com/google/ink/actions/workflows/bazel-test.yaml)

The Ink library is a freehand stroke generation library. It produces smoothed,
modeled stroke shapes with brush effect shaders as mesh-based vector graphics.

This provides the core of the implementation of the Android Jetpack
[Ink](https://developer.android.com/jetpack/androidx/releases/ink) library.
While the implementation is well-tested, the developers of this library are not
currently making hard guarantees about interface stability.

## How to Build and Test

### Bazel

#### Prerequisites:

*   Bazel 7: https://bazel.build/install
*   Android NDK: https://developer.android.com/ndk/downloads
    *   Download and set `ANDROID_NDK_HOME` to point to it.
    *   Alternately use `sdkmanager` or Android Studio:
        https://developer.android.com/studio/projects/install-ndk
*   You may need to install libtinfo for clang
    *   `sudo apt-get install libtinfo5`

Ink can be built and tested from the repo root:

```shell
bazel test --config=linux ink/...
```

## Library Structure

Ink consists of a set of modules that can be used separately. You should only
need to include the parts of the library that you need. The basic dependency
structure is:

```
 ┌──────────┐ ┌───────┐
 │Rendering │ │Storage│
 └────┬─────┘ └──┬────┘
      │          │
      ▼          │
   ┌───────┐     │
   │Strokes│◄────┘
   └───┬───┘
       ▼
   ┌───────┐
   | Brush |
   └─┬────┬┘
     │    │
     │    ▼
     │ ┌────────┐
     │ │Geometry│
     │ └───┬────┘
     │     │
     ▼     ▼
 ┌─────┐ ┌─────┐
 │Color│ │Types│
 └─────┘ └─────┘
```

*   `color`: Color spaces, encoding, and format conversion.
*   `types`: Utility types; time, units, constants, small arrays.
*   `geometry`: Geometric types (point, segment, triangle, rect, quad), meshes,
    transforms, utility functions, and algorithms (intersection, envelope).
*   `brush`: Defines stroke styles and behaviors.
*   `strokes`: The primary `Stroke` data type and `InProgressStroke` builder.
*   `rendering`: Rendering utilities for strokes. Currently only has support for
    android.graphics.Mesh based rendering.
*   `storage`: Protobuf serialization utilities for `Stroke` and related types.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on sending a PR.

## Contact

Use GitHub Issues to file feedback: https://github.com/google/ink/issues
