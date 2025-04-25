#!/bin/bash

# --- Configuration ---
DISPATCHER_CMD="./dispatcher"
RETRY_DELAY=5

CRAWLER_PATTERN="^crawler" #

# --- Logging Function ---
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# --- Main Loop Function ---
run_dispatcher_monitor() {
    log "Starting dispatcher monitor loop."

    while true; do
        log "Attempting to start $DISPATCHER_CMD..."

        "$DISPATCHER_CMD"
        status=$?

        log "$DISPATCHER_CMD exited with status: $status"

        if [ $status -eq 0 ]; then
            # Dispatcher exited successfully (status 0)
            log "Dispatcher finished successfully. Exiting monitor loop."
            break # Exit the while loop
        else
            # Dispatcher failed or exited with a non-zero status
            log "Dispatcher failed or exited non-zero (status $status)."

            # --- Kill Crawler Logic ---
            log "Attempting to kill processes matching pattern '$CRAWLER_PATTERN'..."

            pkill -f "$CRAWLER_PATTERN"
            kill_status=$?
            if [ $kill_status -eq 0 ]; then
                log "pkill command succeeded (at least one process matched and was signaled)."
            elif [ $kill_status -eq 1 ]; then
                log "pkill command found no processes matching the pattern."
            else
                log "pkill command encountered an error (status $kill_status)."
            fi
            # --- End Kill Crawler Logic ---

            log "Retrying $DISPATCHER_CMD in $RETRY_DELAY seconds..."
            sleep "$RETRY_DELAY"
        fi
    done

    log "Dispatcher monitor loop finished."
}

# --- Script Entry Point ---
if [ ! -x "$DISPATCHER_CMD" ]; then
    log "Error: Dispatcher command '$DISPATCHER_CMD' not found or not executable." >&2
    exit 1
fi

run_dispatcher_monitor &

monitor_pid=$!
log "Dispatcher monitor loop started in background with PID: $monitor_pid"
log "Main script exiting. Monitor will continue running."
log "If using nohup, output will typically be in 'nohup.out'."

exit 0