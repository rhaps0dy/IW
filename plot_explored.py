#!/usr/bin/env python
import matplotlib.pyplot as plt
import scipy.misc
import numpy as np
import sys

def pic_yx_to_ram_yx(pic_y, pic_x):
    ram_y = (pic_y/420.*84 - 15)*44./17.5 + 0x94
    ram_x = (pic_x/576.*84 - 15)*48./25 + 0x18
    return ram_y, ram_x

with open(sys.argv[1], 'r') as f:
    a = np.array(eval(f.read()))
img = scipy.misc.imread("montezuma_screen_1.png")[::-1]
stride = 2

for y in range(0, img.shape[0]-stride):
    for x in range(stride, img.shape[1]-stride):
        ry, rx = pic_yx_to_ram_yx(y+24, x)
        # Change to other values depending on the screen you want to plot
        # look at IW3OnlySearch.cpp in the first few lines
        rry = 256-int(round(ry))
        rrx = 160*4+int(round(rx))
        if a[rry:rry+stride, rrx:rrx+stride].any():
            img[y,x,:] = [0x2b, 0xdf, 0xff]

x1, y1 = pic_yx_to_ram_yx(24, 0)
x2, y2 = pic_yx_to_ram_yx(24+img.shape[0], img.shape[1])
extent=[x1,x2,y1,y2]

plt.imshow(img, origin='lower', extent=extent)
plt.show()
