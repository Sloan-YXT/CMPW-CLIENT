#!/usr/bin/python3
#  -*- coding: gb2312 -*-
import numpy as np
import cv2
import sys
import os
import socket
# Ҫʹ��Haar cascadeʵ�֣�����Ҫ�ѿ��޸�Ϊlbpcascade_frontalface.xml
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
# ʶ������ͼƬ�е���������.���ض���ľ��γߴ�
# ����ԭ��detectMultiScale(gray, 1.2,3,CV_HAAR_SCALE_IMAGE,Size(30, 30))
# gray��Ҫʶ���ͼƬ
# 1.03����ʾÿ��ͼ��ߴ��С�ı���
# 5����ʾÿһ��Ŀ������Ҫ����⵽4�β��������Ŀ��(��Ϊ��Χ�����غͲ�ͬ�Ĵ��ڴ�С�����Լ�⵽����)
# CV_HAAR_SCALE_IMAGE��ʾ�������ŷ���������⣬��������ͼ��Size(30, 30)ΪĿ�����С���ߴ�
# faces����ʾ��⵽������Ŀ������
        faces = face_cascade.detectMultiScale(gray, 1.03, 5)
        if len(faces) != 0:
            print('debug:'+fpath+"/"+file)
            cv2.imwrite(fpath + "/" + file, img)
            b = fpath+"/"+file
            f_record.sendall(len(b))
            f_record.sendall(b)
            #print('did not detect any')
        for (x, y, w, h) in faces:
            if w+h > 200:  # //������ͼƬ�����������
                img2 = cv2.rectangle(
                    img, (x, y), (x+w, y+h), (255, 255, 255), 4)
        #roi_gray = gray[y:y+h, x:x+w]
        #roi_color = img[y:y+h, x:x+w]

        #cv2.imshow('img', img)
        # cv2.waitKey(1000)
        # cv2.destroyAllWindows()
          # ����ͼƬ
f_record.close()
