#!/usr/bin/env
'''
emulate a chameleon camera, getting images from a playback tool

The API is the same as the chameleon module, but takes images from fake_chameleon.pgm
'''

from . import chameleon
import time, os, sys, cv, numpy

from cuav.camera.cam_params import CameraParams
from cuav.lib import cuav_util
from cuav.image import scanner

error = chameleon.error
config_file = 'cuav/data/tau.json'

continuous_mode = False
fake = 'cuav/tests/test-tau.png'
frame_counter = 0
trigger_time = 0
frame_rate = 7.5
chameleon_gamma = 950
last_frame_time = 0
image_height = 480
image_width = 640

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

def capture(h, timeout):
    print "tau::capture"
    global continuous_mode, trigger_time, frame_rate, frame_counter, fake, last_frame_time, image_height, image_width 
    print "tau::capture Calculate time of capture"
    tnow = time.time()
    due = trigger_time + (1.0/frame_rate)
    if tnow < due:
        time.sleep(due - tnow)
        timeout -= int(due*1000)
    # wait for a new image to appear
    print "tau::capture Resolving to realpath"
    filename = os.path.realpath(fake)

    try:
	print "tau::capture Allocating memory for bgr height:%i, width:%i, filename:%s" % (image_height, image_width, filename)
	bgr = numpy.zeros((image_height,image_width,3),dtype='uint8')
	print 'tau::capture calling convert_png_raw_to_bgr'
	scanner.png_raw_to_bgr(bgr, filename)
    except Exception, msg:
        raise chameleon.error('missing %s' % fake)
    print"tau::capture img shape height:%i,width%i" % (bgr.shape[0],bgr.shape[1])
    frame_counter += 1
    trigger_time = time.time()
    print "trigger_time:%i, frame_counter:%i" % (trigger_time, frame_counter) 
    return trigger_time, frame_counter, 0, bgr 

def close(h):
    print "tau::close"
    return

def set_gamma(h, gamma):
    global chameleon_gamma
    chameleon_gamma = gamma

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

def save_pgm(filename, bgr):
    mat = cv.GetMat(cv.fromarray(bgr))
    return cv.SaveImage(filename, mat)

def save_file(filename, bytes):
    return chameleon.save_file(filename, bytes)

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
