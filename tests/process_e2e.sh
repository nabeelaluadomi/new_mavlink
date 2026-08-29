#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${NEWMAVLINK_BUILD_DIR:-${ROOT}/build}"
TMP="$(mktemp -d /tmp/new_mavlink_process_e2e.XXXXXX)"
cleanup() {
  for pid in "${vehicle_pid:-}" "${proxy_pid:-}" "${gcs_pid:-}"; do
    if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then kill "$pid" 2>/dev/null || true; wait "$pid" 2>/dev/null || true; fi
  done
  rm -rf "$TMP"
}
wait_ready() {
  local file="$1" marker="$2" i
  for i in $(seq 1 40); do
    grep -q "$marker" "$file" 2>/dev/null && return 0
    sleep 0.05
  done
  return 1
}
trap cleanup EXIT
"$BUILD/newmavlink_gcs" 24661 >"$TMP/gcs.log" 2>&1 & gcs_pid=$!
"$BUILD/newmavlink_proxy" 24660 24661 >"$TMP/proxy.log" 2>&1 & proxy_pid=$!
wait_ready "$TMP/gcs.log" 'NEWMAVLINK_GCS_READY'
wait_ready "$TMP/proxy.log" 'NEWMAVLINK_PROXY_READY'
"$BUILD/newmavlink_vehicle" 0 24660 >"$TMP/vehicle.log" 2>&1
wait "$proxy_pid"
wait "$gcs_pid"
grep -q 'NEWMAVLINK_VEHICLE_SNAPSHOT=PASS' "$TMP/vehicle.log"
grep -q 'NEWMAVLINK_PROXY_REENCRYPT=PASS' "$TMP/proxy.log"
grep -q 'NEWMAVLINK_GCS_SNAPSHOT=PASS' "$TMP/gcs.log"
printf 'NEWMAVLINK_PROCESS_E2E=PASS vehicle_proxy_gcs=1 typed_position=1 session_reencryption=1\n'
