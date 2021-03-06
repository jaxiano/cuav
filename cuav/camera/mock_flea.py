#!/usr/bin/env
'''
emulate a chameleon camera, getting images from a playback tool

The API is the same as the chameleon module, but takes images from fake_chameleon.pgm
'''

import time, os, sys, cv, numpy, shutil

from cuav.camera.cam_params import CameraParams
from cuav.lib import cuav_util
from cuav.image import scanner

error = scanner.error
config_file = 'cuav/data/flea.json'

fake = 'cuav/tests/test-flea.pgm'
frame_counter = 0
trigger_time = 0
frame_rate = 7.5
last_frame_time = 0
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
    print 'Requested (%ix%i). Using (%i,%i)' % (width,height,image_width,image_height)
    return 0

def trigger(h, continuous):
    global continuous_mode, trigger_time
    continuous_mode = continuous
    trigger_time = time.time()


def load_fake_image(filename):
    print "Loading mock file: %s" % filename
    if filename.endswith('.pgm'):
	return cuav_util.PGM(filename)

    img = cuav_util.LoadImage(filename)
    array = numpy.ascontiguousarray(cv.GetMat(img))
    grey = numpy.zeros((image_height, image_width), dtype='uint8')
    scanner.rebayer(array, grey)
    return array
    
def load_image(filename):
    img = cuav_util.LoadImage(filename=filename)
    return numpy.asarray(cv.GetMat(img))

def capture(h, timeout):
    global continuous_mode, trigger_time, frame_rate, frame_counter, fake, last_frame_time
    tnow = time.time()
    due = trigger_time + (1.0/frame_rate)
    if tnow < due:
        time.sleep(due - tnow)
        timeout -= int(due*1000)
    # wait for a new image to appear
    filename = os.path.realpath(fake)
    pgm = load_fake_image(filename)
    bgr = None

    if continuous_mode:
    	try:
		print 'mock_flea::capture converting raw to bgr'
    		bgr = convertRawToBGR(pgm)
    	except Exception, msg:
        	raise scanner.error('missing %s' % fake)
    frame_counter += 1
    trigger_time = time.time()
    print 'mock_flea::capture returning data'
    return trigger_time, frame_counter, 0, bgr, bgr

def convertRawToBGR(pgm):
    global image_width, image_height
    raw = pgm.array
    print 'mock_flea::convertRawToBGR debayer'
    bgr = numpy.zeros((image_height, image_width, 3), dtype='uint8')
    scanner.debayer(raw, bgr)
    return bgr

def close(h):
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
    mat = cv.GetMat(cv.fromarray(raw))
    return cv.SaveImage(filename, mat)
    #shutil.copyfile(fake, filename)

def save_file(filename, bytes):
    return scanner.save_file(filename, bytes)

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

