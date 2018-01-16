# parodus

C implementation of the WebPA client coordinator

[![Build Status](https://travis-ci.org/Comcast/parodus.svg?branch=master)](https://travis-ci.org/Comcast/parodus)
[![codecov.io](http://codecov.io/github/Comcast/parodus/coverage.svg?branch=master)](http://codecov.io/github/Comcast/parodus?branch=master)
[![Coverity](https://img.shields.io/coverity/scan/11192.svg)](https://scan.coverity.com/projects/comcast-parodus)
[![Apache V2 License](http://img.shields.io/badge/license-Apache%20V2-blue.svg)](https://github.com/Comcast/parodus/blob/master/LICENSE)
# Building and Testing Instructions

```
mkdir build
cd build
cmake ..
make
make test
```
# Command line Arguments needed to start parodus

```
- /hw-model -The hardware model name.

- /hw-serial-number -The serial number.

- /hw-manufacturer -The device manufacturer.

- /hw-mac -The MAC address used to manage the device.

- /hw-last-reboot-reason -The last known reboot reason.

- /fw-name -The firmware name.

- /boot-time -The boot time in unix time.

- /webpa-url -The URL that the WRP client should reach out to.

- /webpa-backoff-max -The maximum value of c in the binary backoff algorithm

- /webpa-ping-time -The maximum time to wait between pings before assuming the connection is broken.

- /webpa-interface-used -The device interface being used to connect to the cloud.

- /parodus-local-url -Parodus local server url (optional argument)

- /partner-id -  Partner ID of broadband gateway (optional argument)

- /ssl-cert-path -Provide the certs for establishing secure connection (optional argument)

- /force-ipv4 -Forcefully connect parodus to ipv4 address (optional argument)

- /force-ipv6 -Forcefully connect parodus to ipv6 address (optional argument)

- /webpa-token -Application to get webpa token (absolute path where that application is present) -optional argument 

- /secure-flag - secure flag to switch between http or https (optional argument)

- /port - Parodus local server port (optional argument)


# if ENABLE_SESHAT is enabled
- /seshat-url - The seshat server url 

# if ENABLE_CJWT is enabled then below mentioned arguments are needed

- /dns-id  - this parameter is used along with the hw_mac parameter to create the dns txt record id (i.e. 'fabric' or 'test')

- /jwt-algo -Allowed algorithm used for communication

- /jwt-key -JWT token validation key

```

# Sample parodus start commands:

```
# Seshat & CJWT Enabled

sudo ./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-time=180 --webpa-interface-used=eth0 --webpa-url=fabric.webpa.comcast.net --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --dns-id=fabric --jwt-key=webpa-rs256.pem --jwt-algo=none:RS256 --seshat-url=tcp://127.0.0.1:7777 --webpa-token=/usr/ccsp/parodus/parodus_token.sh --force-ipv4 --secure-flag=https --port=8080

# Seshat is not enabled

sudo ./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-time=180 --webpa-interface-used=eth0 --webpa-url=fabric.webpa.comcast.net --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --dns-id=fabric --jwt-key=webpa-rs256.pem --jwt-algo=none:RS256 --webpa-token=/usr/ccsp/parodus/parodus_token.sh --force-ipv4 --secure-flag=https --port=8080

# When both Seshat & CJWT not Enabled

sudo ./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-time=180 --webpa-interface-used=eth0 --webpa-url=fabric.webpa.comcast.net --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --webpa-token=/usr/ccsp/parodus/parodus_token.sh --force-ipv4 --secure-flag=https --port=8080

```

