#!/usr/bin/env python
import sys
import numpy as np
import matplotlib.pyplot as plt
import scipy.misc

img = scipy.misc.imread("/Users/adria/Documents/TFG/report/img/montezuma_all.png")[::-1]

def pic_yx_to_ram_yx(pic_y, pic_x):
    ram_y = (pic_y/420.*84 - 15)*44./17.5 + 0x94
    ram_x = (pic_x/576.*84 - 15)*48./25 + 0x18
    return ram_y, ram_x

WIDTH, HEIGHT = 128, 160
PIC_H, PIC_W = 296, 576

#colors = np.array([[1,0,0],[1,0,1],[0,1,1],[0,1,0]])
colors = np.ones((4,3))
visited = np.array(list(eval(open(a).read()) for a in sys.argv[1:]))
assert colors.shape[0] >= visited.shape[0], "Not enough colors for all exparrs"
visited = np.clip(visited, 0, 1)
counts = np.sum(visited, axis=0)
visited_img = np.ones(visited.shape[1:] + (3,))
for v in xrange(visited.shape[0]):
    for y in xrange(visited.shape[1]):
        for x in xrange(visited.shape[2]):
            if visited[v,y,x]:
                if visited_img[y,x].sum() == 3:
                    visited_img[y,x,:] = 0
                visited_img[y,x] += colors[v]
visited_img /= counts.clip(1,visited.shape[0]).reshape(counts.shape + (1,)).repeat(3,2)
#colors = colors[:visited.shape[0]]
#colors = colors.reshape((visited.shape[0],) + (1,1) +
#                        (colors.shape[1],)) \
#               .repeat(visited.shape[1], 1) \
#               .repeat(visited.shape[2], 2)
#visited = visited.reshape(visited.shape + (1,)) \
#                 .repeat(colors.shape[3], 3)
#counts = counts.reshape((1,) + counts.shape + (1,)) \
#               .repeat(visited.shape[0], 0) \
#               .repeat(visited.shape[3], 3)

#visited_img = np.sum(visited * colors / counts, axis=0)
plt.imshow(visited_img)
plt.show()

#for gy in xrange(img.shape[0]):
#    ly = gy % PIC_H
#    iy = (gy // PIC_H) * HEIGHT
#    for gx in xrange(img.shape[1]):
#        lx = gx % PIC_W
#        ix = (gx // PIC_W) * WIDTH
#        y, x = pic_yx_to_ram_yx(ly, lx)
#        y += yi
#        x += xi
#        if exparr
#

