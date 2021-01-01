#!/bin/bash
#duration;file;vcode
raspivid -w 300 -h 200 -b 15000000 -t ${1}000 -a 12 -a 1024 -a "CAM-1 %Y-%m-%d %X" -ae 18,0xff,0x808000 -o - |tee $2|ffmpeg -re -i - -s 300x200 -vcodec copy -acodec copy -b:v 800k -b:a 32k -f flv rtmp://172.81.227.199/livevideo$3