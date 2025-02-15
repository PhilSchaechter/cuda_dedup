cc_library(
    name = "logger",
    srcs = ["is_logger.c"],
    hdrs = ["is_logger.h"],
    includes = ["."],
    linkopts = ["-lzlog", "-L/usr/local/lib"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "system",
    srcs = ["system.c"],
    hdrs = ["system.h", "system_struct.h"],
    includes = ["."],
    linkopts = ["-pthread"],
    deps = [
        ":logger",
        "//store:store",
        "//packer:packer",
        "//index:index",
],
)

cc_binary(
    name = "unit-test",
    srcs = ["system_unit_test.c"],
    deps = [
        ":system",
    ],
)

