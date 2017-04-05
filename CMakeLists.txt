#   Copyright 2010-2016 Comcast Cable Communications Management, LLC
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

cmake_minimum_required(VERSION 2.8.7)

project(parodus)

include(ExternalProject)
include(CTest)

set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/_install)
set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR}/_prefix)
set(INCLUDE_DIR ${INSTALL_DIR}/include)
set(LIBRARY_DIR ${INSTALL_DIR}/lib)
set(LIBRARY_DIR64 ${INSTALL_DIR}/lib64)
set(TEST_RESULTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_results)
file(MAKE_DIRECTORY ${TEST_RESULTS_DIR})

include_directories(${INCLUDE_DIR}
                    ${INCLUDE_DIR}/cjson
                    ${INCLUDE_DIR}/nopoll
                    ${INCLUDE_DIR}/msgpack
                    ${INCLUDE_DIR}/trower-base64
                    ${INCLUDE_DIR}/wrp-c                   
                    ${INCLUDE_DIR}/libparodus
                    ${INCLUDE_DIR}/cimplog
                    ${INCLUDE_DIR}/libseshat
                   )

# Compile options/flags
#-------------------------------------------------------------------------------
add_definitions(-std=c99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -DNOPOLL_LOGGER ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")


# pthread external dependency
#-------------------------------------------------------------------------------
find_package (Threads)

if (NOT BUILD_YOCTO)

# base64 external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(trower-base64
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/trower-base64
    GIT_REPOSITORY https://github.com/Comcast/trower-base64.git
    GIT_TAG "master"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libtrower-base64 STATIC SHARED IMPORTED)
add_dependencies(libtrower-base64 trower-base64)


# nopoll external dependency
#-------------------------------------------------------------------------------
set(PATCHES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/patches)
set(NOPOLL_LOG_SRC ${PREFIX_DIR}/nopoll/src/nopoll/src/nopoll_log.c)
ExternalProject_Add(nopoll
    PREFIX ${PREFIX_DIR}/nopoll
    GIT_REPOSITORY https://github.com/ASPLes/nopoll.git
    GIT_TAG "b18aacc06b4dc9700e0a261efc201a8e125e4328"
    PATCH_COMMAND patch -p1 < ${PATCHES_DIR}/nopoll.patch
    COMMAND touch NEWS README AUTHORS ChangeLog
    COMMAND libtoolize --force
    COMMAND aclocal
    COMMAND autoheader --warnings=error
    COMMAND automake --add-missing -Werror
    COMMAND autoconf --force --warnings=error
    CONFIGURE_COMMAND COMMAND <SOURCE_DIR>/configure --prefix=${PREFIX}
                                                     --includedir=${INCLUDE_DIR}
                                                     --libdir=${LIBRARY_DIR}
                                                     ${CUSTOM_HOST}
    BUILD_IN_SOURCE 1
)
add_library(libnopoll STATIC SHARED IMPORTED)
add_dependencies(libnopoll nopoll)


# nanoMsg external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(nanomsg
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/nanomsg
    GIT_REPOSITORY https://github.com/nanomsg/nanomsg.git
    #GIT_TAG "096998834451219ee7813d8977f6a4027b0ccb43"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libnanomsg STATIC SHARED IMPORTED)
add_dependencies(libnanomsg nanomsg)


# cJSON external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(cJSON
    #PREFIX ${PREFIX_DIR}/cJSON
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/cJSON
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG "aafb64a1c549b7b927e339df6d35b1d5059dc235"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libcJSON STATIC SHARED IMPORTED)
add_dependencies(libcJSON cJSON)


# msgpack-c external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(msgpack
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/msgpack
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG "c6e6dbc608366090c12b142b3832604e6aa12f54"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
                  -DMSGPACK_ENABLE_CXX=OFF
                  -DMSGPACK_BUILD_EXAMPLES=OFF
)
add_library(libmsgpack STATIC SHARED IMPORTED)
add_dependencies(libmsgpack msgpack)


# cimplog external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(cimplog
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/cimplog
    GIT_REPOSITORY https://github.com/Comcast/cimplog.git
    GIT_TAG "master"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libcimplog STATIC SHARED IMPORTED)
add_dependencies(libcimplog cimplog)


# wrp-c external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(wrp-c
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/wrp-c
    GIT_REPOSITORY https://github.com/Comcast/wrp-c.git
    GIT_TAG "master"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
                  -DMSGPACK_ENABLE_CXX=OFF
                  -DMSGPACK_BUILD_EXAMPLES=OFF
                  -DBUILD_TESTING=OFF
)
add_library(libwrp-c STATIC SHARED IMPORTED)
add_dependencies(libwrp-c wrp-c)

# libparodus external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(libparodus
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/libparodus
    GIT_REPOSITORY https://github.com/Comcast/libparodus.git
    GIT_TAG "master"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(liblibparodus STATIC SHARED IMPORTED)
add_dependencies(liblibparodus libparodus)

# seshat external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(seshat
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/seshat
    GIT_REPOSITORY https://github.com/comcast/seshat.git
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(liblibseshat STATIC SHARED IMPORTED)
add_dependencies(liblibseshat seshat)

if (BUILD_TESTING)
# cmocka external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(cmocka
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/cmocka
    GIT_REPOSITORY https://git.cryptomilk.org/projects/cmocka.git
    GIT_TAG "master"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libcmocka STATIC SHARED IMPORTED)
add_dependencies(libcmocka cmocka)
endif (BUILD_TESTING)

endif ()

link_directories ( ${LIBRARY_DIR} ${LIBRARY_DIR64})

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif (BUILD_TESTING)
