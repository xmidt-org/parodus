#   Copyright 2016 Comcast Cable Communications Management, LLC
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")
set (PARODUS_COMMON_LIBS gcov cunit -lwrp-c -lpthread -lm -lmsgpackc -lcjson -ltrower-base64)

link_directories ( ${LIBRARY_DIR} )

#-------------------------------------------------------------------------------
#   test_networking
#-------------------------------------------------------------------------------
add_test(NAME test_networking COMMAND test_networking)
add_executable(test_networking test_networking.c ../src/networking.c )
target_link_libraries (test_networking ${PARODUS_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_nopoll_helpers
#-------------------------------------------------------------------------------
add_test(NAME test_nopoll_helpers COMMAND test_nopoll_helpers)
add_executable(test_nopoll_helpers test_nopoll_helpers.c ../src/nopoll_helpers.c )
target_link_libraries (test_nopoll_helpers ${PARODUS_COMMON_LIBS} -lnanomsg)

#-------------------------------------------------------------------------------
#   simple test
#-------------------------------------------------------------------------------
add_test(NAME simple COMMAND simple)
add_executable(simple simple.c
                      ../src/wss_mgr.c
                      ../src/networking.c
                      ../src/nopoll_helpers.c
                      ../src/string_helpers.c
                      ../src/mutex.c )
target_link_libraries (simple ${PARODUS_COMMON_LIBS} gcov -lnopoll -lnanomsg -lrt )




add_custom_target(coverage-prog
                  COMMAND lcov -q --capture --directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/simple.dir/__/src --output-file ${CMAKE_CURRENT_BINARY_DIR}/coverage.info
                  COMMAND lcov -q --capture --directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_networking.dir/__/src --output-file ${CMAKE_CURRENT_BINARY_DIR}/coverage.info
                  COMMAND lcov -q --capture --directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_nopoll_helpers.dir/__/src --output-file ${CMAKE_CURRENT_BINARY_DIR}/coverage.info
                 )

