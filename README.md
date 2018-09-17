# parodus

[![Build Status](https://travis-ci.org/Comcast/parodus.svg?branch=master)](https://travis-ci.org/Comcast/parodus)
[![codecov.io](http://codecov.io/github/Comcast/parodus/coverage.svg?branch=master)](http://codecov.io/github/Comcast/parodus?branch=master)
[![Coverity](https://img.shields.io/coverity/scan/11192.svg)](https://scan.coverity.com/projects/comcast-parodus)
[![Apache V2 License](http://img.shields.io/badge/license-Apache%20V2-blue.svg)](https://github.com/Comcast/parodus/blob/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/Comcast/parodus.svg)](CHANGELOG.md)


C implementation of the XMiDT client coordinator

XMiDT is a highly scalable, highly available, generic message routing system.

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

- /webpa-url -The URL that the WRP client should reach out to. (required). Must begin with 'https://' or 'http://'.  May end with a port number. If no port specified, then 443 is assumed for https, 80 for http.

- /webpa-backoff-max -The maximum value of c in the binary backoff algorithm

- /webpa-ping-timeout -The maximum time to wait between pings before assuming the connection is broken.

- /webpa-interface-used -The device interface being used to connect to the cloud.

- /parodus-local-url -Parodus local server url (optional argument)

- /partner-id -  Partner ID of broadband gateway (optional argument)

- /ssl-cert-path -Provide the certs for establishing secure connection (optional argument)

- /force-ipv4 -Forcefully connect parodus to ipv4 address (optional argument)

- /force-ipv6 -Forcefully connect parodus to ipv6 address (optional argument)

- /token-read-script -Script to get auth token for establishing secure connection (absolute path where that script is present) -optional argument

- /token-acquisition-script -Script to create new auth token for establishing secure connection (absolute path where that script is present) -optional argument 

- /crud-config-file -Config json file to store objects during create, retrieve, update and delete (CRUD) operations -optional argument 


# if ENABLE_SESHAT is enabled
- /seshat-url - The seshat server url 

# if FEATURE_DNS_QUERY is enabled then below mentioned arguments are needed

- /acquire-jwt - this parameter (0 or 1) specifies whether there will be a dns lookup. If not, or if any problem occurs with the dns lookup, then webpa-url will be the target. 

- /dns-txt-url  - this parameter is used along with the hw_mac parameter to create the dns txt record id

- /jwt-algo -Allowed algorithm used for communication

- /jwt-public-key-file -JWT token validation key

# if ENABLE_MUTUAL_AUTH is enabled

- /client-cert-path - Provide the client cert for establishing a mutual auth secure connection

- /client-key-path - Provide the client cert key for establishing a mutual auth secure connection

```

# Sample parodus start commands:

```
# Seshat & FEATURE_DNS_QUERY Enabled

./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=eth0 --webpa-url=somebody.net:8080 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --acquire-jwt=1 --dns-txt-url=somebody.net --jwt-public-key-file=webpa-rs256.pem --jwt-algo=RS256 --seshat-url=tcp://127.0.0.1:7777 --token-read-script=/usr/ccsp/parodus/parodus_token1.sh --token-acquisition-script=/usr/ccsp/parodus/parodus_token2.sh --force-ipv4 --crud-config-file=/tmp/parodus_cfg.json


# Seshat is not enabled

./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=eth0 --webpa-url=somebody.net:8080 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --acquire-jwt=1 --dns-txt-url=somebody.net --jwt-public-key-file=webpa-rs256.pem --jwt-algo=RS256 --token-read-script=/usr/ccsp/parodus/parodus_token1.sh --token-acquisition-script=/usr/ccsp/parodus/parodus_token2.sh --force-ipv4 --crud-config-file=/tmp/parodus_cfg.json


# When both Seshat & FEATURE_DNS_QUERY not Enabled

./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=eth0 --webpa-url=somebody.net:8080 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --token-read-script=/usr/ccsp/parodus/parodus_token1.sh --token-acquisition-script=/usr/ccsp/parodus/parodus_token2.sh --force-ipv4 --crud-config-file=/tmp/parodus_cfg.json

```

