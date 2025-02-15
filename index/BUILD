cc_library(
    name = "index",
    srcs = ["index.c"],
    hdrs = ["index.h", "index_const_dev.h", "index_struct.h"],
    includes = ["."],
    linkopts = ["-pthread"],
    visibility = ["//system:__pkg__"],
    deps = [
        "//system:logger",
	"//store:store",
	"//packer:packer",
],
)

cc_binary(
    name = "unit-test",
    srcs = ["index_unit_test.c"],
    deps = [
	":index",
    ],
)
