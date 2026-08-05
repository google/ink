# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Macros for generating shaders from WGSL."""

def generate_shader(name, wgsl, entry_point, format, out, **kwargs):
    """Outputs a shader file in the given format, generated from the given WGSL source.

    Includes a `golden_test` to compare the output with a golden file.

    Args:
        name: The base target name.
        wgsl: The input WGSL filename.
        entry_point: The name of the entry point (e.g., `vertexMain`, `fragmentMain`) in the `wgsl`
            source file.
        format: The output format, as expected by Tint's `--format` flag (e.g., `msl`).
        out: The output filename, to be used as a dependency by a renderer implementation.
        **kwargs: Additional arguments to pass to the genrule.
    """

    # Run Tint to generate the raw shader, and annotate it with a comment.
    native.genrule(
        name = name,
        srcs = [wgsl],
        outs = [out],
        cmd = (
            "echo '// Generated from WGSL source with Tint, do not edit manually.' > $@; " +
            "$(location @dawn//src/tint/cmd/tint:cmd) $< --disable-robustness " +
            "--format {} --ep {} >> $@"
        ).format(format, entry_point),
        tools = ["@dawn//src/tint/cmd/tint:cmd"],
        **kwargs
    )
