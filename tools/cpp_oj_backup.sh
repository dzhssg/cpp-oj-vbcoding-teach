#!/bin/bash
set -euo pipefail

DB_HOST="127.0.0.1"
DB_PORT="3306"
DB_USER="root"
DB_PASS=""
DB_NAME="cpp_oj"

BACKUP_DIR="/home/dzh/backups/cpp_oj"
RETENTION_DAYS=14

mkdir -p "$BACKUP_DIR"

TS=$(date +%F_%H%M%S)
OUT="$BACKUP_DIR/cpp_oj_${TS}.sql.gz"

if [ -n "$DB_PASS" ]; then
    AUTH="-p${DB_PASS}"
else
    AUTH=""
fi

mysqldump -u"$DB_USER" -h"$DB_HOST" -P"$DB_PORT" $AUTH \
    --single-transaction --routines --triggers --events \
    --databases "$DB_NAME" | gzip > "$OUT"

echo "[$(date '+%F %T')] backup done: $OUT ($(du -h "$OUT" | cut -f1))"

find "$BACKUP_DIR" -name 'cpp_oj_*.sql.gz' -mtime +${RETENTION_DAYS} -delete
