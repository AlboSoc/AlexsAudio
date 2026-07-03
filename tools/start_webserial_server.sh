#!/usr/bin/env sh

PORT="${1:-8000}"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
URL="http://localhost:${PORT}/tools/webserial_trigger.html"

if command -v python3 >/dev/null 2>&1; then
  PYTHON_CMD="python3"
elif command -v python >/dev/null 2>&1; then
  PYTHON_CMD="python"
else
  echo "Neither 'python3' nor 'python' was found on PATH." >&2
  exit 1
fi

echo
echo "Starting local web server from: $REPO_ROOT"
echo "Open this URL in a Chromium-based browser:"
echo "$URL"
echo
echo "Press Ctrl+C to stop the server."
echo

cd "$REPO_ROOT" || exit 1
exec "$PYTHON_CMD" -m http.server "$PORT"
