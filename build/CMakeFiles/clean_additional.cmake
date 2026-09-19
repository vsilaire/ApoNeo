# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "src/core/ApoNeoCore_autogen"
  "src/core/CMakeFiles/ApoNeoCore_autogen.dir/AutogenUsed.txt"
  "src/core/CMakeFiles/ApoNeoCore_autogen.dir/ParseCache.txt"
  "src/engine/ApoNeoEngine_autogen"
  "src/engine/CMakeFiles/ApoNeoEngine_autogen.dir/AutogenUsed.txt"
  "src/engine/CMakeFiles/ApoNeoEngine_autogen.dir/ParseCache.txt"
  "src/ui/ApoNeoApp_autogen"
  "src/ui/CMakeFiles/ApoNeoApp_autogen.dir/AutogenUsed.txt"
  "src/ui/CMakeFiles/ApoNeoApp_autogen.dir/ParseCache.txt"
  "tests/CMakeFiles/test_engine_autogen.dir/AutogenUsed.txt"
  "tests/CMakeFiles/test_engine_autogen.dir/ParseCache.txt"
  "tests/CMakeFiles/test_genome_autogen.dir/AutogenUsed.txt"
  "tests/CMakeFiles/test_genome_autogen.dir/ParseCache.txt"
  "tests/test_engine_autogen"
  "tests/test_genome_autogen"
  )
endif()
