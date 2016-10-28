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

add_test(NAME test_internals COMMAND test_internals)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")
set (PARODUS_COMMON_LIBS gcov cunit -lwrp-c -lpthread -lm -lmsgpackc -lcJSON -ltrower-base64)


link_directories ( ${LIBRARY_DIR} )

add_executable(simple simple.c ../src/wss_mgr.c ../src/ParodusInternal.c )
add_executable(test_internals test_internals.c ../src/ParodusInternal.c )

target_link_libraries (simple ${PARODUS_COMMON_LIBS} -lnopoll -lnanomsg -lrt )


target_link_libraries (test_internals ${PARODUS_COMMON_LIBS} )


add_custom_target(coverage
                  COMMAND lcov -q --capture --directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_internals.dir/__/src --output-file coverage.info
                  COMMAND genhtml coverage.info
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

