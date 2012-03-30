{
    "targets": [
        {
            "target_name": "binding",
            "sources": [
                "src/common.cpp",
                "src/png_encoder.cpp",
                "src/png.cpp",
                "src/fixed_png_stack.cpp",
                "src/dynamic_png_stack.cpp",
                "src/module.cpp",
                "src/buffer_compat.cpp",
            ],
            "include_dirs" : [ "gyp/include" ],
            "libraries" : [
                '<(module_root_dir)/gyp/lib/libpng.lib',
                '<(module_root_dir)/gyp/lib/zlib.lib'
            ]
        }
    ]
}

