#! /bin/bash

DEST="/usr/local/share/ca-certificates/caddy-local.crt"

docker compose exec caddy cat \
    /data/caddy/pki/authorities/local/root.crt | \
    sudo tee $DEST \
  && sudo update-ca-certificates
