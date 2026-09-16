#!/bin/bash

THIS_DIR="$PATH_TO_TESTS_DIR/endpoints/api/image/get"

LOG_FILE="$PATH_TO_TESTS_DIR/endpoints/api/image/get/log.log"
rm "$LOG_FILE"

echo "Cleaning previous files"
log() {

  echo "$1" >&2
  echo "$1" >>"$LOG_FILE"
}
log "Starting log for /api/image/get/ test"
tar -xzvf "$THIS_DIR"/images-files.tar.gz -C "$THIS_DIR" || {
  echo "tar failed."
  exit 1
}

send_request() {
  log "ATTEMPTING TO UPLOAD IMAGE: \"$1\""
  log "REQUEST DATA: $2"
  echo "curl -v -s -D \"$2\" -X POST -H \"Expect: \" -H \"Content-Length: $(wc -c <"$1")\" --data-binary @\"$1\" \"http://$HOST/api/image\"" >&2
  RESULT=$(curl -v -s --no-buffer -D "$2" -X POST -H "Expect: " -H "Content-Length: $(wc -c <"$1")" --data-binary @"$1" "http://$HOST/api/image" 2>"/tmp/hermit-purple-test-helper.log")
  log "RESULT:=$RESULT"
  cat /tmp/hermit-purple-test-helper.log >>"$2"
  echo "$RESULT"

}

echo "Uploading images"
send_request "$THIS_DIR/1.original.jpg" "$THIS_DIR/1.original.jpg.log"
send_request "$THIS_DIR/2.original.jpeg" "$THIS_DIR/2.original.jpeg.log"
send_request "$THIS_DIR/3.original.png" "$THIS_DIR/3.original.png.log"
send_request "$THIS_DIR/4.original.heic" "$THIS_DIR/4.original.heic.log"

curl_image() {

  RESULT=$(curl -v -s -D "$THIS_DIR/$1.request.log" "http://$HOST/api/image?variant=$2&id=$1" 2>"/tmp/hermit-purple-test-helper.log")

  cat /tmp/hermit-purple-test-helper.log >>"$THIS_DIR/$1.request.log"

  echo "$RESULT"
}
get_image() {

  wget -q -O "$THIS_DIR/test.$1.$2" "http://$HOST/api/image?variant=$2&id=$1" >&2
  echo "$THIS_DIR/test.$1.$2"
}
log "Testing for: 1.original.jpg, original"
RESULT=$(get_image 1 original)
log "$RESULT"
if [ "$(diff "$THIS_DIR/1.original.jpg" "$RESULT")" != "" ]; then
  log "RESULTADO:"
  log "$(diff "$THIS_DIR/1.original.jpg" "$RESULT")"
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

log "Testing for: 1.compressed.jpg, compressed"
RESULT=$(get_image 1 compressed)
log "$RESULT"
if [ "$(diff "$THIS_DIR/1.compressed.jpg" "$RESULT")" != "" ]; then
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

log "Testing for: 1.thumbnail.jpg, thumbnail"
RESULT=$(get_image 1 thumbnail)
log "$RESULT"
if [ "$(diff "$THIS_DIR/1.thumbnail.jpg" "$RESULT")" != "" ]; then
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi
log "Testing for: 2.original.jpeg, original"
RESULT=$(get_image 2 original)
log "$RESULT"
if [ "$(diff "$THIS_DIR/2.original.jpeg" "$RESULT")" != "" ]; then
  log "RESULTADO:"
  log "$(diff "$THIS_DIR/2.original.jpeg" "$RESULT")"
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

log "Testing for: 2.compressed.jpeg, compressed"
RESULT=$(get_image 2 compressed)
log "$RESULT"
if [ "$(diff "$THIS_DIR/2.compressed.jpeg" "$RESULT")" != "" ]; then
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

log "Testing for: 2.thumbnail.jpeg, thumbnail"
RESULT=$(get_image 2 thumbnail)
log "$RESULT"
if [ "$(diff "$THIS_DIR/2.thumbnail.jpeg" "$RESULT")" != "" ]; then
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi
log "Testing for: 3.original.png, original"
RESULT=$(get_image 3 original)
log "$RESULT"
if [ "$(diff "$THIS_DIR/3.original.png" "$RESULT")" != "" ]; then
  log "RESULTADO:"
  log "$(diff "$THIS_DIR/3.original.png" "$RESULT")"
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

log "Testing for: 3.compressed.png, compressed"
RESULT=$(get_image 3 compressed)
log "$RESULT"
if [ "$(diff "$THIS_DIR/3.compressed.png" "$RESULT")" != "" ]; then
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

log "Testing for: 3.thumbnail.png, thumbnail"
RESULT=$(get_image 3 thumbnail)
log "$RESULT"
if [ "$(diff "$THIS_DIR/3.thumbnail.png" "$RESULT")" != "" ]; then
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi
log "Testing for: 4.original.heic, original"
RESULT=$(get_image 4 original)
log "$RESULT"
if [ "$(diff "$THIS_DIR/4.original.heic" "$RESULT")" != "" ]; then
  log "RESULTADO:"
  log "$(diff "$THIS_DIR/4.original.heic" "$RESULT")"
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

log "Testing for: 4.compressed.heic, compressed"
RESULT=$(get_image 4 compressed)
log "$RESULT"
if [ "$(diff "$THIS_DIR/4.compressed.heic" "$RESULT")" != "" ]; then
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

log "Testing for: 4.thumbnail.heic, thumbnail"
RESULT=$(get_image 4 thumbnail)
log "$RESULT"
if [ "$(diff "$THIS_DIR/4.thumbnail.heic" "$RESULT")" != "" ]; then
  log "TEST FAILED"
  exit 1
else
  log "TEST PASSED"
fi

#
# Testing for not found image - Image not found
#
# curl -v "http://localhost:1600/api/image?variant=original&id=12314123123"
log "Testing for id not found"
RESULT=$(curl_image 12314123123 original)
log "$RESULT"
if [ "$RESULT" = "Image not found" ]; then
  log "TEST PASSED"
else
  log "TEST FAILED"
  exit 1
fi
#
# TODO: Test for not supported variant - Unsupported variant
#
log "Testing for Unsupported variant not found"
RESULT=$(curl_image 1 madurohijodeputaestaspresomaldito)
log "$RESULT"
if [ "$RESULT" = "Unsupported variant" ] || [ "$RESULT" = "Not Found" ]; then
  log "TEST PASSED"
else
  log "TEST FAILED"
  exit 1
fi
#
# TODO: Test for out of bounds indexes
#

log "Testing for idx out of bounds "
# exit 1
exit 0
