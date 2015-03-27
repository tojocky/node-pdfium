{
  'conditions': [
    ['component=="shared_library"', {
      'cflags': [
        '-fPIC',
      ],
    }],
    ['OS=="win"', {
      'target_defaults': {
        'defines': [
          'NOMINMAX',
          '_CRT_SECURE_NO_DEPRECATE',
          '_CRT_NONSTDC_NO_DEPRECATE'
        ],
        'conditions': [
          ['component=="static_library"', {
            'defines': [
              '_HAS_EXCEPTIONS=0',
            ],
          }],
        ],
      },
    }],  # OS=="win"
    ['OS=="mac"', {
      'target_defaults': {
        'target_conditions': [
          ['_type!="static_library"', {
            'xcode_settings': {'OTHER_LDFLAGS': ['-Wl,-search_paths_first']},
          }],
        ],  # target_conditions
      },  # target_defaults
    }],  # OS=="mac"
  ],
  'target_defaults': {
    'defines': [
      'BUILDING_NODE_EXTENSION' # required for dependent projects
    ],
  	'defines!': [
      'DEBUG'
    ],
    'cflags': [
      '-Wall',
      '-W',
      '-Wno-unused-parameter',
      '-pthread',
      '-fno-exceptions',
      '-fvisibility=hidden',
    ],
    'cflags_cc': [
      '-std=gnu++0x',
      '-Wnon-virtual-dtor',
      '-fno-rtti',
    ],
    'ldflags': [
      '-pthread',
    ],
    'conditions' : [
      ['OS=="mac"', {
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11'],
          #'MACOSX_DEPLOYMENT_TARGET': '10.7',
          #'ARCHS': '$(ARCHS_STANDARD_64_BIT)'
        },
        'link_settings': {
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/CoreServices.framework',
            '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework'
          ]
        }
      }]
    ]
  }
}