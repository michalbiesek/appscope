# CMake generated Testfile for 
# Source directory: /home/michalbiesek/Github/mimalloc
# Build directory: /home/michalbiesek/Github/mimalloc/out/release
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test-api "/home/michalbiesek/Github/mimalloc/out/release/mimalloc-test-api")
set_tests_properties(test-api PROPERTIES  _BACKTRACE_TRIPLES "/home/michalbiesek/Github/mimalloc/CMakeLists.txt;393;add_test;/home/michalbiesek/Github/mimalloc/CMakeLists.txt;0;")
add_test(test-api-fill "/home/michalbiesek/Github/mimalloc/out/release/mimalloc-test-api-fill")
set_tests_properties(test-api-fill PROPERTIES  _BACKTRACE_TRIPLES "/home/michalbiesek/Github/mimalloc/CMakeLists.txt;393;add_test;/home/michalbiesek/Github/mimalloc/CMakeLists.txt;0;")
add_test(test-stress "/home/michalbiesek/Github/mimalloc/out/release/mimalloc-test-stress")
set_tests_properties(test-stress PROPERTIES  _BACKTRACE_TRIPLES "/home/michalbiesek/Github/mimalloc/CMakeLists.txt;393;add_test;/home/michalbiesek/Github/mimalloc/CMakeLists.txt;0;")
