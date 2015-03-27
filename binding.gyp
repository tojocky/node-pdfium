{
  'includes': [
      './common.gypi'
  ],
  'target_defaults': {
    'defines' : [
      'PNG_PREFIX',
      'PNGPREFIX_H',
      'PNG_USE_READ_MACROS',
    ],
#    'include_dirs': [
#      '<(DEPTH)/third_party/pdfium',
#      '<(DEPTH)/third_party/pdfium/third_party/freetype/include',
#    ],
    'conditions': [
      ['OS=="linux"', {
        'conditions': [
          ['target_arch=="x64"', {
            'defines' : [ '_FX_CPU_=_FX_X64_', ],
            'cflags': [ '-fPIC', ],
          }],
          ['target_arch=="ia32"', {
            'defines' : [ '_FX_CPU_=_FX_X86_', ],
          }],
        ],
      }]
    ],
    'msvs_disabled_warnings': [
      4005, 4018, 4146, 4333, 4345, 4267
    ]
  },
  'targets': [
    {
      'target_name': 'node_pdfium',
      'dependencies' : [
        'fx_lpng',
        './third_party/pdfium/pdfium.gyp:pdfium'
      ],
      'sources': [
        # is like "ls -1 src/*.cc", but gyp does not support direct patterns on
        # sources
        '<!@(["python", "tools/getSourceFiles.py", "src", "cc"])'
      ]
    },
    {
      'target_name': 'fx_lpng',
      'type': 'static_library',
      'dependencies': [
        'third_party/pdfium/pdfium.gyp:fxcodec',
      ],
      'include_dirs': [
        'third_party/pdfium/core/src/fxcodec/fx_zlib/include/',
      ],
      'sources': [
        'third_party/fx_lpng/include/fx_png.h',
        'third_party/fx_lpng/src/fx_png.c',
        'third_party/fx_lpng/src/fx_pngerror.c',
        'third_party/fx_lpng/src/fx_pngget.c',
        'third_party/fx_lpng/src/fx_pngmem.c',
        'third_party/fx_lpng/src/fx_pngpread.c',
        'third_party/fx_lpng/src/fx_pngread.c',
        'third_party/fx_lpng/src/fx_pngrio.c',
        'third_party/fx_lpng/src/fx_pngrtran.c',
        'third_party/fx_lpng/src/fx_pngrutil.c',
        'third_party/fx_lpng/src/fx_pngset.c',
        'third_party/fx_lpng/src/fx_pngtrans.c',
        'third_party/fx_lpng/src/fx_pngwio.c',
        'third_party/fx_lpng/src/fx_pngwrite.c',
        'third_party/fx_lpng/src/fx_pngwtran.c',
        'third_party/fx_lpng/src/fx_pngwutil.c',
      ]
    }
  ]
}
