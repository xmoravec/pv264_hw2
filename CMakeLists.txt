cmake_minimum_required(VERSION 3.8)
project(blist)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -g" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fprofile-arcs -ftest-coverage" )
if ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
    set(GCOV_COMMAND "llvm-cov gcov")
else()
    set(GCOV_COMMAND "gcov -r")
endif()

add_subdirectory("rapidcheck")

add_custom_target(git_update
                  COMMAND git submodule update -i
                  VERBATIM
                 )
set(SRCS main.cpp test_static_vector.cpp test_blist.cpp)
add_executable(blist_test ${SRCS})
add_executable(blist_test_san ${SRCS})
add_dependencies(blist_test git_update)
target_link_libraries(blist_test rapidcheck)
target_link_libraries(blist_test_san rapidcheck)
set_target_properties(blist_test_san PROPERTIES COMPILE_FLAGS "-fsanitize=address")
set_target_properties(blist_test_san PROPERTIES LINK_FLAGS "-fsanitize=address")
set(TEST_ENV env "RC_PARAMS=seed=0 max_success=1000 max_size=100")
set(TEST_ENV_VG env "RC_PARAMS=seed=0 max_success=100 max_size=100")
add_custom_target(unit
                  COMMAND find ${CMAKE_CURRENT_BUILD_DIR} -name *.gcda -delete
                  COMMAND ${TEST_ENV} ./blist_test_san
                  COMMAND bash -c "${GCOV_COMMAND} main.cpp -o CMakeFiles/blist_test_san.dir/test_blist.cpp.gcno | grep -A1 'File.*blist.hpp'"
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR}
                  SOURCES main.cpp
                  VERBATIM
                  DEPENDS blist_test_san
                 )
add_custom_target(vgunit
                  COMMAND find ${CMAKE_CURRENT_BUILD_DIR} -name *.gcda -delete
                  COMMAND ${TEST_ENV_VG} valgrind --leak-check=full --num-callers=64 ./blist_test
                  COMMAND bash -c "${GCOV_COMMAND} main.cpp -o CMakeFiles/blist_test.dir/test_blist.cpp.gcno | grep -A1 'File.*blist.hpp'"
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR}
                  SOURCES main.cpp
                  VERBATIM
                  DEPENDS blist_test
                 )
add_custom_target(lint
                  COMMAND clang-tidy -header-filter=blist.hpp test_blist.cpp -- -std=c++17 -Irapidcheck/include
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  VERBATIM
                 )
