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
exit 0
