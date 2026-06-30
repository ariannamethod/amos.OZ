#!/bin/sh
# amosOZ shell treaty smoke suite (non-interactive)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/amosoz"
[ -x "$BIN" ] || { echo "build first: make"; exit 1; }

run() {
  printf '%s\n' "$@" | "$BIN" 2>&1
}

out=$(run 'echo treaty > /tmp/treaty.txt' 'cat /tmp/treaty.txt')
echo "$out" | grep -q treaty || { echo "FAIL: redirect"; exit 1; }

out=$(run 'echo line2 >> /tmp/treaty.txt' 'cat /tmp/treaty.txt')
echo "$out" | grep -q line2 || { echo "FAIL: append"; exit 1; }

out=$(run 'echo piped | cat')
echo "$out" | grep -q piped || { echo "FAIL: pipe"; exit 1; }

out=$(run 'which echo')
echo "$out" | grep -q '/bin/echo' || { echo "FAIL: which"; exit 1; }

out=$(run 'exec /home/user/hello.amos amos')
echo "$out" | grep -q amos || { echo "FAIL: script"; exit 1; }

echo "shell_treaty: ALL PASSED"