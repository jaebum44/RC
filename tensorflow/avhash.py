#!/usr/bin/env python

import os
os.environ['TF_CPP_MIN_LOG_LEVEL']='2' 
# https://github.com/tensorflow/tensorflow/issues/7778

import tensorflow as tf

from PIL import Image
import numpy as np

def average_hash(fname, size = 16):
	img = Image.open(fname)
	img = img.convert('L')
	img = img.resize((size, size), Image.ANTIALIAS)
	pixel_data = img.getdata()
	pixels = np.array(pixel_data)
	pixels = pixels.reshape((size, size))
	avg = pixels.mean()
	diff = 1 * (pixels > avg)
	return diff

def np2hash(n):
	bhash = []
	for nl in ahash.tolist():
		sl = [str(i) for i in nl]
		s2 = "".join(sl)
		i = int(s2, 2)
		bhash.append("%04x" % i)
	return "".join(bhash)

ahash = average_hash('tower.jpg')
print(ahash)
print(np2hash(ahash))
