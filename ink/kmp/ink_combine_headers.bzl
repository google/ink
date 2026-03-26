"""Rule for combining Ink headers for native cinterop into a single file for export."""

def _ink_combine_headers_impl(ctx):
    header_files = sorted(ctx.files.srcs, key = lambda header: header.path)
    args = ctx.actions.args()
    args.add_all(header_files)
    ctx.actions.run_shell(
        outputs = [ctx.outputs.out],
        inputs = header_files,
        mnemonic = "InkCombineHeaders",
        arguments = [args],
        command = "cat $@ > {}".format(ctx.outputs.out.path),
    )

ink_combine_headers = rule(
    attrs = {
        "srcs": attr.label_list(),
        "out": attr.output(mandatory = True),
    },
    implementation = _ink_combine_headers_impl,
)
