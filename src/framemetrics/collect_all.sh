#!/bin/bash

if [ ! -d out ]; then
  mkdir out;
fi

if [ ! -e groups.csv ]; then
    ./framemetrics $* > groups.csv
fi

while read GROUP
do
    ./framemetrics -g "$GROUP" $* > "out/$GROUP"
done <groups.csv

paste out/* > full_metrics.csv
