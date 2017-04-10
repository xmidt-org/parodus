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
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTEST ")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")
set (PARODUS_COMMON_SRC ../src/string_helpers.c ../src/mutex.c ../src/time.c ../src/config.c ../src/spin_thread.c)
set (PARODUS_COMMON_LIBS gcov -lcunit -lcimplog -llibseshat -lwrp-c -luuid -lpthread -lm -lmsgpackc -lcjson -ltrower-base64 -lnopoll -lnanomsg -Wl,--no-as-needed -lrt)

link_directories ( ${LIBRARY_DIR} )

#-------------------------------------------------------------------------------
#   test_mutex
#-------------------------------------------------------------------------------
add_test(NAME test_mutex COMMAND test_mutex)
add_executable(test_mutex test_mutex.c ../src/mutex.c)
target_link_libraries (test_mutex ${PARODUS_COMMON_LIBS} -lcmocka)

#-------------------------------------------------------------------------------
#   test_networking
#-------------------------------------------------------------------------------
add_test(NAME test_networking COMMAND test_networking)
add_executable(test_networking test_networking.c ../src/networking.c)
target_link_libraries (test_networking ${PARODUS_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_nopoll_helpers
#-------------------------------------------------------------------------------
add_test(NAME test_nopoll_helpers COMMAND test_nopoll_helpers)
add_executable(test_nopoll_helpers test_nopoll_helpers.c ../src/nopoll_helpers.c)
target_link_libraries (test_nopoll_helpers -Wl,--no-as-needed -lrt -lcmocka -lcimplog -lnopoll)

#-------------------------------------------------------------------------------
#   libpd_test
#-------------------------------------------------------------------------------
add_executable (libpd_test libpd_test.c)

target_link_libraries (libpd_test
                       cunit
                       -llibparodus
                       -lwrp-c
		       -llibseshat
		       -luuid
                       -lmsgpackc
                       -ltrower-base64
                       -lnanomsg
                       -lcimplog 
                       -lm
                       -lpthread
                       -lrt)

#-------------------------------------------------------------------------------
#   test_time
#-------------------------------------------------------------------------------
add_test(NAME test_time COMMAND test_time)
add_executable(test_time test_time.c ../src/time.c)
target_link_libraries (test_time ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_spin_thread error
#-------------------------------------------------------------------------------
add_test(NAME test_spin_thread_e COMMAND test_spin_thread_e)
add_executable(test_spin_thread_e test_spin_thread_e.c ../src/spin_thread.c)
target_link_libraries (test_spin_thread_e ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_spin_thread success
#-------------------------------------------------------------------------------
add_test(NAME test_spin_thread_s COMMAND test_spin_thread_s)
add_executable(test_spin_thread_s test_spin_thread_s.c ../src/spin_thread.c)
target_link_libraries (test_spin_thread_s ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_string_helpers 
#-------------------------------------------------------------------------------
add_test(NAME test_string_helpers COMMAND test_string_helpers)
add_executable(test_string_helpers test_string_helpers.c ../src/string_helpers.c)
target_link_libraries (test_string_helpers ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_nopoll_handlers
#-------------------------------------------------------------------------------
add_test(NAME test_nopoll_handlers COMMAND test_nopoll_handlers)
add_executable(test_nopoll_handlers test_nopoll_handlers.c ../src/nopoll_handlers.c)
target_link_libraries (test_nopoll_handlers -lnopoll -lcunit -lcimplog -Wl,--no-as-needed -lrt -lpthread -lm)

#-------------------------------------------------------------------------------
#   test_connection
#-------------------------------------------------------------------------------
add_test(NAME test_connection COMMAND test_connection)
add_executable(test_connection test_connection.c ../src/connection.c ${PARODUS_COMMON_SRC}) 
target_link_libraries (test_connection ${PARODUS_COMMON_LIBS} -lcmocka)

#-------------------------------------------------------------------------------
#   test_connection - function createNopollConnection
#-------------------------------------------------------------------------------
add_test(NAME test_createConnection COMMAND test_createConnection)
add_executable(test_createConnection test_createConnection.c ../src/connection.c ../src/string_helpers.c ../src/config.c) 
target_link_libraries (test_createConnection ${PARODUS_COMMON_LIBS} -lcmocka)

#-------------------------------------------------------------------------------
#   test_client_list
#-------------------------------------------------------------------------------
add_test(NAME test_client_list COMMAND test_client_list)
add_executable(test_client_list test_client_list.c ../src/client_list.c ../src/service_alive.c ../src/upstream.c ../src/networking.c ../src/nopoll_helpers.c ../src/downstream.c ../src/connection.c ../src/nopoll_handlers.c ../src/ParodusInternal.c ../src/thread_tasks.c ../src/conn_interface.c ../src/partners_check.c ${PARODUS_COMMON_SRC})
target_link_libraries (test_client_list ${PARODUS_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_service_alive
#-------------------------------------------------------------------------------
add_test(NAME test_service_alive COMMAND test_service_alive)
add_executable(test_service_alive test_service_alive.c ../src/client_list.c ../src/service_alive.c ../src/upstream.c ../src/networking.c ../src/nopoll_helpers.c ../src/nopoll_handlers.c ../src/config.c ../src/connection.c ../src/ParodusInternal.c ../src/downstream.c ../src/thread_tasks.c ../src/conn_interface.c ../src/partners_check.c ${PARODUS_COMMON_SRC})
target_link_libraries (test_service_alive ${PARODUS_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_config
#-------------------------------------------------------------------------------
add_test(NAME test_config COMMAND test_config)
add_executable(test_config test_config.c ../src/config.c ../src/string_helpers.c)
target_link_libraries (test_config -lcmocka -lm -Wl,--no-as-needed -lrt -lcimplog)

#-------------------------------------------------------------------------------
#   test_upstream
#-------------------------------------------------------------------------------
add_test(NAME test_upstream COMMAND test_upstream)
add_executable(test_upstream test_upstream.c ../src/upstream.c)
target_link_libraries (test_upstream -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_downstream
#-------------------------------------------------------------------------------
add_test(NAME test_downstream COMMAND test_downstream)
add_executable(test_downstream test_downstream.c ../src/downstream.c ../src/string_helpers.c)
target_link_libraries (test_downstream -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_thread_tasks
#-------------------------------------------------------------------------------
add_test(NAME test_thread_tasks COMMAND test_thread_tasks)
add_executable(test_thread_tasks test_thread_tasks.c ../src/thread_tasks.c)
target_link_libraries (test_thread_tasks -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_conn_interface
#-------------------------------------------------------------------------------
add_test(NAME test_conn_interface COMMAND test_conn_interface)
add_executable(test_conn_interface test_conn_interface.c ../src/conn_interface.c ../src/config.c ../src/string_helpers.c ../src/mutex.c)
target_link_libraries (test_conn_interface -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_ParodusInternal
#-------------------------------------------------------------------------------
add_test(NAME test_ParodusInternal COMMAND test_ParodusInternal)
add_executable(test_ParodusInternal test_ParodusInternal.c ../src/ParodusInternal.c ../src/config.c ../src/string_helpers.c)
target_link_libraries (test_ParodusInternal -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_partners_check
#-------------------------------------------------------------------------------
add_test(NAME test_partners_check COMMAND test_partners_check)
add_executable(test_partners_check test_partners_check.c ../src/partners_check.c)
target_link_libraries (test_partners_check -lcmocka -lwrp-c -lcimplog -Wl,--no-as-needed -lrt)

if (INTEGRATION_TESTING)
#-------------------------------------------------------------------------------
#   simple_connection test
#-------------------------------------------------------------------------------
add_test(NAME simple_connection COMMAND simple_connection)
add_executable(simple_connection simple_connection.c ${PARODUS_COMMON_SRC}
                    ../src/upstream.c
                    ../src/conn_interface.c
                    ../src/thread_tasks.c
                    ../src/downstream.c
                    ../src/networking.c
                    ../src/nopoll_helpers.c
                    ../src/nopoll_handlers.c
                    ../src/connection.c
                    ../src/ParodusInternal.c
                    ../src/client_list.c
                    ../src/partners_check.c
                    ../src/service_alive.c) 
target_link_libraries (simple_connection ${PARODUS_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   simple test
#-------------------------------------------------------------------------------
add_test(NAME simple COMMAND simple)
add_executable(simple simple.c
                    ../src/upstream.c
                    ../src/conn_interface.c
                    ../src/downstream.c
                    ../src/thread_tasks.c
                    ../src/networking.c
                    ../src/nopoll_helpers.c
                    ../src/nopoll_handlers.c
                    ../src/string_helpers.c
                    ../src/mutex.c
                    ../src/time.c
                    ../src/config.c
                    ../src/connection.c
                    ../src/ParodusInternal.c
                    ../src/spin_thread.c
                    ../src/client_list.c
                    ../src/partners_check.c
                    ../src/service_alive.c) 

target_link_libraries (simple ${PARODUS_COMMON_LIBS} gcov -lnopoll -lnanomsg )
endif (INTEGRATION_TESTING)
