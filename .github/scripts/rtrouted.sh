#!/bin/bash
export RBUS_ROOT=${HOME}/rbus
export RBUS_INSTALL_DIR=${RBUS_ROOT}/install
export RBUS_BRANCH=main
mkdir -p $RBUS_INSTALL_DIR
cd $RBUS_ROOT
git clone https://github.com/rdkcentral/rbus
cmake -Hrbus -Bbuild/rbus -DCMAKE_INSTALL_PREFIX=${RBUS_INSTALL_DIR}/usr -DBUILD_FOR_DESKTOP=ON -DCMAKE_BUILD_TYPE=Debug
make -C build/rbus && make -C build/rbus install
export PATH=${RBUS_INSTALL_DIR}/usr/bin:${PATH} && \
export LD_LIBRARY_PATH=${RBUS_INSTALL_DIR}/usr/lib:${LD_LIBRARY_PATH}
nohup rtrouted -f -l DEBUG > /tmp/rtrouted_log.txt &
mkdir ${RBUS_INSTALL_DIR}/usr/lib/rbus_temp_lib
cp ${RBUS_INSTALL_DIR}/usr/lib/librbuscore.so* ${RBUS_INSTALL_DIR}/usr/lib/rbus_temp_lib/
cp ${RBUS_INSTALL_DIR}/usr/lib/librtMessage.so* ${RBUS_INSTALL_DIR}/usr/lib/rbus_temp_lib/
cp ${RBUS_INSTALL_DIR}/usr/lib/libcjson.so ${RBUS_INSTALL_DIR}/usr/lib/rbus_temp_lib/
