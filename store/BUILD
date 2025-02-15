cc_library(
    name = "store",
    srcs = ["store.c"],
    hdrs = ["store_const.h", "store_struct.h", "store.h"],
    includes = ["."],
    visibility = ["//packer:__pkg__", "//index:__pkg__", "//system:__pkg__"],
    linkopts = ["-pthread"],
    deps = [
	"//system:logger",
    ],
)

cc_binary(
    name = "unit-test",
    srcs = ["store_unit_test.c"],
    deps = [
	":store",
    ],
)
