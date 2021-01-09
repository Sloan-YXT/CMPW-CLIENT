#!/bin/bash
#duration;file;vcode;id(for debug)
echo "$1;$2;$3;$4"
#raspivid -w 600 -h 480 -b 15000000 -t ${1}000 -a 12 -a 1024 -a "CAM-1 %Y-%m-%d %X" -ae 18,0xff,0x808000 -o - |tee $2|ffmpeg -re -i - -s 600x480 -vcodec copy -acodec copy -b:v 800k -b:a 32k -f flv rtmp://47.108.170.207/livevideo$3
raspivid -w 200 -h 200 -b 15000000 -t ${1}000 -a 12 -a 1024 -a "CAM-1 %Y-%m-%d %X" -ae 18,0xff,0x808000 -o - |ffmpeg -re -i - -s 200x200 -vcodec copy -acodec copy -b:v 800k -b:a 32k -f flv rtmp://47.108.170.207/livevideo$3