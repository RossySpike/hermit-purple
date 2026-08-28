#!/bin/bash

HOST="localhost:1600"
IMAGES=$(curl http://"$HOST"/api/image/cursor/start || (
  echo "Network error"
  exit 1
))

# https://www.baeldung.com/linux/bash-variable-is-numeric
if ! [[ $IMAGES =~ ^[0-9]+$ ]]; then
  echo "Response from is not a number" >&2
  exit 1
fi
echo "$IMAGES" >&2
echo "TEST PASSED"
exit 0
