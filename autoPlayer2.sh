#!/bin/bash
BROKER="localhost"
STATUS_TOPIC="statusOutput"
AVAIL_TOPIC="availSpacesOutput"
INPUT_TOPIC="bashInput"

echo "Player 2 (Bash) ready!"

STATUS=""
AVAIL=""

# Start mosquitto_sub and save its PID
mosquitto_sub -h $BROKER -t $STATUS_TOPIC -t $AVAIL_TOPIC -v &
MOSQ_PID=$!

# Read from the background process
mosquitto_sub -h $BROKER -t $STATUS_TOPIC -t $AVAIL_TOPIC -v | while read -r TOPIC MSG; do
    if [[ "$TOPIC" == "$STATUS_TOPIC" ]]; then
        STATUS="$MSG"
        if [[ "$STATUS" == *"wins"* ]] || [[ "$STATUS" == *"draw"* ]]; then
            echo "Game over: $STATUS"
            kill $MOSQ_PID 2>/dev/null
            exit 0
        fi
    elif [[ "$TOPIC" == "$AVAIL_TOPIC" ]]; then
        AVAIL="$MSG"
        if [[ "$STATUS" == *"Player 2"* ]]; then
            IFS=',' read -ra SPACES <<< "$AVAIL"
            RANDOM_INDEX=$((RANDOM % ${#SPACES[@]}))
            MOVE=${SPACES[$RANDOM_INDEX]}
            echo "Playing move: $MOVE"
            mosquitto_pub -h $BROKER -t $INPUT_TOPIC -m "$MOVE"
        fi
    fi
done

wait $MOSQ_PID 2>/dev/null
