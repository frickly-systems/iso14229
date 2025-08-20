package(default_visibility = ["//visibility:public"])

filegroup(
    name = "sources",
    srcs = [
        "client.c",
        "log.c",
        "server.c",
        "tp.c",
        "util.c",
        "tp/isotp_c_socketcan.c",
        "tp/isotp_c.c",
        "tp/isotp_mock.c",
        "tp/isotp_sock.c",
        "tp/isotp-c/isotp.c",
    ]
)

filegroup(
    name = "headers",
    srcs = [
        "version.h",
    ]
)

genrule(
    name="amalgamated",
    srcs=[
        "//src:sources",
        "//src:headers",
    ],
    outs=["iso14229.c", "iso14229.h"],
    cmd="$(location //tools:amalgamate) --out_c $(location :iso14229.c) --out_h $(location :iso14229.h) $(SRCS)",
    tools=["//tools:amalgamate"],
)