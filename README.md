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

- /webpa-url -The URL that the WRP client should reach out to. (required). Must begin with 'https://' or 'http://'.  May end with a port number. If no port specified, then 443 is assumed for https, 80 for http.

- /webpa-backoff-max -The maximum value of c in the binary backoff algorithm

- /webpa-ping-timeout -The maximum time to wait between pings before assuming the connection is broken.

- /webpa-interface-used -The device interface being used to connect to the cloud.

- /parodus-local-url -Parodus local server url (optional argument)

- /partner-id -  Partner ID of broadband gateway (optional argument)

- /ssl-cert-path -Provide the certs for establishing secure connection (optional argument)

- /force-ipv4 -Forcefully connect parodus to ipv4 address (optional argument)

- /force-ipv6 -Forcefully connect parodus to ipv6 address (optional argument)

- /token-read-script -Script to get webpa auth token for establishing secure connection (absolute path where that script is present) -optional argument 

- /token-acquisition-script -Script to create new auth token for establishing secure connection (absolute path where that script is present) -optional argument 


# if ENABLE_SESHAT is enabled
- /seshat-url - The seshat server url 

# if FEATURE_DNS_QUERY is enabled then below mentioned arguments are needed

- /acquire-jwt - this parameter (0 or 1) specifies whether there will be a dns lookup. If not, or if any problem occurs with the dns lookup, then webpa-url will be the target. 

- /dns-txt-url  - this parameter is used along with the hw_mac parameter to create the dns txt record id

- /jwt-algo -Allowed algorithm used for communication

- /jwt-public-key-file -JWT token validation key

```

# Sample parodus start commands:

```
# Seshat & FEATURE_DNS_QUERY Enabled

./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=eth0 --webpa-url=somebody.net:8080 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --acquire-jwt=1 --dns-txt-url=somebody.net --jwt-public-key-file=webpa-rs256.pem --jwt-algo=RS256 --seshat-url=tcp://127.0.0.1:7777 --token-read-script=/usr/ccsp/parodus/parodus_token1.sh --token-acquisition-script=/usr/ccsp/parodus/parodus_token2.sh --force-ipv4


# Seshat is not enabled

./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=eth0 --webpa-url=somebody.net:8080 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --acquire-jwt=1 --dns-txt-url=somebody.net --jwt-public-key-file=webpa-rs256.pem --jwt-algo=RS256 --token-read-script=/usr/ccsp/parodus/parodus_token1.sh --token-acquisition-script=/usr/ccsp/parodus/parodus_token2.sh --force-ipv4


# When both Seshat & FEATURE_DNS_QUERY not Enabled

./parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=14cfexxxxxxx --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=eth0 --webpa-url=somebody.net:8080 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-certificates.crt --token-read-script=/usr/ccsp/parodus/parodus_token1.sh --token-acquisition-script=/usr/ccsp/parodus/parodus_token2.sh --force-ipv4

```

# Sample start commands for parodus to parodus demo:

1. Hub parodus
```
sudo ./src/parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=666666666666 --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=enp1s0 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:6666 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-bundle.crt --webpa-url=https://fabric.webpa.comcast.net:8080 --force-ipv4 --hub-or-spoke=hub
```

2. Printer component connecting to Hub
```
./tests/printer -p tcp://127.0.0.1:6666 -c tcp://127.0.0.1:9001 -t 10000
```

3. Spoke1 parodus
```
sudo ./src/parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=888888888888 --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=enp1s0 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:8888 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-bundle.crt --webpa-url=https://fabric.webpa.comcast.net:8080 --force-ipv4 --hub-or-spoke=spk
```

4. Printer1 component connecting to Spoke
```
./tests/printer -p tcp://127.0.0.1:8888 -c tcp://127.0.0.1:8001 -t 10000
```

5. Spoke2 parodus
```
sudo ./src/parodus --hw-model=TGXXX --hw-serial-number=E8GBUEXXXXXXXXX --hw-manufacturer=ARRIS --hw-mac=999999999999 --hw-last-reboot-reason=unknown --fw-name=TG1682_DEV_master_20170512115046sdy --boot-time=1494590301 --webpa-ping-timeout=180 --webpa-interface-used=enp1s0 --webpa-backoff-max=9 --parodus-local-url=tcp://127.0.0.1:9999 --partner-id=comcast --ssl-cert-path=/etc/ssl/certs/ca-bundle.crt --webpa-url=https://fabric.webpa.comcast.net:8080 --force-ipv4 --hub-or-spoke=spk
```

6. Printer2 component connecting to Spoke
```
./tests/printer -p tcp://127.0.0.1:9999 -c tcp://127.0.0.1:9002 -t 10000
```

7. Producer connecting to Hub
```
./tests/producer -p tcp://127.0.0.1:6666 -c tcp://127.0.0.1:9010 -t 5 -m 666666666666
```

8. Producer1 connecting to Spoke1
```
./tests/producer -p tcp://127.0.0.1:8888 -c tcp://127.0.0.1:9012 -t 5 -m 888888888888
```

9. Producer2 connecting to Spoke2
```
./tests/producer -p tcp://127.0.0.1:9999 -c tcp://127.0.0.1:9011 -t 5 -m 999999999999
```
