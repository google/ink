# Ink Brush Internal

Internal headers and bindings for `third_party/ink/brush`:

*   `brush_family_internal_accessor.h`: Defines `BrushFamilyInternalAccessor`, a
    friend class of `BrushFamily` restricted to `//third_party/ink/brush` and
    `//third_party/ink/storage`. Allows `DecodeBrushFamily` and
    `EncodeBrushFamily` (`../../storage/brush.cc`) to read and write
    `BrushFamily::opaque_decoded_proto_bytes_with_fallbacks_` so serialized
    fallback brush families (`newer_brush_families`) survive decode/encode
    round-trips without exposing raw proto bytes on `BrushFamily`'s public API.
*   `jni/`: C ABI and JNI bindings backing `androidx.ink.brush`.
