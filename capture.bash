#!/bin/bash
#vpath;duration;gpath
#g++ faceDetect.cpp -o faceDetct  `pkg-config opencv4 --cflags --libs`
#ffmpeg -i $1 -r $2 -q:v 2 -f image2 $3-%d.jpeg
fswebcam -d /dev/video0 --no-banner -r 320x640 ${3}.jpg