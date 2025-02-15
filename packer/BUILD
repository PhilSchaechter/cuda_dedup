cc_library(
    name = "packer",
    srcs = ["packer.c"],
    hdrs = ["packer_const.h", "packer_struct.h", "packer.h"],
    includes = ["."],
    linkopts = ["-pthread"],
    visibility = ["//index:__pkg__", "//system:__pkg__"],
    deps = [
        "//system:logger",
	"//store:store",
],
)

cc_binary(
    name = "unit-test",
    srcs = ["packer_unit_test.c"],
    deps = [
	":packer",
    ],
)
