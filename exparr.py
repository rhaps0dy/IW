#!/usr/bin/env python
import sys
import numpy as np, matplotlib.pyplot as plt; a = np.array(eval(open(sys.argv[1]).read())); plt.imshow(np.clip(a, 0, 1), cmap='Greys_r'); plt.show()
