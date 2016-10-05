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

add_test(NAME NMsgTest COMMAND nmsg)
add_test(NAME LibPDTest COMMAND libpd)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")

add_executable(nmsg nmsg.c)

target_link_libraries (nmsg
 ${INSTALL_DIR}/lib64/libnanomsg.so
 -lnanomsg
 -lm
 -lpthread
)

add_executable(libpd libpd_test.c)

target_link_libraries (libpd
 cunit
 ${BUILD_DIR}/src/libparodus-lib.so
 ${INSTALL_DIR}/lib64/libnanomsg.so
 ${INSTALL_DIR}/lib/libwrp-c.so
 ${INSTALL_DIR}/lib/libmsgpackc.so
 ${INSTALL_DIR}/lib/libtrower-base64.so
 -lm
 -lpthread
)

#target_link_libraries (simple gcov)
#target_link_libraries (simple cunit)

#add_custom_target(coverage
#                  COMMAND lcov -q --capture --directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/simple.dir/__/src --output-file coverage.info
#                  COMMAND genhtml coverage.info
#                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
