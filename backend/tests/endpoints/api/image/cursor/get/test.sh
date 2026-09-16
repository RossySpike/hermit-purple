#!/bin/bash
THIS_DIR="$PATH_TO_TESTS_DIR/endpoints/api/image/cursor/get"
echo "COMPARING RETRIEVED FILES WITH LOCAL IMAGES"
RESULT=$(python "$THIS_DIR/test.py")
echo "$RESULT"
if [ "$RESULT" != "TEST PASSED" ]; then
  exit 1
fi
# TODO: bound check for limit and current url param
echo "WARNING. NO TEST FOR: bound check for limit and current url param"

log() {

  echo "$1" >&2
}

send_request() {
  echo "curl -v -s       \"http://$HOST/api/image/cursor?current=\"$1\"&limit=\"$2\"\"" >&2
  RESULT=$(curl -v -s "http://$HOST/api/image/cursor?current=$1&limit=$2")
  log "RESULT:=$RESULT"
  echo "$RESULT"

}

TEST=$(send_request -1 2)
if [ "$TEST" = "Not Found" ] || [ "$TEST" = "Invalid current parameter" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
TEST=$(send_request 5 -2)
if [ "$TEST" = "Not Found" ] || [ "$TEST" = "Invalid limit parameter" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
TEST=$(send_request 5 7)
if [ "$TEST" = "Invalid limit parameter" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
TEST=$(send_request 999 7)
if [ "$TEST" = "Current parameter exceeds available images" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi

exit 0
