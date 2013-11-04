#!/bin/env bash

OUTPUT_FILE="output_`date +%F_%H-%M-%S`"

./_test-threads.sh /dev/null $OUTPUT_FILE.xml

./_parse-throughput.py < $OUTPUT_FILE.xml |gnuplot -e "
set terminal unknown;
set xrange [0:201];
set terminal png size 1000,500;
set output \"$OUTPUT_FILE.png\";
set boxwidth 0.5;
set style fill solid;
set xtics 0,10,200;
set xlabel \"Threads\";
set ylabel \"MiB/s\";
plot \"-\" with boxes notitle"
