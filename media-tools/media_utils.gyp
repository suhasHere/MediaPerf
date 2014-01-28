{
    "includes": [
        "common.gypi"
    ],
    "targets": [
        {
            "target_name": "media_utils",
            "product_name": "media_utils",
            "type": "static_library",
            "sources": [
                "src/media_utils.h",
                "src/file_wrapper.h",
                "src/file_wrapper.cc",
                "src/media_provider.h",
		"src/media_provider.cc",
                "src/media_processor.h",
		"src/media_processor.cc",
            ],
            "include_dirs": [
                "include"
            ],
            'direct_dependent_settings': {
              'include_dirs': [ 'include/' ],
            },
        },
        {
            "target_name": "MediaUtils",
            "type": "executable",
            "sources": [
                "src/media_app.cc"
            ],
            "include_dirs": [
                "src"
            ],
            "dependencies": [
                "media_utils"
            ]
        }
    ]
}
