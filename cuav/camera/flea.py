#!/usr/bin/env
'''
emulate a chameleon camera, getting images from a playback tool

The API is the same as the chameleon module, but takes images from fake_chameleon.pgm
'''

import time, os, sys, cv, numpy

from cuav.camera.cam_params import CameraParams
from cuav.lib import cuav_util
from cuav.image import scanner
from cuav.camera import libflea as flea

error = scanner.error
config_file = 'cuav/data/flea.json'

image_height = 2048
image_width = 2448
continuous_mode = False

def load_camera_settings():
	global image_height, image_width

        c_params = CameraParams(lens=4.0, xresolution=0, yresolution=0)
        if os.path.exists(config_file):
            c_params.load(config_file)
	    image_height = c_params.yresolution
	    image_width = c_params.xresolution
            print("Loaded %s" % config_file)
        else:
            print("Warning: %s not found. Using default resolution height=%i and width=%i" % (config_file,image_height,image_width))

def get_resolution():
	return image_height, image_width

def open(colour, depth, brightness, height, width):
	global image_width, image_height
	
	if height > 0 and width > 0:
		image_height = height
		image_width = width
	
	print 'Requested (%ix%i). Using (%i,%i)' % (width,height,image_width,image_height)
	return flea.open(colour, depth, brightness, image_height, image_width)

def close(h):
	print 'flea::close'
	flea.close(h)    

def trigger(h, continuous):
	global continuous_mode

	print 'flea::trigger continuous_mode: %i' % continuous
	continuous_mode = continuous
	flea.trigger(h, continuous)

def capture(h, timeout):
	global continuous_mode, image_height, image_width
	img = numpy.zeros((image_height, image_width), dtype='uint8')
	frame_time, frame_counter, shutter = flea.capture(h, timeout, img)
	bgr = None
	if continuous_mode:
		print 'flea::capture In continuous mode...'
		bgr = convertRawToBGR(img)
		if bgr is None:
			print 'flea::capture Rats...' 
		print 'flea::capture Returning bgr'
	return frame_time, frame_counter, shutter, bgr
	
def convertRawToBGR(img):
	global image_width, image_height
	bgr = numpy.zeros((image_height, image_width, 3), dtype='uint8')
	print 'flea::convertRawToBGR debayer'
	try:
		scanner.debayer(img, bgr)
	except Exception, msg:
		print 'Exception: %s' % msg
	return bgr

def set_gamma(h, gamma):
	flea.set_gamma(h, gamma)

def get_gamma(h):
	return flea.get_gamma(h)

def get_brightness(h):
	return flea.get_brightness(h)

def get_auto_setting(h, settings):
	return flea.get_auto_setting(h, settings)

def set_framerate(h, framerate):
	flea.set_framerate(h, framerate)

def set_auto_exposure(h):
	flea.set_auto_exposure(h)

def set_auto_shutter(h):
	flea.set_auto_shutter(h)

def set_auto_gain(h):
	flea.set_auto_gain(h)

def set_brightness(h):
	flea.set_brightness(h)

def save_pgm(filename, bgr):
	#flea.save_pgm(filename, bgr)
	mat = cv.GetMat(cv.fromarray(bgr))
	return cv.SaveImage(filename, mat)

def save_file(filename, bytes):
	flea.save_file(filename, bytes)

load_camera_settings()
