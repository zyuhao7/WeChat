#!/usr/bin/env bash
# Create local mock accounts so the app can be tested without a real mailbox.
#
# Each mock account: name mockN, email mockN@test.com, pwd 123456.
# Also seeds the Redis verify code (1234) for each email so the full
# /user_register endpoint can be exercised with verifycode=1234.
#
# The client obfuscates the password with xorString before sending, and the
# server stores/compares that value verbatim (no server-side decode). So the
# row must hold xorString("123456"), not the plaintext -- otherwise login
# fails with "email or password wrong" (1009).
#
# Existing accounts are reused (reg_user is idempotent per name/email).
#
# usage: scripts/mock_accounts.sh [count]      # default 5 -> mock1..mock5
set -euo pipefail

COUNT="${1:-5}"
MYSQL_HOST=127.0.0.1
MYSQL_PORT=3306
MYSQL_USER=root
MYSQL_PASS=123456
MYSQL_DB=db01
REDIS_PORT=6380
REDIS_PASS=123456
PWD_VALUE=123456
VERIFY_CODE=1234

mysql_cmd() {
  mysql -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" -p"$MYSQL_PASS" "$MYSQL_DB" "$@"
}
redis_cmd() {
  redis-cli -p "$REDIS_PORT" -a "$REDIS_PASS" --no-auth-warning "$@"
}

# mirror ChatClient/global.cpp xorString: xor_code = len % 255, each byte ^ code.
# emit hex so the value survives SQL as a binary literal (0x...).
xor_hex() {
  local s="$1"
  local len=${#s}
  local code=$(( len % 255 ))
  local out=""
  local i c v
  for ((i = 0; i < len; i++)); do
    c=${s:i:1}
    v=$(printf '%d' "'$c")
    out+=$(printf '%02x' "$(( v ^ code ))")
  done
  printf '%s' "$out"
}

PWD_HEX="$(xor_hex "$PWD_VALUE")"

for i in $(seq 1 "$COUNT"); do
  name="mock$i"
  email="mock$i@test.com"
  # cycle through the bundled avatar resources (ChatClient/res/head_*.jpg)
  head=":/res/head_$(( (i - 1) % 8 + 1 )).jpg"
  uid="$(mysql_cmd -N -e "CALL reg_user('$name','$email',0x$PWD_HEX,@r); SELECT uid FROM user WHERE email='$email';" | tail -n1)"
  # keep the stored password/avatar in sync even when the row already existed
  mysql_cmd -e "UPDATE user SET pwd = 0x$PWD_HEX, icon = '$head' WHERE email = '$email';"
  redis_cmd SET "code_$email" "$VERIFY_CODE" EX 600 >/dev/null
  echo "$name -> uid=$uid email=$email pwd=$PWD_VALUE verifycode=$VERIFY_CODE"
done
