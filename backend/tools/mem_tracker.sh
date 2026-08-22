#!/bin/bash

PROCESS="$1" || (
  echo "You must provide the path of the program that you want to launch"
  echo "USAGE: ./mem_tracker.sh path_to_program optional_path_to_log_file"
  exit 1
)
LOG_FILE_BASE_NAME="${2:-./mem_tracker}"
LOG_FILE="$LOG_FILE_BASE_NAME.$(date +"%Y-%m-%d_%H-%M-%S").log"
log() {

  STRING="[MEM_TRACKER]: \"$1\""
  echo "$STRING"
  echo "$STRING" >>"$LOG_FILE"
}
log "launching \"$PROCESS\""
log "log file \"$LOG_FILE\""

./"$PROCESS" &

PROCESS_PID=$!
# check if process is running
while true; do
  date +"%Y-%m-%d_%H-%M-%S" >>"$LOG_FILE"
  grep -E 'Name|VmSize|VmRSS|VmData|VmStk|VmExe|VmSwap|VmHWM|FDSize|RssAnon|RssFile|voluntary_ctxt_switches|nonvoluntary_ctxt_switches' /proc/"$PROCESS_PID"/status >>"$LOG_FILE"
  sleep 1m
done
