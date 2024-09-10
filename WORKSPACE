# This file marks the root of the bazel workspace for Bazel 6.2 and earlier.
# See MODULE.bazel for external dependencies setup.

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Set up NDK toolchains here; this doesn't work in the MODULE.bazel file.
#
# TODO(b/350001211): Set this up in the MODULE.bazel file instead.
RULES_ANDROID_NDK_COMMIT= "86c88af2729cc752de148808c5b5dc5830a35d2b"
http_archive(
    name = "rules_android_ndk",
    urls = ["https://github.com/bazelbuild/rules_android_ndk/archive/" + RULES_ANDROID_NDK_COMMIT + ".tar.gz"],
    strip_prefix="rules_android_ndk-" + RULES_ANDROID_NDK_COMMIT,
    integrity = "sha256-RIhjYLlElW9iVDgaZtPNj/VPqZKqSyrakZ4kRXN/GO8=",
)

load("@rules_android_ndk//:rules.bzl", "android_ndk_repository")
android_ndk_repository(name = "androidndk")
register_toolchains("@androidndk//:all")
