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
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")
set (PARODUS_COMMON_SRC ../src/string_helpers.c ../src/mutex.c 
  ../src/time.c ../src/config.c ../src/spin_thread.c 
  ../src/event_handler.c ../src/token.c)
set (PARODUS_COMMON_LIBS gcov -lcunit -lcimplog -lwrp-c
 -luuid -lmsgpackc -lnopoll -lnanomsg -lpthread
 -Wl,--no-as-needed -lcjson -lcjwt -ltrower-base64
 -lssl -lcrypto -lrt -lm)

if (ENABLE_SESHAT)
set (PARODUS_COMMON_LIBS -llibseshat ${PARODUS_COMMON_LIBS})
endif (ENABLE_SESHAT)

if (FEATURE_DNS_QUERY)
set (PARODUS_COMMON_LIBS ${PARODUS_COMMON_LIBS} -lucresolv -lresolv)
endif (FEATURE_DNS_QUERY)

if (ENABLE_WEBCFGBIN)
set (PARODUS_COMMON_LIBS ${PARODUS_COMMON_LIBS} -lrbus)
endif (ENABLE_WEBCFGBIN)

if(NOT DISABLE_VALGRIND)
set (MEMORY_CHECK valgrind --leak-check=full --show-reachable=yes -v)
endif ()

link_directories ( ${LIBRARY_DIR} )

#-------------------------------------------------------------------------------
#   test_heartBeatTimer
#-------------------------------------------------------------------------------
add_test(NAME test_heartBeatTimer COMMAND ${MEMORY_CHECK} ./test_heartBeatTimer)
add_executable(test_heartBeatTimer test_heartBeatTimer.c ../src/heartBeat.c ../src/time.c)
target_link_libraries (test_heartBeatTimer ${PARODUS_COMMON_LIBS} -lcmocka)

#-------------------------------------------------------------------------------
#   test_close_retry
#-------------------------------------------------------------------------------
add_test(NAME test_close_retry COMMAND ${MEMORY_CHECK} ./test_close_retry)
add_executable(test_close_retry test_close_retry.c ../src/close_retry.c)
target_link_libraries (test_close_retry ${PARODUS_COMMON_LIBS} -lcmocka)

#-------------------------------------------------------------------------------
#   test_mutex
#-------------------------------------------------------------------------------
add_test(NAME test_mutex COMMAND ${MEMORY_CHECK} ./test_mutex)
add_executable(test_mutex test_mutex.c ../src/mutex.c)
target_link_libraries (test_mutex gcov -lcunit -lcimplog -lwrp-c
 -luuid -lmsgpackc -lnopoll -lnanomsg -lpthread
 -Wl,--no-as-needed -lcjson -lcjwt -ltrower-base64
 -lssl -lcrypto -lrt -lm -lcmocka)

#-------------------------------------------------------------------------------
#   test_networking
#-------------------------------------------------------------------------------
add_test(NAME test_networking COMMAND ${MEMORY_CHECK} ./test_networking)
add_executable(test_networking test_networking.c ../src/networking.c)
target_link_libraries (test_networking ${PARODUS_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_nopoll_helpers
#-------------------------------------------------------------------------------
add_test(NAME test_nopoll_helpers COMMAND ${MEMORY_CHECK} ./test_nopoll_helpers)
add_executable(test_nopoll_helpers test_nopoll_helpers.c ../src/nopoll_helpers.c ../src/string_helpers.c ../src/config.c)
target_link_libraries (test_nopoll_helpers -Wl,--no-as-needed -lrt -lcmocka -lcimplog -lnopoll ${PARODUS_COMMON_LIBS})

#-------------------------------------------------------------------------------
#   test_time
#-------------------------------------------------------------------------------
add_test(NAME test_time COMMAND ${MEMORY_CHECK} ./test_time)
add_executable(test_time test_time.c ../src/time.c)
target_link_libraries (test_time ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_spin_thread error
#-------------------------------------------------------------------------------
add_test(NAME test_spin_thread_e COMMAND ${MEMORY_CHECK} ./test_spin_thread_e)
add_executable(test_spin_thread_e test_spin_thread_e.c ../src/spin_thread.c)
target_link_libraries (test_spin_thread_e ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_spin_thread success
#-------------------------------------------------------------------------------
# add_test(NAME test_spin_thread_s COMMAND ${MEMORY_CHECK} ./test_spin_thread_s)
# add_executable(test_spin_thread_s test_spin_thread_s.c ../src/spin_thread.c)
# target_link_libraries (test_spin_thread_s ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_string_helpers 
#-------------------------------------------------------------------------------
# add_test(NAME test_string_helpers COMMAND ${MEMORY_CHECK} ./test_string_helpers)
# add_executable(test_string_helpers test_string_helpers.c ../src/string_helpers.c)
# target_link_libraries (test_string_helpers ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_nopoll_handlers
#-------------------------------------------------------------------------------
add_test(NAME test_nopoll_handlers COMMAND ${MEMORY_CHECK} ./test_nopoll_handlers)
add_executable(test_nopoll_handlers test_nopoll_handlers.c ../src/nopoll_handlers.c ../src/heartBeat.c ../src/close_retry.c ../src/time.c)
target_link_libraries (test_nopoll_handlers -lnopoll -lcunit -lcimplog -Wl,--no-as-needed -lrt -lpthread -lm)


#-------------------------------------------------------------------------------
#   test_nopoll_handlers_fragment
#-------------------------------------------------------------------------------
add_test(NAME test_nopoll_handlers_fragment COMMAND ${MEMORY_CHECK} ./test_nopoll_handlers_fragment)
add_executable(test_nopoll_handlers_fragment test_nopoll_handlers_fragment.c ../src/nopoll_handlers.c ../src/heartBeat.c ../src/close_retry.c ../src/time.c)
target_link_libraries (test_nopoll_handlers_fragment -lnopoll -lcunit -lcimplog -Wl,--no-as-needed -lrt -lpthread -lm -lcmocka)

#-------------------------------------------------------------------------------
#   test_connection
#-------------------------------------------------------------------------------
add_test(NAME test_connection COMMAND ${MEMORY_CHECK} ./test_connection)
set (CONN_SRC ../src/connection.c 
  ../src/string_helpers.c ../src/mutex.c ../src/time.c 
  ../src/config.c ../src/auth_token.c ../src/spin_thread.c  ../src/heartBeat.c ../src/close_retry.c)
#set(CONN_SRC ../src/connection.c ${PARODUS_COMMON_SRC})
add_executable(test_connection test_connection.c ${CONN_SRC})
target_link_libraries (test_connection ${PARODUS_COMMON_LIBS} -lcmocka -lcurl -luuid)

#-------------------------------------------------------------------------------
#   test_connection - function createNopollConnection
#-------------------------------------------------------------------------------
#add_test(NAME test_createConnection COMMAND ${MEMORY_CHECK} ./test_createConnection)
#add_executable(test_createConnection test_createConnection.c ../src/connection.c ../src/string_helpers.c ../src/config.c) 
#target_link_libraries (test_createConnection ${PARODUS_COMMON_LIBS} -lcmocka)
#add_executable(test_createConnection test_createConnection.c ../src/connection.c ../src/string_helpers.c ../src/config.c ../src/heartBeat.c) 
#target_link_libraries (test_createConnection ${PARODUS_CONN_LIBS} ${PARODUS_COMMON_LIBS} -lcmocka )
#target_link_libraries (test_createConnection ${PARODUS_COMMON_LIBS} -lcmocka )

#-------------------------------------------------------------------------------
#   test_client_list
#-------------------------------------------------------------------------------
add_test(NAME test_client_list COMMAND ${MEMORY_CHECK} ./test_client_list)
#add_executable(test_client_list test_client_list.c ../src/client_list.c ../src/service_alive.c ../src/upstream.c ../src/networking.c ../src/nopoll_helpers.c ../src/downstream.c ../src/connection.c ../src/nopoll_handlers.c ../src/ParodusInternal.c ../src/thread_tasks.c ../src/conn_interface.c ../src/partners_check.c ${PARODUS_COMMON_SRC})
#target_link_libraries (test_client_list ${PARODUS_COMMON_LIBS})
set(CLIST_SRC test_client_list.c ../src/client_list.c ../src/auth_token.c
 ../src/service_alive.c ../src/upstream.c ../src/networking.c ../src/nopoll_helpers.c 
 ../src/downstream.c ../src/connection.c ../src/nopoll_handlers.c ../src/heartBeat.c ../src/close_retry.c
 ../src/ParodusInternal.c ../src/thread_tasks.c ../src/conn_interface.c 
 ../src/partners_check.c ../src/crud_interface.c ../src/crud_tasks.c ../src/crud_internal.c ${PARODUS_COMMON_SRC})

if (ENABLE_SESHAT)
set(CLIST_SRC ${CLIST_SRC} ../src/seshat_interface.c)
else()
set(CLIST_SRC ${CLIST_SRC} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)

if (ENABLE_WEBCFGBIN)
set(CLIST_SRC ${CLIST_SRC} ../src/upstream_rbus.c ../src/xmidtsend_rbus.c)
endif (ENABLE_WEBCFGBIN)

add_executable(test_client_list ${CLIST_SRC})
#target_link_libraries (test_client_list ${PARODUS_CONN_LIBS} ${PARODUS_COMMON_LIBS})
target_link_libraries (test_client_list ${PARODUS_COMMON_LIBS} -lcurl -luuid)

#-------------------------------------------------------------------------------
#   test_service_alive
#-------------------------------------------------------------------------------
add_test(NAME test_service_alive COMMAND ${MEMORY_CHECK} ./test_service_alive)
#add_executable(test_service_alive test_service_alive.c ../src/client_list.c ../src/service_alive.c ../src/upstream.c ../src/networking.c ../src/nopoll_helpers.c ../src/nopoll_handlers.c ../src/config.c ../src/connection.c ../src/ParodusInternal.c ../src/downstream.c ../src/thread_tasks.c ../src/conn_interface.c ../src/partners_check.c ${PARODUS_COMMON_SRC})
#target_link_libraries (test_service_alive ${PARODUS_COMMON_LIBS})
set(SVA_SRC test_service_alive.c ../src/client_list.c ../src/service_alive.c ../src/auth_token.c ../src/upstream.c ../src/networking.c ../src/nopoll_helpers.c ../src/nopoll_handlers.c ../src/config.c ../src/connection.c ../src/ParodusInternal.c ../src/downstream.c ../src/thread_tasks.c ../src/conn_interface.c ../src/partners_check.c ../src/heartBeat.c ../src/close_retry.c ${PARODUS_COMMON_SRC})
if (ENABLE_SESHAT)
set(SVA_SRC ${SVA_SRC} ../src/seshat_interface.c)
else()
set(SVA_SRC ${SVA_SRC} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)

if (ENABLE_WEBCFGBIN)
set(SVA_SRC ${SVA_SRC} ../src/upstream_rbus.c ../src/xmidtsend_rbus.c)
endif (ENABLE_WEBCFGBIN)

add_executable(test_service_alive ${SVA_SRC})
#target_link_libraries (test_service_alive ${PARODUS_CONN_LIBS} ${PARODUS_COMMON_LIBS})
target_link_libraries (test_service_alive ${PARODUS_COMMON_LIBS} -lcurl -luuid)

#-------------------------------------------------------------------------------
#   test_config
#-------------------------------------------------------------------------------
add_test(NAME test_config COMMAND ${MEMORY_CHECK} ./test_config)
add_executable(test_config test_config.c ../src/config.c ../src/auth_token.c ../src/string_helpers.c)
target_link_libraries (test_config -lcmocka
 -Wl,--no-as-needed -lcimplog
 -lcjson -lcjwt -ltrower-base64 -lssl -lcrypto -lrt -lm -lcurl -luuid
)

#-------------------------------------------------------------------------------
#   test_auth_token
#-------------------------------------------------------------------------------
add_test(NAME test_auth_token COMMAND ${MEMORY_CHECK} ./test_auth_token)
add_executable(test_auth_token test_auth_token.c ../src/config.c ../src/auth_token.c ../src/string_helpers.c)
target_link_libraries (test_auth_token -lcmocka
 -Wl,--no-as-needed -lcimplog
 -lcjson -lcjwt -ltrower-base64 -lssl -lcrypto -lrt -lm -lcurl -luuid
)

#-------------------------------------------------------------------------------
#   test_auth_token_more
#-------------------------------------------------------------------------------
add_test(NAME test_auth_token_more COMMAND ${MEMORY_CHECK} ./test_auth_token_more)
add_executable(test_auth_token_more test_auth_token_more.c ../src/config.c ../src/auth_token.c ../src/string_helpers.c ../src/config.c)
target_link_libraries (test_auth_token_more -lcmocka
 -Wl,--no-as-needed -lcimplog
 -lcjson -lcjwt -ltrower-base64 -lssl -lcrypto -lrt -lm -lcurl -luuid
)

#-------------------------------------------------------------------------------
#   test_crud_interface
#-------------------------------------------------------------------------------
add_test(NAME test_crud_interface COMMAND ${MEMORY_CHECK} ./test_crud_interface)
add_executable(test_crud_interface test_crud_interface.c ../src/crud_interface.c ../src/ParodusInternal.c)
target_link_libraries (test_crud_interface -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_crud_tasks
#-------------------------------------------------------------------------------
add_test(NAME test_crud_tasks COMMAND ${MEMORY_CHECK} ./test_crud_tasks)
add_executable(test_crud_tasks test_crud_tasks.c ../src/crud_tasks.c )
target_link_libraries (test_crud_tasks -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_crud_internal
#-------------------------------------------------------------------------------
add_test(NAME test_crud_internal COMMAND ${MEMORY_CHECK} ./test_crud_internal)
add_executable(test_crud_internal test_crud_internal.c ../src/config.c ../src/close_retry.c 
  ../src/ParodusInternal.c ../src/string_helpers.c ../src/crud_internal.c ../src/client_list.c)
target_link_libraries (test_crud_internal -lcmocka ${PARODUS_COMMON_LIBS} -lcurl -luuid)

#-------------------------------------------------------------------------------
#   test_upstream
#-------------------------------------------------------------------------------
add_test(NAME test_upstream COMMAND ${MEMORY_CHECK} ./test_upstream)
add_executable(test_upstream test_upstream.c ../src/upstream.c ../src/close_retry.c ../src/string_helpers.c)
target_link_libraries (test_upstream -lcmocka gcov -lcunit -lcimplog
 -lwrp-c -luuid -lpthread -lmsgpackc -lnopoll
 -Wl,--no-as-needed -lcjson -lcjwt -ltrower-base64
 -lssl -lcrypto -lrt -lm -lnanomsg)

#-------------------------------------------------------------------------------
#   test_upstream_sock
#-------------------------------------------------------------------------------
add_test(NAME test_upstream_sock COMMAND ${MEMORY_CHECK} ./test_upstream_sock)
add_executable(test_upstream_sock test_upstream_sock.c ../src/upstream.c 
  ../src/client_list.c ../src/close_retry.c ../src/string_helpers.c)
target_link_libraries (test_upstream_sock -lcmocka gcov -lcunit -lcimplog
 -lwrp-c -luuid -lpthread -lmsgpackc -lnopoll -lnanomsg
 -Wl,--no-as-needed -lcjson -lcjwt -ltrower-base64
 -lssl -lcrypto -lrt -lm)
 
#-------------------------------------------------------------------------------
#   test_upstream_rbus
#-------------------------------------------------------------------------------
if(ENABLE_WEBCFGBIN)
add_test(NAME test_upstream_rbus COMMAND ${MEMORY_CHECK} ./test_upstream_rbus)
add_executable(test_upstream_rbus test_upstream_rbus.c ../src/upstream_rbus.c)
target_link_libraries (test_upstream_rbus -lcmocka gcov -lcunit -lcimplog
 -lwrp-c -luuid -lpthread -lmsgpackc -lnopoll
 -Wl,--no-as-needed -lcjson -lcjwt -ltrower-base64
 -lssl -lcrypto -lrt -lm -lnanomsg -lrbus)
endif(ENABLE_WEBCFGBIN)


#-------------------------------------------------------------------------------
#   test_downstream
#-------------------------------------------------------------------------------
add_test(NAME test_downstream COMMAND ${MEMORY_CHECK} ./test_downstream)
add_executable(test_downstream test_downstream.c ../src/downstream.c ../src/string_helpers.c ../src/config.c)
target_link_libraries (test_downstream -lcmocka gcov -lcunit -lcimplog
 -lwrp-c -luuid -lpthread -lmsgpackc -lnopoll
 -Wl,--no-as-needed -lcjson -lcjwt -ltrower-base64
 -lssl -lcrypto -lrt -lm)

#-------------------------------------------------------------------------------
#   test_downstream_more
#-------------------------------------------------------------------------------
add_test(NAME test_downstream_more COMMAND ${MEMORY_CHECK} ./test_downstream_more)
add_executable(test_downstream_more test_downstream_more.c ../src/downstream.c ../src/string_helpers.c ../src/config.c)
target_link_libraries (test_downstream_more -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_thread_tasks
#-------------------------------------------------------------------------------
add_test(NAME test_thread_tasks COMMAND ${MEMORY_CHECK} ./test_thread_tasks)
add_executable(test_thread_tasks test_thread_tasks.c ../src/thread_tasks.c)
target_link_libraries (test_thread_tasks -lcmocka ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   test_conn_interface
#-------------------------------------------------------------------------------
add_test(NAME test_conn_interface COMMAND ${MEMORY_CHECK} ./test_conn_interface)
set(CONIFC_SRC test_conn_interface.c 
  ../src/crud_interface.c ../src/crud_tasks.c ../src/crud_internal.c 
  ../src/conn_interface.c 
  ../src/config.c 
  ../src/token.c
  ../src/string_helpers.c 
  ../src/mutex.c
  ../src/time.c
  ../src/heartBeat.c
  ../src/close_retry.c
  ../src/event_handler.c
  ../src/client_list.c
)
if (ENABLE_SESHAT)
set(CONIFC_SRC ${CONIFC_SRC} ../src/seshat_interface.c)
else()
set(CONIFC_SRC ${CONIFC_SRC} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
if (ENABLE_WEBCFGBIN)
set(CONIFC_SRC ${CONIFC_SRC} ../src/upstream_rbus.c ../src/xmidtsend_rbus.c)
endif (ENABLE_WEBCFGBIN)
add_executable(test_conn_interface ${CONIFC_SRC})
target_link_libraries (test_conn_interface -lcmocka ${PARODUS_COMMON_LIBS} -lcurl -luuid )

#-------------------------------------------------------------------------------
#   test_ParodusInternal
#-------------------------------------------------------------------------------
add_test(NAME test_ParodusInternal COMMAND ${MEMORY_CHECK} ./test_ParodusInternal)
add_executable(test_ParodusInternal test_ParodusInternal.c ../src/ParodusInternal.c ../src/config.c ../src/string_helpers.c)
target_link_libraries (test_ParodusInternal -lcmocka ${PARODUS_COMMON_LIBS} -lcurl -luuid)

#-------------------------------------------------------------------------------
#   test_partners_check
#-------------------------------------------------------------------------------
add_test(NAME test_partners_check COMMAND ${MEMORY_CHECK} ./test_partners_check)
add_executable(test_partners_check test_partners_check.c ../src/partners_check.c ../src/string_helpers.c)
target_link_libraries (test_partners_check -lcmocka ${PARODUS_COMMON_LIBS} -lwrp-c)

#-------------------------------------------------------------------------------
#   test_token - token.c tests
#-------------------------------------------------------------------------------
add_test(NAME test_token COMMAND ${MEMORY_CHECK} ./test_token)
set(TOKEN_SRC ../src/conn_interface.c ../src/config.c
 ../src/connection.c ../src/spin_thread.c
 ../src/service_alive.c ../src/client_list.c
 ../src/nopoll_handlers.c ../src/nopoll_helpers.c
 ../src/partners_check.c ../src/ParodusInternal.c
 ../src/upstream.c ../src/downstream.c 
 ../src/networking.c ../src/auth_token.c
 ../src/thread_tasks.c ../src/time.c
 ../src/string_helpers.c ../src/mutex.c 
 ../src/token.c ../src/heartBeat.c
 ../src/close_retry.c ../src/event_handler.c
 )

if (ENABLE_SESHAT)
set(TOKEN_SRC ${TOKEN_SRC} ../src/seshat_interface.c)
else()
set(TOKEN_SRC ${TOKEN_SRC} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)

if (FEATURE_DNS_QUERY)
set(TOKEN_SRC test_token.c ${TOKEN_SRC})
else()
set(TOKEN_SRC test_token_stub.c ${TOKEN_SRC})
endif (FEATURE_DNS_QUERY)

if (ENABLE_WEBCFGBIN)
set(TOKEN_SRC ${TOKEN_SRC} ../src/upstream_rbus.c ../src/xmidtsend_rbus.c)
endif (ENABLE_WEBCFGBIN)

add_executable(test_token ${TOKEN_SRC} ) 
#target_link_libraries (test_token ${PARODUS_COMMON_LIBS} ${PARODUS_JWT_LIBS} -lcmocka )
target_link_libraries (test_token ${PARODUS_COMMON_LIBS} -lcmocka -lcurl -luuid)

#-------------------------------------------------------------------------------
#   test_seshat_interface - registerWithSeshat
#-------------------------------------------------------------------------------
add_test(NAME test_seshat_interface COMMAND ${MEMORY_CHECK} ./test_seshat_interface)
if (ENABLE_SESHAT)
set(SESHIFC_SRC test_seshat_interface.c ../src/seshat_interface.c ../src/string_helpers.c)
else()
set(SESHIFC_SRC test_seshat_interface_stub.c ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
add_executable(test_seshat_interface ${SESHIFC_SRC})
target_link_libraries (test_seshat_interface -lcmocka ${PARODUS_COMMON_LIBS} -lwrp-c)

if (INTEGRATION_TESTING)
#-------------------------------------------------------------------------------
#   simple_connection test
#-------------------------------------------------------------------------------
add_test(NAME simple_connection COMMAND ${MEMORY_CHECK} ./simple_connection)
set(SIMCON_SRC simple_connection.c ${PARODUS_COMMON_SRC} ../src/upstream.c ../src/conn_interface.c
 ../src/thread_tasks.c ../src/downstream.c ../src/networking.c ../src/nopoll_helpers.c ../src/nopoll_handlers.c ../src/connection.c ../src/ParodusInternal.c ../src/client_list.c ../src/partners_check.c ../src/service_alive.c)
if (ENABLE_SESHAT)
set(SIMCON_SRC ${SIMCON_SRC} ../src/seshat_interface.c)
else()
set(SIMCON_SRC ${SIMCON_SRC} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
add_executable(simple_connection ${SIMCON_SRC}) 
target_link_libraries (simple_connection ${PARODUS_CONN_LIBS} ${PARODUS_COMMON_LIBS} )

#-------------------------------------------------------------------------------
#   simple test
#-------------------------------------------------------------------------------
add_test(NAME simple COMMAND ${MEMORY_CHECK} ./simple)
set(SIMPLE_SRC simple.c ../src/upstream.c ../src/conn_interface.c ../src/downstream.c ../src/thread_tasks.c ../src/networking.c ../src/nopoll_helpers.c ../src/nopoll_handlers.c ../src/string_helpers.c ../src/mutex.c ../src/time.c
 ../src/config.c ../src/connection.c ../src/ParodusInternal.c ../src/spin_thread.c ../src/client_list.c ../src/partners_check.c ../src/service_alive.c)
if (ENABLE_SESHAT)
set(SIMPLE_SRC ${SIMPLE_SRC} ../src/seshat_interface.c)
else()
set(SIMPLE_SRC ${SIMPLE_SRC} ../src/seshat_interface_stub.c)
endif (ENABLE_SESHAT)
add_executable(simple ${SIMPLE_SRC}) 

target_link_libraries (simple ${PARODUS_CONN_LIBS} ${PARODUS_COMMON_LIBS}  gcov -lnopoll )
endif (INTEGRATION_TESTING)
