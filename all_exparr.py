#!/usr/bin/env python
import sys
import numpy as np
import matplotlib.pyplot as plt
import scipy.misc
import os

#img = scipy.misc.imread(
#    os.path.join(os.path.dirname(os.path.realpath(__file__)),
#    '../report-TFG/img/montezuma_all.png'))[:,:,:3]
#
#facs = np.ones(img.shape)
#facs[:,:,0] = 0.2126
#facs[:,:,1] = 0.7152
#facs[:,:,2] = 0.0722
#img = np.sum(facs*img, axis=2)
#img = img.reshape(img.shape + (1,)).repeat(3,axis=2)


HEIGHT, WIDTH = 128, 160
PIC_H, PIC_W = 296, 576

#with open('action_reward_montezuma_iw.1') as f:
#    i = 0
#    joe_positions = []
#    for l in f.readlines():
#        if i == 0:
#            x, y = l.strip().split()
#            x = int(x) + 3*WIDTH
#            y = 2*HEIGHT - int(y)
#            joe_positions.append((x, y))
#        i = (i+1)%3

def pic_y_to_ram_y(pic_y):
    return int(round((pic_y/420.*84 - 15)*44./17.5 + 0x94))
def pic_x_to_ram_x(pic_x):
    return int(round((pic_x/576.*84 - 15)*48./25 + 0x18))

colors = ["e9803a", "6082e6", "002598", "cc3428", "ffffff", "deaded", "995fad",
          "e98fa4", "86f456", "47a41e", "efde4d", "10e2bd", "be447c"]
def convert(col):
    return list(int(col[i:i+2], 16) for i in xrange(0,6,2))
colors = np.array(list(convert(c) for c in colors))
col_out = np.zeros((10, 10*colors.shape[0], 3))
for i in range(colors.shape[0]):
    col_out[:,i*10:i*10+10] = colors[i]
scipy.misc.imsave('colors.png', col_out)


visited = np.array(list(eval(open(a).read()) for a in sys.argv[1:]))
img = np.ones(visited.shape[1:]+(3,))*255
assert colors.shape[0] >= visited.shape[0], "Not enough colors for all exparrs"
#visited = np.clip(visited, 0, 1)
#counts = np.sum(visited, axis=0)
#visited_img = np.zeros(visited.shape[1:] + (3,), dtype=np.uint8)
#print visited_img.shape
#for v in xrange(visited.shape[0]):
#    for y in xrange(visited.shape[1]):
#        for x in xrange(visited.shape[2]):
#            if visited[v,y,x] and not np.any(visited[:v,y,x]):
#                visited_img[y,x,:] = colors[v,:]



#visited_img /= counts.clip(1,visited.shape[0]).reshape(counts.shape + (1,)).repeat(3,2)
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
#plt.imshow(visited_img)
#plt.show()

STRIDE=2
for gy in xrange(img.shape[0]):
#    ly = gy % PIC_H
#    iy = (gy // PIC_H) * HEIGHT
#    y = pic_y_to_ram_y(ly) - 128 + iy
    y = gy
    if y >= 512:
        break
    for gx in xrange(img.shape[1]):
#        lx = gx % PIC_W
#        ix = (gx // PIC_W) * WIDTH
#        x = pic_x_to_ram_x(lx) + 2 + ix
        x = gx
        for v in range(visited.shape[0]):
            #xx = (joe_positions[v][0]-x)
            #yy = (joe_positions[v][1]-y)
            #if visited[v,y,x] or xx*xx*2+yy*yy < 100:
            if visited[v,y,x]:
                img[gy-STRIDE:gy+STRIDE+1,gx-STRIDE:gx+STRIDE+1,:] = colors[v]
                break
#scipy.misc.imsave('exparr.png', img)
plt.imshow(img)
plt.show()
