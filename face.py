#!/usr/bin/python3
#  -*- coding: utf-8 -*-
import numpy as np
import cv2
import sys
import os
import socket
# Ҫʹ��Haar cascadeʵ�֣�����Ҫ�ѿ��޸�Ϊlbpcascade_frontalface.xml
# cv2.data.haarcascades
# face_cascade = cv2.CascadeClassifier('lbpcascade_frontalface.xml')
# FIFO_NAME = "./communication.fifo"
HOST = "172.27.35.3"
PORT = sys.argv[3]
# f_record = open(FIFO_NAME, 'w')
f_record = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# str(socket.htons(int(PORT)))
print('debug:' + PORT)
# socket.htons(int(PORT))

face_cascade = cv2.CascadeClassifier(
    "/home/pi/opencv/data/haarcascades/haarcascade_frontalface_default.xml"
)
gpath = sys.argv[1]
fpath = sys.argv[2]
words = []
for roots, dirs, files in os.walk(gpath):
    for file in files:
        # print("./" + roots + "/" + file)
        words.append(file)
words.sort()
if(len(words) != 0):
    a = words[len(words) - 1]
else:
    sys.exit(0)
print(a)
print(type(a))
if int(a[5]) > int(sys.argv[4]):
    words[len(words)-1] = a[0:5]+sys.argv[4]+".jpg"
    print('debug:modify to '+words[len(words)-1])

gpath = sys.argv[1] + "/"+words[len(words) - 1]
fpath = sys.argv[2]+"/"+words[len(words)-1]
# print(gpath)
# print(fpath)
img = cv2.imread(gpath)
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
    print('debug:'+fpath)
    for (x, y, w, h) in faces:
        if w+h > 200:  # //������ͼƬ�����������
            img2 = cv2.rectangle(
                img, (x, y), (x + w, y + h), (255, 255, 255), 4)
            roi_gray = gray[y:y+h, x:x+w]
            roi_color = img[y:y+h, x:x+w]
    cv2.imwrite(fpath, img)
    f_record.connect((HOST, int(PORT)))
    b = fpath
    f_record.send(len(b).to_bytes(4, 'little'))
    f_record.sendall(b.encode())
    # print('did not detect any')

# roi_gray = gray[y:y+h, x:x+w]
# roi_color = img[y:y+h, x:x+w]

# cv2.imshow('img', img)
# cv2.waitKey(1000)
# cv2.destroyAllWindows()
  # ����ͼƬ
f_record.close()
