#!/bin/env bash
# usage: ./run-test.sh [ input_file [eps]]

FORMAT="png" # png or eps


if [ $# -gt 0 ]; then
  OUTPUT_FILE=$1
else
  OUTPUT_FILE=$(hostname |grep -Eo "^[^.]+")"_"$(date +%F_%H-%M-%S)
  ./_test-threads.sh /dev/null $OUTPUT_FILE.xml
fi

if [ $# -eq 2 ];then
  FORMAT=$2
fi

  #OUTPUT_FILE="output"

CPUS=$((2*$( grep processor -c /proc/cpuinfo)))
if [ "$FORMAT" == "eps" ]; then
  ./_parse-throughput.py < $OUTPUT_FILE.xml |gnuplot -e "
  set terminal postscript eps fontscale 1.5;
  set output \"$OUTPUT_FILE.eps\";
  set boxwidth 0.9 absolute;
  set style fill pattern 1 border lt -1;
  set key inside right top vertical Right noreverse noenhanced autotitles nobox;
  set style histogram clustered gap 1 title  offset character 0, 0, 0;
  set style data histograms;
  set xtics  norangelimit font \",8\";
  set xtics   () ;
  set xlabel \"Threads\";
  set ylabel \"MiB/s\";
  plot \"-\" with boxes notitle"

else
  ./_parse-throughput.py < $OUTPUT_FILE.xml |gnuplot -e "
  set terminal png fontscale 1;
  set output \"$OUTPUT_FILE.png\";
  set boxwidth 0.9 absolute;
  set style fill pattern 1 border lt -1;
  set key inside right top vertical Right noreverse noenhanced autotitles nobox;
  set style histogram clustered gap 1 title  offset character 0, 0, 0;
  set style data histograms;
  set xtics  norangelimit font \",8\";
  set xtics   () ;
  set xlabel \"Threads\";
  set ylabel \"MiB/s\";
  plot \"-\"  with boxes notitle"

fi
