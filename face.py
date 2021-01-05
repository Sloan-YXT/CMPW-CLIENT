#!/usr/bin/python3
#  -*- coding: gb2312 -*-
import numpy as np
import cv2
import sys
import os
import socket
# 要使用Haar cascade实现，仅需要把库修改为lbpcascade_frontalface.xml
# cv2.data.haarcascades
#face_cascade = cv2.CascadeClassifier('lbpcascade_frontalface.xml')
#FIFO_NAME = "./communication.fifo"
HOST = "127.0.0.1"
PORT = sys.argv[3]
#f_record = open(FIFO_NAME, 'w')
f_record = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
f_record.connect((HOST, PORT))
face_cascade = cv2.CascadeClassifier(
    "/home/pi/opencv/data/haarcascades/haarcascade_frontalface_default.xml"
)
gpath = sys.argv[1]
fpath = sys.argv[2]
for roots, dirs, files in os.walk(gpath):
    for file in files:
        print("./"+roots+"/"+file)
        img = cv2.imread(roots+"/"+file)
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
# 识别输入图片中的人脸对象.返回对象的矩形尺寸
# 函数原型detectMultiScale(gray, 1.2,3,CV_HAAR_SCALE_IMAGE,Size(30, 30))
# gray需要识别的图片
# 1.03：表示每次图像尺寸减小的比例
# 5：表示每一个目标至少要被检测到4次才算是真的目标(因为周围的像素和不同的窗口大小都可以检测到人脸)
# CV_HAAR_SCALE_IMAGE表示不是缩放分类器来检测，而是缩放图像，Size(30, 30)为目标的最小最大尺寸
# faces：表示检测到的人脸目标序列
        faces = face_cascade.detectMultiScale(gray, 1.03, 5)
        if len(faces) != 0:
            print('debug:'+fpath+"/"+file)
            cv2.imwrite(fpath + "/" + file, img)
            b = fpath+"/"+file
            f_record.sendall(len(b))
            f_record.sendall(b)
            #print('did not detect any')
        for (x, y, w, h) in faces:
            if w+h > 200:  # //针对这个图片画出最大的外框
                img2 = cv2.rectangle(
                    img, (x, y), (x+w, y+h), (255, 255, 255), 4)
        #roi_gray = gray[y:y+h, x:x+w]
        #roi_color = img[y:y+h, x:x+w]

        #cv2.imshow('img', img)
        # cv2.waitKey(1000)
        # cv2.destroyAllWindows()
          # 保存图片
f_record.close()
