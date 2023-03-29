#!/bin/sh
parodus_port=16014d

if [[ -z "${URL}" ]]; then
    URL="http://petasos:6400"
fi

if [[ -z "${FIRMWARE}" ]]; then
    FIRMWARE="mock-rdkb-firmware"
fi

if [[ -z "${BOOT_TIME}" ]]; then
    BOOT_TIME=$(date +%s)
fi

if [[ -z "${HW_MANUFACTURER}" ]]; then
    HW_MANUFACTURER="Example Inc."
fi

if [[ -z "${REBOOT_REASON}" ]]; then
    REBOOT_REASON="unknown"
fi

if [[ -z "${SERIAL_NUMBER}" ]]; then
    SERIAL_NUMBER="mock-rdkb-simulator"
fi

if [[ -z "${PARTNER_ID}" ]]; then
    PARTNER_ID="comcast"
fi

if [[ -z "${CMAC}" ]]; then
    CMAC="112233445566"
fi

if [[ -z "${TOKEN_SERVER_URL}" ]]; then
    TOKEN_SERVER_URL="http://themis:6501/issue"
fi

if [[ -z "${SSL_CERT_PATH}" ]]; then
    SSL_CERT_PATH="/etc/ssl/certs/ca-certificates.crt"
fi

#In this docker-compose cluster, themis has mtls disabled so
#feel free to ignore the --client-cert-path flag value
#it is required by parodus to fetch a token
if [[ -z "${CLIENT_CERT_PATH}" ]]; then
    CLIENT_CERT_PATH="/etc/ssl/certs/ca-certificates.crt"
fi

# MTLS_CLIENT_* is used to authenticate with talaria.
if [[ -z "${MTLS_CLIENT_CERT_PATH}" ]]; then
    MTLS_CLIENT_CERT_PATH=""
fi

if [[ -z "${MTLS_CLIENT_KEY_PATH}" ]]; then
    MTLS_CLIENT_KEY_PATH=""
fi

build/src/parodus --hw-model=aker-testing \
    --ssl-cert-path=$SSL_CERT_PATH \
    --client-cert-path=$CLIENT_CERT_PATH \
    --mtls-client-cert-path=$MTLS_CLIENT_CERT_PATH \
    --mtls-client-key-path=$MTLS_CLIENT_KEY_PATH \
    --hw-serial-number=$SERIAL_NUMBER \
    --hw-manufacturer=$HW_MANUFACTURER \
    --hw-mac=$CMAC \
    --hw-last-reboot-reason=$REBOOT_REASON \
    --fw-name=$FIRMWARE \
    --boot-time=$BOOT_TIME \
    --partner-id=$PARTNER_ID \
    --parodus-local-url=tcp://127.0.0.1:$parodus_port \
    --webpa-ping-timeout=60 \
    --token-server-url=$TOKEN_SERVER_URL \
    --webpa-backoff-max=2 \
    --webpa-interface-used=eth0 \
    --webpa-url=$URL \
    --force-ipv4 