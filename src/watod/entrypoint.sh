#!/bin/sh
set -e

# Trust Caddy's internal CA certificate if available
CA_CERT="/caddy_data/caddy/pki/authorities/local/root.crt"
if [ -f "$CA_CERT" ]; then
    cp "$CA_CERT" /usr/local/share/ca-certificates/caddy-local.crt
    update-ca-certificates
fi

exec ./watod "$@"
