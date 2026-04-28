#IA code

#!/bin/bash

# File to store the PID
PID_FILE=".monitor_pid"

# Function to handle cleanup on exit
cleanup() {
    echo -e "\n[$(date +'%Y-%m-%d %H:%M:%S')] SIGINT received. Cleaning up..."
    if [ -f "$PID_FILE" ]; then
        rm "$PID_FILE"
        echo "[$(date +'%Y-%m-%d %H:%M:%S')] $PID_FILE removed. Exiting."
    fi
    exit 0
}

# Function to handle SIGUSR1 (new report added)
handle_sigusr1() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] SIGUSR1 received: New report added."
}

# 1. Setup Signal Traps
# Trap SIGINT (Ctrl+C) for cleanup
trap cleanup SIGINT
# Trap SIGUSR1 to handle new report notifications
trap handle_sigusr1 SIGUSR1

# 2. Startup: Create/Overwrite .monitor_pid
echo $$ > "$PID_FILE"
echo "[$(date +'%Y-%m-%d %H:%M:%S')] Monitor started with PID: $$"
echo "[$(date +'%Y-%m-%d %H:%M:%S')] Monitoring reports... (Press Ctrl+C to stop)"

# 3. Keep the program running
while true; do
    # Pause the loop, waiting for signals
    sleep 1
done
