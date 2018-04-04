#   Copyright 2016 Comcast Cable Communications Management, LLCD
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
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=600 -Du_char='unsigned char' -Du_int='unsigned int' -Du_long='unsigned long'")
endif()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")
set (PARODUS_COMMON_SRC ../src/string_helpers.c ../src/mutex.c ../src/time.c ../src/config.c ../src/spin_thread.c ../src/token.c)
set (PARODUS_COMMON_LIBS -lcunit -lcimplog -lwrp-c
 -lpthread -lmsgpackc -lnopoll -lnanomsg
 -lcjwt -lcjson -ltrower-base64 -lssl -lcrypto -lm)
set (PARODUS_LINUX_ONLY_LIBS gcov -luuid -lrt -Wl,--no-as-needed)

if (ENABLE_SESHAT)
set (PARODUS_COMMON_LIBS -llibseshat ${PARODUS_COMMON_LIBS})
endif (ENABLE_SESHAT)

if (FEATURE_DNS_QUERY)
set (PARODUS_COMMON_LIBS ${PARODUS_COMMON_LIBS} -lucresolv -lresolv)
endif (FEATURE_DNS_QUERY)

if(NOT DISABLE_VALGRIND)
set (MEMORY_CHECK valgrind --leak-check=full --show-reachable=yes -v)
endif ()

link_directories ( ${LIBRARY_DIR} )

#-------------------------------------------------------------------------------
#   test_mutex
#-------------------------------------------------------------------------------
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
add_test(NAME test_mutex COMMAND ./test_mutex)
else()
add_test(NAME test_mutex COMMAND ${MEMORY_CHECK} ./test_mutex)
endif()
add_executable(test_mutex test_mutex.c ../src/mutex.c)
target_link_libraries (test_mutex ${PARODUS_COMMON_LIBS} -lcmocka)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_mutex ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_networking
#-------------------------------------------------------------------------------
add_test(NAME test_networking COMMAND ${MEMORY_CHECK} ./test_networking)
add_executable(test_networking test_networking.c ../src/networking.c)
target_link_libraries (test_networking ${PARODUS_COMMON_LIBS})
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_networking ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_nopoll_helpers
#-------------------------------------------------------------------------------
add_test(NAME test_nopoll_helpers COMMAND ${MEMORY_CHECK} ./test_nopoll_helpers)
add_executable(test_nopoll_helpers test_nopoll_helpers.c ../src/nopoll_helpers.c)
target_link_libraries (test_nopoll_helpers -lcmocka -lcimplog -lnopoll)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_nopoll_helpers rt -Wl,--no-as-needed)
endif()

#-------------------------------------------------------------------------------
#   libpd_test
#-------------------------------------------------------------------------------
add_executable (libpd_test libpd_test.c ../src/string_helpers.c) 

target_link_libraries (libpd_test
                       cunit
                       -llibparodus
                       -lwrp-c
                       -lmsgpackc
                       -ltrower-base64
                       -lnanomsg
                       -lcimplog 
                       -lm
                       -lpthread)
if (ENABLE_SESHAT)
target_link_libraries (libpd_test -llibseshat)
endif (ENABLE_SESHAT)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (libpd_test ${PARODUS_LINUX_ONLY_LIBS})
endif()
#-------------------------------------------------------------------------------
#   test_time
#-------------------------------------------------------------------------------
add_test(NAME test_time COMMAND ${MEMORY_CHECK} ./test_time)
add_executable(test_time test_time.c ../src/time.c)
target_link_libraries (test_time ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_time ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_spin_thread error
#-------------------------------------------------------------------------------
add_test(NAME test_spin_thread_e COMMAND ${MEMORY_CHECK} ./test_spin_thread_e)
add_executable(test_spin_thread_e test_spin_thread_e.c ../src/spin_thread.c)
target_link_libraries (test_spin_thread_e ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_spin_thread_e ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_spin_thread success
#-------------------------------------------------------------------------------
add_test(NAME test_spin_thread_s COMMAND ${MEMORY_CHECK} ./test_spin_thread_s)
add_executable(test_spin_thread_s test_spin_thread_s.c ../src/spin_thread.c)
target_link_libraries (test_spin_thread_s ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_spin_thread_s ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_string_helpers 
#-------------------------------------------------------------------------------
add_test(NAME test_string_helpers COMMAND ${MEMORY_CHECK} ./test_string_helpers)
add_executable(test_string_helpers test_string_helpers.c ../src/string_helpers.c)
target_link_libraries (test_string_helpers ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_string_helpers ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_nopoll_handlers
#-------------------------------------------------------------------------------
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
add_test(NAME test_nopoll_handlers COMMAND ./test_nopoll_handlers)
else()
add_test(NAME test_nopoll_handlers COMMAND ${MEMORY_CHECK} ./test_nopoll_handlers)
endif()
add_executable(test_nopoll_handlers test_nopoll_handlers.c ../src/nopoll_handlers.c)
target_link_libraries (test_nopoll_handlers -lnopoll -lcunit -lcimplog -lpthread -lm)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_nopoll_handlers -lrt -Wl,--no-as-needed)
endif()

#-------------------------------------------------------------------------------
#   test_connection
#-------------------------------------------------------------------------------
add_test(NAME test_connection COMMAND ${MEMORY_CHECK} ./test_connection)
set(SOURCES test_connection.c ../src/connection.c ${PARODUS_COMMON_SRC})
add_executable(test_connection ${SOURCES})
target_link_libraries (test_connection ${PARODUS_COMMON_LIBS} -lcmocka)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_connection ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_connection - function createNopollConnection
#-------------------------------------------------------------------------------
add_test(NAME test_createConnection COMMAND ${MEMORY_CHECK} ./test_createConnection)
add_executable(test_createConnection test_createConnection.c ../src/connection.c ../src/string_helpers.c ../src/config.c) 
target_link_libraries (test_createConnection ${PARODUS_COMMON_LIBS} -lcmocka )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_createConnection ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_client_list
#-------------------------------------------------------------------------------
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
add_test(NAME test_client_list COMMAND ./test_client_list)
else()
add_test(NAME test_client_list COMMAND ${MEMORY_CHECK} ./test_client_list)
endif()
set(SOURCES test_client_list.c ../src/client_list.c 
 ../src/service_alive.c ../src/upstream.c ../src/networking.c ../src/nopoll_helpers.c 
 ../src/downstream.c ../src/connection.c ../src/nopoll_handlers.c 
 ../src/ParodusInternal.c ../src/thread_tasks.c ../src/conn_interface.c 
 ../src/partners_check.c ${PARODUS_COMMON_SRC})

if (ENABLE_SESHAT)
set(SOURCES ${SOURCES} ../src/seshat_interface.c)
else()
set(SOURCES ${SOURCES} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
 
add_executable(test_client_list ${SOURCES})
target_link_libraries (test_client_list ${PARODUS_COMMON_LIBS})
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_client_list ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_service_alive
#-------------------------------------------------------------------------------
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
add_test(NAME test_service_alive COMMAND ./test_service_alive)
else()
add_test(NAME test_service_alive COMMAND ${MEMORY_CHECK} ./test_service_alive)
endif()
set(SOURCES test_service_alive.c ../src/client_list.c ../src/service_alive.c ../src/upstream.c ../src/networking.c ../src/nopoll_helpers.c ../src/nopoll_handlers.c ../src/config.c ../src/connection.c ../src/ParodusInternal.c ../src/downstream.c ../src/thread_tasks.c ../src/conn_interface.c ../src/partners_check.c ${PARODUS_COMMON_SRC})
if (ENABLE_SESHAT)
set(SOURCES ${SOURCES} ../src/seshat_interface.c)
else()
set(SOURCES ${SOURCES} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)

add_executable(test_service_alive ${SOURCES})
target_link_libraries (test_service_alive ${PARODUS_COMMON_LIBS})
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_service_alive ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_config
#-------------------------------------------------------------------------------
add_test(NAME test_config COMMAND ${MEMORY_CHECK} ./test_config)
add_executable(test_config test_config.c ../src/config.c ../src/string_helpers.c)
target_link_libraries (test_config -lcmocka ${PARODUS_COMMON_LIBS})
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_config ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_upstream
#-------------------------------------------------------------------------------
add_test(NAME test_upstream COMMAND ${MEMORY_CHECK} ./test_upstream)
add_executable(test_upstream test_upstream.c ../src/upstream.c ../src/string_helpers.c)
target_link_libraries (test_upstream -lcmocka ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_upstream ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_downstream
#-------------------------------------------------------------------------------
add_test(NAME test_downstream COMMAND ${MEMORY_CHECK} ./test_downstream)
add_executable(test_downstream test_downstream.c ../src/downstream.c ../src/string_helpers.c)
target_link_libraries (test_downstream -lcmocka ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_downstream ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_downstream_more
#-------------------------------------------------------------------------------
add_test(NAME test_downstream_more COMMAND ${MEMORY_CHECK} ./test_downstream_more)
add_executable(test_downstream_more test_downstream_more.c ../src/downstream.c ../src/string_helpers.c)
target_link_libraries (test_downstream_more -lcmocka ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_downstream_more ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_thread_tasks
#-------------------------------------------------------------------------------
add_test(NAME test_thread_tasks COMMAND ${MEMORY_CHECK} ./test_thread_tasks)
add_executable(test_thread_tasks test_thread_tasks.c ../src/thread_tasks.c)
target_link_libraries (test_thread_tasks -lcmocka ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_thread_tasks ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_conn_interface
#-------------------------------------------------------------------------------
add_test(NAME test_conn_interface COMMAND ${MEMORY_CHECK} ./test_conn_interface)
set(SOURCES test_conn_interface.c ../src/conn_interface.c ../src/config.c ../src/string_helpers.c ../src/mutex.c)
if (ENABLE_SESHAT)
set(SOURCES ${SOURCES} ../src/seshat_interface.c)
else()
set(SOURCES ${SOURCES} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
add_executable(test_conn_interface ${SOURCES})
target_link_libraries (test_conn_interface -lcmocka ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_conn_interface ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_ParodusInternal
#-------------------------------------------------------------------------------
add_test(NAME test_ParodusInternal COMMAND ${MEMORY_CHECK} ./test_ParodusInternal)
add_executable(test_ParodusInternal test_ParodusInternal.c ../src/ParodusInternal.c ../src/config.c ../src/string_helpers.c)
target_link_libraries (test_ParodusInternal -lcmocka ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_ParodusInternal ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_partners_check
#-------------------------------------------------------------------------------
add_test(NAME test_partners_check COMMAND ${MEMORY_CHECK} ./test_partners_check)
add_executable(test_partners_check test_partners_check.c ../src/partners_check.c ../src/string_helpers.c)
target_link_libraries (test_partners_check -lcmocka ${PARODUS_COMMON_LIBS} -lwrp-c)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_partners_check ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_token - token.c tests
#-------------------------------------------------------------------------------
add_test(NAME test_token COMMAND ${MEMORY_CHECK} ./test_token)
add_executable(test_token ${SOURCES} ) 
target_link_libraries (test_token ${PARODUS_COMMON_LIBS} -lcmocka )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_token ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   test_seshat_interface - registerWithSeshat
#-------------------------------------------------------------------------------
add_test(NAME test_seshat_interface COMMAND ${MEMORY_CHECK} ./test_seshat_interface)
if (ENABLE_SESHAT)
set(SOURCES test_seshat_interface.c ../src/seshat_interface.c ../src/string_helpers.c)
else()
set(SOURCES test_seshat_interface_stub.c ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
add_executable(test_seshat_interface ${SOURCES})
target_link_libraries (test_seshat_interface -lcmocka ${PARODUS_COMMON_LIBS} -lwrp-c)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (test_seshat_interface ${PARODUS_LINUX_ONLY_LIBS})
endif()

if (INTEGRATION_TESTING)
#-------------------------------------------------------------------------------
#   simple_connection test
#-------------------------------------------------------------------------------
add_test(NAME simple_connection COMMAND ${MEMORY_CHECK} ./simple_connection)
set(SOURCES simple_connection.c ${PARODUS_COMMON_SRC} ../src/upstream.c ../src/conn_interface.c
 ../src/thread_tasks.c ../src/downstream.c ../src/networking.c ../src/nopoll_helpers.c ../src/nopoll_handlers.c ../src/connection.c ../src/ParodusInternal.c ../src/client_list.c ../src/partners_check.c ../src/service_alive.c)
if (ENABLE_SESHAT)
set(SOURCES ${SOURCES} ../src/seshat_interface.c)
else()
set(SOURCES ${SOURCES} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
add_executable(simple_connection ${SOURCES}) 
target_link_libraries (simple_connection ${PARODUS_CONN_LIBS} ${PARODUS_COMMON_LIBS} )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (simple_connection ${PARODUS_LINUX_ONLY_LIBS})
endif()

#-------------------------------------------------------------------------------
#   simple test
#-------------------------------------------------------------------------------
add_test(NAME simple COMMAND ${MEMORY_CHECK} ./simple)
set(SOURCES simple.c ../src/upstream.c ../src/conn_interface.c ../src/downstream.c ../src/thread_tasks.c ../src/networking.c ../src/nopoll_helpers.c ../src/nopoll_handlers.c ../src/string_helpers.c ../src/mutex.c ../src/time.c
 ../src/config.c ../src/connection.c ../src/ParodusInternal.c ../src/spin_thread.c ../src/client_list.c ../src/partners_check.c ../src/service_alive.c)
if (ENABLE_SESHAT)
set(SOURCES ${SOURCES} ../src/seshat_interface.c)
else()
set(SOURCES ${SOURCES} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
add_executable(simple ${SOURCES}) 

target_link_libraries (simple ${PARODUS_CONN_LIBS} ${PARODUS_COMMON_LIBS}  -lnopoll -lnanomsg )
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries (simple ${PARODUS_LINUX_ONLY_LIBS})
endif()

endif (INTEGRATION_TESTING)
