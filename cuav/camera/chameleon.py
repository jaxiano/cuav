#!/usr/bin/env
'''
emulate a chameleon camera, getting images from a playback tool

The API is the same as the chameleon module, but takes images from fake_chameleon.pgm
'''

from . import libchameleon as chameleon
import time, os, sys, cv, numpy

from cuav.lib import cuav_util
from cuav.image import scanner

error = chameleon.error
image_height = 960
image_width = 1280

def open(colour, depth, brightness, height, width):
	return chameleon.open(colour, depth, brightness, height, width)

def trigger(h, continuous):
	chameleon.trigger(h, continuous)

def capture(h, timeout, img):
	img = numpy.zeros((image_height, image_width), dtype='uint8')
	frame_time, frame_counter, shutter = chameleon.capture(h, timeout, img)

	bgr = convertRawToBGR(img)
	return trigger_time, frame_counter, 0, bgr

def convertRawToBGR(img):
	global image_height, image_width
	bgr = numpy.zeros((image_height, image_width, 3), dtype='uint8')
	print 'chameleon::convertRawToBGR debayer'
	scanner.debayer(img, bgr)
	return bgr

def close(h):
	return chamelon.close(h)

def set_brightness(h, brightness):
	chameleon.set_brightness(h, brightness)

def set_gamma(h, gamma):
	chameleon.set_gamma(h, gamma)

def set_framerate(h, framerate):
	chameleon.set_framerate(h, framerate)

def save_pgm(filename, img):
	chameleon.save_pgm(filename, img)

def save_file(filename, bytes):
	chameleon.save_file(filename, bytes)
