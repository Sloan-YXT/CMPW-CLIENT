# -*- coding: utf-8 -*-
"""
Created on Wed Jun 22 20:59:21 2016

@author: Administrator
"""

# -*- coding: utf-8 -*

import numpy as np
import cv2
import sys
import os
# Ҫʹ��Haar cascadeʵ�֣�����Ҫ�ѿ��޸�Ϊlbpcascade_frontalface.xml
# cv2.data.haarcascades
#face_cascade = cv2.CascadeClassifier('lbpcascade_frontalface.xml')
face_cascade = cv2.CascadeClassifier(
   "/home/pi/opencv/data/haarcascades/haarcascade_frontalface_default.xml"
)
gpath = sys.argv[1]
fpath = sys.argv[2]
for roots,dirs,files in os.walk(gpath):
    for file in files:

        img = cv2.imread(roots+file)
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
# ʶ������ͼƬ�е���������.���ض���ľ��γߴ�
# ����ԭ��detectMultiScale(gray, 1.2,3,CV_HAAR_SCALE_IMAGE,Size(30, 30))
# gray��Ҫʶ���ͼƬ
# 1.03����ʾÿ��ͼ��ߴ��С�ı���
# 5����ʾÿһ��Ŀ������Ҫ����⵽4�β��������Ŀ��(��Ϊ��Χ�����غͲ�ͬ�Ĵ��ڴ�С�����Լ�⵽����)
# CV_HAAR_SCALE_IMAGE��ʾ�������ŷ���������⣬��������ͼ��Size(30, 30)ΪĿ�����С���ߴ�
# faces����ʾ��⵽������Ŀ������
        faces = face_cascade.detectMultiScale(gray, 1.03, 5)
        for (x, y, w, h) in faces:
            if w+h > 200:  # //������ͼƬ�����������
            img2 = cv2.rectangle(img, (x, y), (x+w, y+h), (255, 255, 255), 4)
        #roi_gray = gray[y:y+h, x:x+w]
        #roi_color = img[y:y+h, x:x+w]

#cv2.imshow('img', img)
            cv2.waitKey(1000)
            cv2.destroyAllWindows()
            cv2.imwrite(fpath+file, img)  # ����ͼƬ

