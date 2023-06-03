{
  'targets': [
    {
      'target_name': 'gorilla-codec-native',
      'sources': [ 'src/gorilla_codec.cc', "src/aligned_buffer.cpp", "src/compressed_buffer.cpp", "src/integer_encoder.cpp", "src/simple8b.cpp", "src/float_encoder.cpp" ],
      'include_dirs': ["<!@(node -p \"require('node-addon-api').include\")", "/usr/local/include"],
      'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7'
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
      "libraries": [
        "-L/usr/local/lib",
        "-lsnappy"
      ],
    }
  ]
}