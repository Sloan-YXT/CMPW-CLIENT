#!/bin/bash
ffmpeg -i $1 -r $2 -q:v 2 -f image2 $3-%d.jpeg