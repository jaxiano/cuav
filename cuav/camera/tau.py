#!/usr/bin/env
'''
emulate a chameleon camera, getting images from a playback tool

The API is the same as the chameleon module, but takes images from fake_chameleon.pgm
'''

import __builtin__
from . import sightline
import time, os, sys, cv, numpy, glob

from cuav.camera.cam_params import CameraParams
from cuav.lib import cuav_util
from cuav.image import scanner

try:
	import cv2.cv as cv
except ImportError:
	import cv

error = scanner.error
config_file = 'cuav/data/tau.json'

raw_png = 'tau/images/raw'
raw_ftp_base_path = '/data/'
raw_png_search_path = raw_ftp_base_path + raw_png + '*'
frame_counter = 0
trigger_time = 0
frame_rate = 7.5
last_frame_time = 0
image_height = 480
image_width = 640
continuous_mode = False
tau_ip = '192.168.168.204'
odroid_ip = '192.168.168.203'

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
    print 'Requested (%ix%i). Using (%i,%i)' % (width,height,image_width,image_height)
    return 0

def trigger(h, continuous):
    print "tau::trigger"
    global continuous_mode, trigger_time
    continuous_mode = continuous
    trigger_time = time.time()


# 640x480 16-bit PNG
def load_image(filename):
    print "tau::load_image"
    bgr = numpy.zeros((image_height, image_width, 3), dtype='uint8')
    scanner.png_raw_to_bgr(bgr, filename)
    return bgr

def request_image():
    print 'Initializing Sightline Connector' 
    con = sightline.Connector(tau_ip, sightline.print_output)
    con.start()

    buf = sightline.BaseCommand().get_version()
    con.send(buf)
    ss = sightline.Snapshot(odroid_ip, 'anonymous', 'odroid')
    con.send(ss.set_parameters())
    con.send(ss.do_snapshot(raw_png))

def capture(h, timeout):
    print "tau::capture"
    global continuous_mode, trigger_time, frame_rate, frame_counter, fake, last_frame_time, image_height, image_width,  raw_png 
    print "tau::capture Calculate time of capture"
    tnow = time.time()
    due = trigger_time + (1.0/frame_rate)
    if tnow < due:
        time.sleep(due - tnow)
        timeout -= int(due*1000)

    # wait for a new image to appear
    bgr = None
    raw = None
    if continuous_mode:
    	try:
		counter = 0
		request_image()
		available = False
		raw_png_path = None
		while True:
			print 'waiting for image...'
			files = glob.glob(raw_png_search_path)
			for file in sorted(files):
				filesize = os.path.getsize(file)
				print 'file: %s, filesize: %i' % (file, filesize)
				# uncompressed PNG
				if filesize == 616112L:
					raw_png_path = file
					available = True
					break

			if available:
				break

			counter += 1
			time.sleep(1)
			if counter == 5:
				counter = 0
				request_image()
		
		print "tau::capture Allocating memory for bgr height:%i, width:%i, filename:%s" % (image_height, image_width, raw_png_path)
    		bgr = numpy.zeros((image_height, image_width, 3), dtype='uint8')
    		print"tau::capture img shape height:%i,width%i" % (bgr.shape[0],bgr.shape[1])
		print 'tau::capture calling convert_png_raw_to_bgr'
		scanner.png_raw_to_bgr(bgr, raw_png_path)
		raw = read_binary(raw_png_path)
		os.remove(raw_png_path)
    	except Exception, msg:
        	raise scanner.error('missing %s' % raw_png_path)

    frame_counter += 1
    trigger_time = time.time()
    print "trigger_time:%i, frame_counter:%i" % (trigger_time, frame_counter) 
    return trigger_time, frame_counter, 0, bgr, raw 

def close(h):
    print "tau::close"
    return

def set_gamma(h, gamma):
    pass

def set_framerate(h, framerate):
    global frame_rate
    if framerate >= 15:
        frame_rate = 15
    elif framerate >= 7:
        frame_rate = 7.5
    elif framerate >= 3:
        frame_rate = 3.75
    else:
        frame_rate = 1.875;

def save_pgm(filename, raw):
    #mat = cv.GetMat(cv.fromarray(bgr))
    #return cv.SaveImage(filename, mat)
    with __builtin__.open(filename, 'wb') as file:
    	file.write(raw)

def save_file(filename, bytes):
    return scanner.save_file(filename, bytes)

def read_binary(filename):
     with __builtin__.open(filename, 'rb') as file:
     	return file.read()

def set_brightness(h):
    pass

def set_auto_exposure(h):
    pass

def set_auto_shutter(h):
    pass

def set_auto_gain(h):
    pass

def get_gamma(h):
    pass

def get_brightness(h):
    pass

def get_auto_setting(h, settings):
    return [0,0,0]

load_camera_settings()
