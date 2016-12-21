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

set(PROJ_PARODUS_LIB libparodus)

file(GLOB HEADERS libparodus.h)
set(SOURCES libparodus.c libparodus_log.c libparodus_time.c libparodus_queues.c)

add_library(${PROJ_PARODUS_LIB} STATIC ${HEADERS} ${SOURCES})
add_library(${PROJ_PARODUS_LIB}.shared SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJ_PARODUS_LIB}.shared PROPERTIES OUTPUT_NAME ${PROJ_PARODUS_LIB})

install (TARGETS ${PROJ_PARODUS_LIB} DESTINATION lib${LIB_SUFFIX})
install (TARGETS ${PROJ_PARODUS_LIB}.shared DESTINATION lib${LIB_SUFFIX})
install (FILES libparodus.h DESTINATION include/${PROJ_PARODUS_LIB})
