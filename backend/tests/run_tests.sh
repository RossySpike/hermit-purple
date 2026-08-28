#!/bin/bash
set -o allexport && source ./tests.env && set +o allexport

echo "Attempting: \"$PATH_TO_TESTS_DIR/endpoints/api/image/get/test.sh\""
"$PATH_TO_TESTS_DIR/endpoints/api/image/get/test.sh"

RESULT="$?"
echo "RESULT:=$RESULT"
echo "$((RESULT != 0))"
if [ $RESULT != 0 ]; then
  exit 1
fi

echo "Attempting: \"$PATH_TO_TESTS_DIR/endpoints/api/image/cursor/start/get/test.sh\""
"$PATH_TO_TESTS_DIR/endpoints/api/image/cursor/start/get/test.sh"
RESULT="$?"
echo "RESULT:=$RESULT"
if [ $RESULT != 0 ]; then
  exit 1
fi

echo "Attempting: \"$PATH_TO_TESTS_DIR/endpoints/api/image/cursor/get/test.sh\""
"$PATH_TO_TESTS_DIR/endpoints/api/image/cursor/get/test.sh"
RESULT="$?"
echo "RESULT:=$RESULT"
if [ $RESULT != 0 ]; then
  exit 1
fi

echo "Attempting: \"$PATH_TO_TESTS_DIR/endpoints/api/image/post/upload-image.sh\""
"$PATH_TO_TESTS_DIR/endpoints/api/image/post/upload-image.sh"

RESULT="$?"
echo "RESULT:=$RESULT"
if [ $RESULT != 0 ]; then
  exit 1
fi

echo "ALL TEST PASSED"
exit 0
