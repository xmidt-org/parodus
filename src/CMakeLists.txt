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
set(SOURCES main.c mutex.c networking.c nopoll_helpers.c heartBeat.c nopoll_handlers.c
    ParodusInternal.c string_helpers.c time.c config.c conn_interface.c
    connection.c spin_thread.c client_list.c service_alive.c
    upstream.c downstream.c thread_tasks.c partners_check.c token.c event_handler.c
	crud_interface.c crud_tasks.c crud_internal.c close_retry.c auth_token.c privilege.c)

if (NOT BUILD_YOCTO)
set(SOURCES ${SOURCES} rdkconfig_generic.c)
endif (NOT BUILD_YOCTO)

if (ENABLE_SESHAT)
set(SOURCES ${SOURCES} seshat_interface.c)
else()
set(SOURCES ${SOURCES} seshat_interface_stub.c)
endif (ENABLE_SESHAT)

if (ENABLE_WEBCFGBIN)
set(SOURCES ${SOURCES} upstream_rbus.c xmidtsend_rbus.c)
endif (ENABLE_WEBCFGBIN)

if (WAN_FAILOVER_SUPPORTED)
message(STATUS "WAN_FAILOVER_SUPPORTED is supported")
else()
message(STATUS "WAN_FAILOVER_SUPPORTED is not supported")
endif (WAN_FAILOVER_SUPPORTED)

add_executable(parodus ${SOURCES})

target_link_libraries (parodus
    ${CMAKE_THREAD_LIBS_INIT}
    -lwrp-c
    -lmsgpackc
    -lcurl
    -ltrower-base64
    -lnopoll
    -luuid
    -lm
    -lcimplog
    -lssl
    -lcrypto
    -lnanomsg
    -lcjson
    -lcjwt
    -lpthread
    -lrt
    )

if (FEATURE_DNS_QUERY)
target_link_libraries (parodus -lucresolv -lresolv)
endif (FEATURE_DNS_QUERY)

if (ENABLE_SESHAT)
target_link_libraries (parodus -llibseshat)
endif (ENABLE_SESHAT)

if (ENABLE_WEBCFGBIN)
target_link_libraries (parodus -lrbus)
endif (ENABLE_WEBCFGBIN)

if (BUILD_YOCTO)
if (PARODUS_SECERT_ENABLE)
target_link_libraries (parodus -lrdkconfig -lsecure_wrapper)
endif (PARODUS_SECERT_ENABLE)
endif (BUILD_YOCTO)
install (TARGETS parodus DESTINATION bin)
