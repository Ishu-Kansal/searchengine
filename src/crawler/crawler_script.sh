#!/bin/bash

set -e
set -u
set -o pipefail

MAX_PROCESSED=5000000

CRAWLERS_PER_BATCH=1

CRAWLER_EXE="./crawler"

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <num_batches> <external_ip>" >&2
  exit 1
fi

NUM_BATCHES_STR="$1"
IP="$2"

if ! [[ "$NUM_BATCHES_STR" =~ ^[0-9]+$ ]]; then
   echo "Error: num_batches must be a non-negative integer." >&2
   exit 1
fi
NUM_BATCHES=$((NUM_BATCHES_STR))

# --- Calculations ---
TOTAL=$((NUM_BATCHES * CRAWLERS_PER_BATCH))

echo "Total potential processed items: $((TOTAL * MAX_PROCESSED))"
echo "Concurrency limit (max parallel crawlers): $CRAWLERS_PER_BATCH"
echo "Total crawlers to launch: $TOTAL"

# --- Spawner Function ---
run_crawler_until_success() {
  local id="$1"
  local ip_addr="$2"
  local attempt=0

  echo "[ID $id] Spawner started."
  while true; do
    attempt=$((attempt + 1))
    echo "[ID $id] Attempt $attempt: Executing '$CRAWLER_EXE $id $ip_addr'"
    # Execute the crawler
    if "$CRAWLER_EXE" "$id" "$ip_addr"; then
      echo "[ID $id] Attempt $attempt: Crawler succeeded."
      break
    else
      local status=$?
      echo "[ID $id] Attempt $attempt: Crawler failed with status $status. Retrying..." >&2
      sleep 1
    fi
  done
   echo "[ID $id] Spawner finished successfully."
}

export -f run_crawler_until_success
export CRAWLER_EXE 

# --- Main Loop ---
active_jobs=0

for i in $(seq 0 $((TOTAL - 1))); do
  if [ "$active_jobs" -ge "$CRAWLERS_PER_BATCH" ]; then
    echo "Concurrency limit ($CRAWLERS_PER_BATCH) reached. Waiting for a job to finish..."
    wait -n
    active_jobs=$((active_jobs - 1))
     echo "A job finished. Active jobs: $active_jobs"
  fi

  # Launch the spawner function in the background
  echo "Launching crawler for ID $i..."
  run_crawler_until_success "$i" "$IP" &
  active_jobs=$((active_jobs + 1))
  echo "Launched crawler for ID $i. Active jobs: $active_jobs"

done

echo "All $TOTAL crawlers launched. Waiting for remaining $active_jobs jobs to complete..."
wait
echo "All crawlers have completed."

exit 0