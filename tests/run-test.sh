#!/bin/env bash

./_test-threads.sh /dev/null output

./_parse-throughput.py < output |gnuplot -e "
set terminal unknown;
set xrange [0:201];
set terminal png size 1000,500;
set output \"throughput.png\";
set boxwidth 0.5;
set style fill solid;
set xtics 0,10,200;
set xlabel \"Threads\";
set ylabel \"MiB/s\";
plot \"-\" with boxes notitle"