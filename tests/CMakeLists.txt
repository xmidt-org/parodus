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
set (PARODUS_COMMON_LIBS gcov -lcunit -lcimplog -lwrp-c -lpthread -lm -lmsgpackc -lcjson -ltrower-base64 -lnopoll)

link_directories ( ${LIBRARY_DIR} )

#-------------------------------------------------------------------------------
#   test_mutex
#-------------------------------------------------------------------------------
add_test(NAME test_mutex COMMAND test_mutex)
add_executable(test_mutex test_mutex.c ../src/mutex.c)
target_link_libraries (test_mutex ${PARODUS_COMMON_LIBS} -lrt)


#-------------------------------------------------------------------------------
#   test_networking
#-------------------------------------------------------------------------------
add_test(NAME test_networking COMMAND test_networking)
add_executable(test_networking test_networking.c ../src/networking.c)
target_link_libraries (test_networking ${PARODUS_COMMON_LIBS} -lrt)

#-------------------------------------------------------------------------------
#   test_nopoll_helpers
#-------------------------------------------------------------------------------
add_test(NAME test_nopoll_helpers COMMAND test_nopoll_helpers)
add_executable(test_nopoll_helpers test_nopoll_helpers.c ../src/nopoll_helpers.c ../src/wss_mgr.c ../src/networking.c ../src/string_helpers.c ../src/mutex.c ../src/time.c ../src/config.c ../src/connection.c ../src/spin_thread.c ../src/client_list.c)
target_link_libraries (test_nopoll_helpers ${PARODUS_COMMON_LIBS} -lnanomsg -lrt)

add_executable (libpd_test libpd_test.c)

target_link_libraries (libpd_test
                       cunit
                       -llibparodus
                       -lwrp-c
                       -lmsgpackc
                       -ltrower-base64
                       -lnanomsg
                       -lcimplog 
                       -lm
                       -lpthread
                       -lrt)

if (NOT EXCLUDE_SIMPLE)
#-------------------------------------------------------------------------------
#   simple test
#-------------------------------------------------------------------------------
add_test(NAME simple COMMAND simple)
add_executable(simple simple.c
                      ../src/wss_mgr.c
                      ../src/networking.c
                      ../src/nopoll_helpers.c
                      ../src/string_helpers.c
                      ../src/mutex.c
                      ../src/time.c
                      ../src/config.c
                      ../src/connection.c
                      ../src/spin_thread.c
					  ../src/client_list.c) 
		      
target_link_libraries (simple ${PARODUS_COMMON_LIBS} gcov -lnopoll -lnanomsg -lrt )
endif (NOT EXCLUDE_SIMPLE)

#-------------------------------------------------------------------------------
#   test_time
#-------------------------------------------------------------------------------
add_test(NAME test_time COMMAND test_time)
add_executable(test_time test_time.c ../src/time.c)
target_link_libraries (test_time ${PARODUS_COMMON_LIBS} -lrt)

#-------------------------------------------------------------------------------
#   test_connection
#-------------------------------------------------------------------------------
add_test(NAME test_connection COMMAND test_connection)
add_executable(test_connection test_connection.c
                      ../src/wss_mgr.c
                      ../src/networking.c
                      ../src/nopoll_helpers.c
                      ../src/string_helpers.c
                      ../src/mutex.c
                      ../src/time.c
                      ../src/config.c
                      ../src/connection.c
                      ../src/spin_thread.c
					  ../src/client_list.c) 
target_link_libraries (test_connection ${PARODUS_COMMON_LIBS} gcov -lnopoll -lnanomsg -lrt)

#-------------------------------------------------------------------------------
#   test_client_list
#-------------------------------------------------------------------------------
add_test(NAME test_client_list COMMAND test_client_list)
add_executable(test_client_list test_client_list.c ../src/client_list.c ../src/wss_mgr.c ../src/networking.c ../src/nopoll_helpers.c ../src/string_helpers.c ../src/mutex.c ../src/time.c ../src/config.c ../src/connection.c ../src/spin_thread.c)
target_link_libraries (test_client_list ${PARODUS_COMMON_LIBS} gcov -lnopoll -lnanomsg -lrt)

