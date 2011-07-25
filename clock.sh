#!/bin/bash
center_x=30
center_y=20
diameter=18

hr_orig_x=0
hr_orig_y=10
sec_orig_x=0
sec_orig_y=18
min_orig_x=0
min_orig_y=15

ticks=""
for i in $(seq 0 12)
do
  tax=$(echo "scale=3;$center_x - $diameter * s($i/12*(2*4*a(1)))" | bc -l)
  tay=$(echo "scale=3;$center_y + $diameter * c($i/12*(2*4*a(1)))" | bc -l)
  tbx=$(echo "scale=3;$center_x - ($diameter - 3) * s($i/12*(2*4*a(1)))" | bc -l)
  tby=$(echo "scale=3;$center_y + ($diameter - 3) * c($i/12*(2*4*a(1)))" | bc -l)
  ticks="$ticks -draw \"line $tax,$tay $tbx,$tby\""
done

while true
do
Date=$(date +%Y-%m-%d)
Time=$(date +%H:%M:%S)
hr=$(date +%I)
min=$(date +%M)
sec=$(date +%S)
hr_x=$(echo "scale=3;$center_x + $hr_orig_y * s(($hr/12+$min/60/12)*(2*4*a(1)))" | bc -l)
hr_y=$(echo "scale=3;$center_y - $hr_orig_y * c(($hr/12+$min/60/12)*(2*4*a(1)))" | bc -l)

sec_x=$(echo "scale=3;$center_x + $sec_orig_y * s($sec/60*(2*4*a(1)))" | bc -l)
sec_y=$(echo "scale=3;$center_y - $sec_orig_y * c($sec/60*(2*4*a(1)))" | bc -l)

min_x=$(echo "scale=3;$center_x + $min_orig_y * s($min/60*(2*4*a(1)))" | bc -l)
min_y=$(echo "scale=3;$center_y - $min_orig_y * c($min/60*(2*4*a(1)))" | bc -l)
preparams="-size 160x43 xc:white -stroke black -fill white -draw \"circle 30,20 30,2\" -draw \"line 30,20 $sec_x,$sec_y\" -draw \"line 30,20 $min_x,$min_y\" -draw \"line 30,20 $hr_x,$hr_y\" "
postparams="-pointsize 16 -fill black -font Courier -draw \"text 60,15 '$Date'\" -draw \"text 68,35 '$Time'\" pbm:- "
eval convert $preparams $ticks $postparams | ./pbm2lpbm > /tmp/g13-0
sleep 1
done
