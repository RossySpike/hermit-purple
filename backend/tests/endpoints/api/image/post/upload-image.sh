#!/bin/bash

# HOST="localhost:1600"
# PATH_TO_TESTS_DIR="/home/honey/proyectos/hermit-purple/backend/tests"

THIS_DIR="$PATH_TO_TESTS_DIR/endpoints/api/image/post"

LOG_FILE="$PATH_TO_TESTS_DIR/endpoints/api/image/post/log.log"
rm "$LOG_FILE"
log() {

  echo "$1" >&2
  echo "$1" >>"$LOG_FILE"
}
log "Starting log for /api/image/post/ test"
# IMAGES=$("$PATH_TO_TESTS_DIR"/endpoints/api/image/cursor/start/get/test.sh || (
#   echo "Network error"
#
#   exit 1
# ))
# echo "$IMAGES"

send_request() {
  log "ATTEMPTING TO UPLOAD IMAGE: \"$1\""
  log "REQUEST DATA: $2"
  echo "curl -v -s -D \"$2\" -X POST -H \"Expect: \" -H \"Content-Length: $(wc -c <"$1")\" --data-binary @\"$1\" \"http://$HOST/api/image\"" >&2
  RESULT=$(curl -v -s -D "$2" -X POST -H "Expect: " -H "Content-Length: $(wc -c <"$1")" --data-binary @"$1" "http://$HOST/api/image" 2>"/tmp/hermit-purple-test-helper.log")
  log "RESULT:=$RESULT"
  cat /tmp/hermit-purple-test-helper.log >>"$2"
  echo "$RESULT"

}
send_request_no_content_length() {
  log "ATTEMPTING TO UPLOAD IMAGE: \"$1\""
  log "REQUEST DATA: $2"
  RESULT=$(curl -v -s -D "$2" -X POST --http1.0 -H "Transfer-Encoding: chunked" -H "Expect: " --data-binary @"$1" "http://$HOST/api/image" 2>"/tmp/hermit-purple-test-helper.log")
  log "RESULT:=$RESULT"
  cat /tmp/hermit-purple-test-helper.log >>"$2"
  echo "$RESULT"

}
send_request_content_length_zero_with_body() {
  log "ATTEMPTING TO UPLOAD IMAGE: \"$1\""
  log "REQUEST DATA: $2"
  echo "curl -v -s -D $2 -X POST -H \"Expect: \" -H \"Content-Length: 0\" --data-binary @\"$1\" \"http://$HOST/api/image\"" >&2
  RESULT=$(curl -v -s -D "$2" -X POST -H "Expect: " -H "Content-Length: 0" --data-binary @"$1" "http://$HOST/api/image" 2>"/tmp/hermit-purple-test-helper.log")
  log "RESULT:=$RESULT"
  cat /tmp/hermit-purple-test-helper.log >>"$2"
  echo "$RESULT"

}

log "TEST: uploading a valid .jpg with Content-Length. Expected results: response: 201 body: Created"
TEST=$(send_request "$THIS_DIR/good.jpg" "$THIS_DIR/good.jpg.201.created.header.log")

if [ "$TEST" = "Created" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading a valid .heic with Content-Length. Expected results: response: 201 body: Created"
TEST=$(send_request "$THIS_DIR/good.heic" "$THIS_DIR/good.heic.201.created.header.log")

if [ "$TEST" = "Created" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi

log "TEST: uploading a valid .png with Content-Length. Expected results: response: 201 body: Created"
TEST=$(send_request "$THIS_DIR/good.png" "$THIS_DIR/good.png.201.created.header.log")

if [ "$TEST" = "Created" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading a valid .jpeg with Content-Length. Expected results: response: 201 body: Created"
TEST=$(send_request "$THIS_DIR/good.jpeg" "$THIS_DIR/good.jpeg.201.created.header.log")

if [ "$TEST" = "Created" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
#
# Missing Content-Length header
#
log "TEST: uploading an unvalid .jpg withouth Content-Length. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_no_content_length "$THIS_DIR/bad-no-contents.jpg" "$THIS_DIR/bad-no-contents.jpg.400.created.header.log")

if [ "$TEST" = "Missing Content-Length header" ] || [ "$TEST" = "Bad headers" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading an unvalid .heic withouth Content-Length. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_no_content_length "$THIS_DIR/bad-no-contents.heic" "$THIS_DIR/bad-no-contents.heic.400.created.header.log")

if [ "$TEST" = "Missing Content-Length header" ] || [ "$TEST" = "Bad headers" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi

log "TEST: uploading an unvalid .png withouth Content-Length. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_no_content_length "$THIS_DIR/bad-no-contents.png" "$THIS_DIR/bad-no-contents.png.400.created.header.log")

if [ "$TEST" = "Missing Content-Length header" ] || [ "$TEST" = "Bad headers" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading an unvalid .jpeg withouth Content-Length. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_no_content_length "$THIS_DIR/bad-no-contents.jpeg" "$THIS_DIR/bad-no-contents.jpeg.400.created.header.log")

if [ "$TEST" = "Missing Content-Length header" ] || [ "$TEST" = "Bad headers" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
#
# Content-Length: 0 but does have body
#
log "TEST: uploading an valid .jpg with Content-Length: 0. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_content_length_zero_with_body "$THIS_DIR/good.jpg" "$THIS_DIR/bad-zero-content-length.jpg.400.created.header.log")

if [ "$TEST" = "Content-Length cannot be 0" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading a valid .heic withouth Content-length:0. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_content_length_zero_with_body "$THIS_DIR/good.heic" "$THIS_DIR/bad-zero-content-length.heic.400.created.header.log")

if [ "$TEST" = "Content-Length cannot be 0" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi

log "TEST: uploading a valid .png withouth Content-length:0. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_content_length_zero_with_body "$THIS_DIR/good.png" "$THIS_DIR/bad-zero-content-length.png.400.created.header.log")

if [ "$TEST" = "Content-Length cannot be 0" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading a valid .jpeg withouth Content-length:0. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_content_length_zero_with_body "$THIS_DIR/good.jpeg" "$THIS_DIR/bad-zero-content-length.jpeg.400.created.header.log")

if [ "$TEST" = "Content-Length cannot be 0" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
#
# Content-Length: 0 but doesnt have body
#
log "TEST: uploading an unvalid .jpg with Content-Length: 0. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_content_length_zero_with_body "$THIS_DIR/bad-no-contents.jpg" "$THIS_DIR/bad-zero-content-length-no-body.jpg.400.created.header.log")

if [ "$TEST" = "PUT and POST methods require a body" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading an unvalid .heic withouth Content-length:0. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_content_length_zero_with_body "$THIS_DIR/bad-no-contents.heic" "$THIS_DIR/bad-zero-content-length-no-body.heic.400.created.header.log")

if [ "$TEST" = "PUT and POST methods require a body" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi

log "TEST: uploading an unvalid .png withouth Content-length:0. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_content_length_zero_with_body "$THIS_DIR/bad-no-contents.png" "$THIS_DIR/bad-zero-content-length-no-body.png.400.created.header.log")

if [ "$TEST" = "PUT and POST methods require a body" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading an unvalid .jpeg withouth Content-length:0. Expected results: response: 400 body: Missing Content-Length header"
TEST=$(send_request_content_length_zero_with_body "$THIS_DIR/bad-no-contents.jpeg" "$THIS_DIR/bad-zero-content-length-no-body.jpeg.400.created.header.log")

if [ "$TEST" = "PUT and POST methods require a body" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
#
# Valid request but unvalid (non matching magic numbers)
#
log "TEST: uploading an unvalid .jpg with Content-Length. Expected results: response: 201 body: Created"
TEST=$(send_request "$THIS_DIR/bad-unvalid.jpg" "$THIS_DIR/bad-unvalid.jpg.201.created.header.log")

if [ "$TEST" = "Unsupported file type" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading an unvalid .heic with Content-Length. Expected results: response: 201 body: Created"
TEST=$(send_request "$THIS_DIR/bad-unvalid.heic" "$THIS_DIR/bad-unvalid.heic.201.created.header.log")

if [ "$TEST" = "Unsupported file type" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi

log "TEST: uploading an unvalid .png with Content-Length. Expected results: response: 201 body: Created"
TEST=$(send_request "$THIS_DIR/bad-unvalid.png" "$THIS_DIR/bad-unvalid.png.201.created.header.log")

if [ "$TEST" = "Unsupported file type" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi
log "TEST: uploading an unvalid .jpeg with Content-Length. Expected results: response: 201 body: Created"
TEST=$(send_request "$THIS_DIR/bad-unvalid.jpeg" "$THIS_DIR/bad-unvalid.jpeg.201.created.header.log")

if [ "$TEST" = "Unsupported file type" ]; then
  echo "Test passed"

else
  echo "Test failed"
  exit 1
fi

send_request_excess_content_length() {
  log "ATTEMPTING TO UPLOAD IMAGE: \"$1\""
  log "REQUEST DATA: $2"
  echo "curl -v -s -D \"$2\" -X POST -H \"Expect: \" -H \"Content-Length: $(($(wc -c <"$1") + 500))\" --data-binary @\"$1\" \"http://$HOST/api/image\"" >&2
  RESULT=$(curl -v -s -D "$2" -X POST -H "Expect: " -H "Content-Length: $(($(wc -c <"$1") + 500))" --data-binary @"$1" "http://$HOST/api/image" 2>"/tmp/hermit-purple-test-helper.log")
  log "RESULT:=$RESULT"
  cat /tmp/hermit-purple-test-helper.log >>"$THIS_DIR/$2"
  echo "$RESULT"

}
#
# Valid file but with a greater Content-Length than file bytes
#
# log "TEST: uploading a valid .jpg with a greater Content-Length. Expected results: response: 408 body: Request Timeout"
# TEST=$(send_request_excess_content_length "$THIS_DIR/good.jpg" "good.jpg.408.timeout.header.log")
#
# if [ "$TEST" = "Request Timeout" ]; then
#   echo "Test passed"
#
# else
#   echo "Test failed"
#   exit 1
# fi
# log "TEST: uploading a valid .heic with a greater Content-Length. Expected results: response: 408 body: Request Timeout"
# TEST=$(send_request_excess_content_length "$THIS_DIR/good.heic" "good.heic.408.timeout.header.log")
#
# if [ "$TEST" = "Request Timeout" ]; then
#   echo "Test passed"
#
# else
#   echo "Test failed"
#   exit 1
# fi
#
# log "TEST: uploading a valid .png with a greater Content-Length. Expected results: response: 408 body: Request Timeout"
# TEST=$(send_request_excess_content_length "$THIS_DIR/good.png" "good.png.408.timeout.header.log")
#
# if [ "$TEST" = "Request Timeout" ]; then
#   echo "Test passed"
#
# else
#   echo "Test failed"
#   exit 1
# fi
# log "TEST: uploading a valid .jpeg with a greater Content-Length. Expected results: response: 408 body: Request Timeout"
# TEST=$(send_request_excess_content_length "$THIS_DIR/good.jpeg" "good.jpeg.408.timeout.header.log")
#
# if [ "$TEST" = "Request Timeout" ]; then
#   echo "Test passed"
#
# else
#   echo "Test failed"
#   exit 1
# fi

exit 0
