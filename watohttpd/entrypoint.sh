#!/bin/sh
set -e

# Create default admin if credentials are provided
if [ -n "${ADMIN_EMAIL:-}" ] && [ -n "${ADMIN_PASSWORD:-}" ]; then
    ./watohttpd superuser upsert "$ADMIN_EMAIL" "$ADMIN_PASSWORD" 2>/dev/null || true
fi

./watohttpd migrate history-sync
exec ./watohttpd serve --dev --http=0.0.0.0:8090
