#!/bin/env bash
# Will run BIN rdrand test with different number of threads.
# If second argument THROUGHPUT_FILE is given,
# then output of the test will be written also to the file 
# as XML Will overwrite file if exists.

BIN="./RdRand"

# Minimum and maximum count of threads tested
MIN=1 
#MAX=30
MAX=$((2*$( grep processor -c /proc/cpuinfo)))

# Duration of each single tested method
DURATION=2

# size of array of 64bit values to be generated at once
CHUNK=2048

# How many times should be the test run for all methods 
# before computing an average throughput
REPETITION=2

# Limit the test to run on only one socket throught numactl -N 0
# Set to 0 to allow all sockets
# Set to 1 to force only socket no. 1
FORCE_SOCKET=0

##################################################################
# START

if [ $# -lt 1 ] || [ $# -gt 2 ]; then 
  echo "Usage: $0 RNG_OUTPUT_FILE [THROUGHPUT_FILE.xml] (files will be overwritten)"
  exit 1
fi

FILE_STDOUT="$1"

THROUGHPUT_FILE=""
if [ $# -eq 2 ]; then
  THROUGHPUT_FILE="$2"
  echo "<root>"> "$THROUGHPUT_FILE" # empty it
fi

echo "Will test from $MIN to $MAX threads."

for ((THREADS=$MIN ; THREADS<=$MAX ; THREADS++ )); do
  printf "Currently testing $THREADS threads:  "
  date 
  
  if [ $FORCE_SOCKET == 0 ]; then
    CMD="'$BIN' '$FILE_STDOUT' -d $DURATION -c $CHUNK -r $REPETITION -t $THREADS -m rdrand_get_bytes_retry"
  else
    CMD="numactl -N 0 '$BIN' '$FILE_STDOUT' -d $DURATION -c $CHUNK -r $REPETITION -t $THREADS -m rdrand_get_bytes_retry"
  fi

  if [ -z "$THROUGHPUT_FILE"  ]; then
    eval $CMD
  else
      RES=$(eval "$CMD 2>&1")
      echo "$RES" 
      echo -e "<test threads='$THREADS'>\n$RES\n</test>" >> "$THROUGHPUT_FILE"
  fi

done
if [ -n "$THROUGHPUT_FILE"  ]; then
  THROUGHPUT_FILE="$2"
  echo "</root>">> "$THROUGHPUT_FILE" # empty it
fi
