#!/usr/bin/env python
import sys
import numpy as np
import matplotlib.pyplot as plt

a = np.array(eval(open(sys.argv[1]).read()))
plt.imshow(1-np.clip(a, 0, 1), cmap='Greys_r')
plt.savefig(sys.argv[1][:-3]+'png')
#plt.show()
