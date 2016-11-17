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
set(SOURCES main.c wss_mgr.c mutex.c networking.c nopoll_helpers.c string_helpers.c)

add_executable(parodus ${SOURCES})

target_link_libraries (parodus
    ${CMAKE_THREAD_LIBS_INIT}
    -lwrp-c
    -lmsgpackc
    -ltrower-base64
    -lnopoll
    -lm
    -lssl
    -lcrypto
    -lnanomsg
    -lcjson
    -lpthread
    -lrt
    )

