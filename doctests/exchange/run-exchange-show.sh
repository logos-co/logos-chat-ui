#!/usr/bin/env bash
# Role: doc-test display entry (the `#exchange` flake app); wraps run-exchange.sh then holds the window open.
# Run the real two-party message exchange, then hold the receiving window open
# showing the result — the entry point for the `#exchange` flake app the doc-test
# drives.
#
# A message round-trip is two-party and slow (two delivery nodes finding each
# other over the network), while the doc-test caps its capture step at 120s. So
# this splits the work: the slow exchange runs first, to a *persistent* data dir,
# on its own inspector ports; then both instances are torn down and Alice is
# relaunched from her data dir, reloading the finished thread from `history.json`
# with no network. The doc-test attaches to that reloaded window (on SHOW_PORT)
# and screenshots it well within its budget — the exchange happens during the
# unbounded `launch_timeout` port-wait, before SHOW_PORT ever opens.
#
# Usage:
#   run-exchange-show.sh [base-dir]
# Env:
#   APP_BIN       run-logos-standalone-ui path (baked in by the flake app)
#   SHOW_PORT     inspector port for the reloaded window (default 3768)
#   SHOW_DELIVERY delivery port for the reloaded window (default 60012)
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE="${1:-$(mktemp -d "${TMPDIR:-/tmp}/chat-exchange-show.XXXXXX")}"
DATA_DIR="$BASE/data"
OUT_DIR="${OUT_DIR:-$BASE/images}"
SHOW_PORT="${SHOW_PORT:-3768}"
SHOW_DELIVERY="${SHOW_DELIVERY:-60012}"
mkdir -p "$DATA_DIR" "$OUT_DIR"

# Phase 1: the real round-trip. Distinct inspector ports (4768/4769) keep
# SHOW_PORT free so it only opens for the reloaded window in phase 2. run-exchange
# tears its two instances down on exit but keeps DATA_DIR (KEEP_WORK_DIR), leaving
# Alice's persisted history at $DATA_DIR/alice.
echo "=== phase 1: two-party exchange (persisting Alice's history) ==="
ALICE_PORT=4768 BOB_PORT=4769 ALICE_DELIVERY=60010 BOB_DELIVERY=60011 \
  WORK_DIR="$DATA_DIR" KEEP_WORK_DIR=1 OUT_DIR="$OUT_DIR" \
  bash "$here/run-exchange.sh" "$OUT_DIR"

# Phase 2: relaunch Alice from her persisted data dir on SHOW_PORT. The module
# host processes set their own session, so kill exactly the ones this phase adds
# (set difference) on exit — matching run-exchange.sh's teardown.
echo "=== phase 2: reload Alice and show the round-trip ==="
LOGOS_PAT='logos_host_qt|logos-standalone-app|ui-host'
PRE_PIDS="$(pgrep -f "$LOGOS_PAT" 2>/dev/null | sort -u || true)"
show_cleanup() {
  local now ours
  now="$(pgrep -f "$LOGOS_PAT" 2>/dev/null | sort -u || true)"
  ours="$(comm -13 <(printf '%s\n' "$PRE_PIDS") <(printf '%s\n' "$now") || true)"
  [ -n "$ours" ] && kill -9 $ours 2>/dev/null || true
}
trap 'exit 0' TERM INT
trap show_cleanup EXIT

QT_QPA_PLATFORM=offscreen QT_FORCE_STDERR_LOGGING=1 \
  QML_INSPECTOR_PORT="$SHOW_PORT" \
  CHAT_MODULE_INSTANCE_PATH="$DATA_DIR/alice" \
  CHAT_MODULE_DELIVERY_PORT="$SHOW_DELIVERY" \
  setsid "$APP_BIN" -platform offscreen > "$BASE/show-alice.log" 2>&1 &

echo "waiting for the reloaded inspector on port $SHOW_PORT ..."
for _ in $(seq 1 120); do
  (exec 3<>"/dev/tcp/127.0.0.1/$SHOW_PORT") 2>/dev/null && break
  sleep 1
done

SHOW_PORT="$SHOW_PORT" node "$here/select-latest.mjs"

# Phase 3: hold the reloaded window open for the doc-test to screenshot. The
# doc-test kills this process group when done; the EXIT trap reaps Alice.
echo "=== ready: Alice reloaded on inspector $SHOW_PORT showing the round-trip ==="
while true; do sleep 3600 & wait $!; done
