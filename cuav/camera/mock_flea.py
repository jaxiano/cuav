#!/usr/bin/env
'''
emulate a chameleon camera, getting images from a playback tool

The API is the same as the chameleon module, but takes images from fake_chameleon.pgm
'''

from . import chameleon
import time, os, sys, cv, numpy

from cuav.lib import cuav_util
from cuav.image import scanner

error = chameleon.error
continuous_mode = False
fake = 'cuav/tests/test-8bit.pgm'
frame_counter = 0
trigger_time = 0
frame_rate = 7.5
chameleon_gamma = 950
last_frame_time = 0
image_height = 960
image_width = 1280

def open(colour, depth, brightness, height, width):
    image_height = height
    image_width = width
    return 0

def trigger(h, continuous):
    global continuous_mode, trigger_time
    continuous_mode = continuous
    trigger_time = time.time()


def load_image(filename):
    if filename.endswith('.pgm'):
        fake_img = cuav_util.PGM(filename)
        return fake_img.array
    img = cv.LoadImage(filename)
    array = numpy.asarray(cv.GetMat(img))
    grey = numpy.zeros((image_height, image_width), dtype='uint8')
    scanner.rebayer(array, grey)
    return grey
    

def capture(h, timeout, img):
    global continuous_mode, trigger_time, frame_rate, frame_counter, fake, last_frame_time
    tnow = time.time()
    due = trigger_time + (1.0/frame_rate)
    if tnow < due:
        time.sleep(due - tnow)
        timeout -= int(due*1000)
    # wait for a new image to appear
    filename = os.path.realpath(fake)

    try:
        fake_img = load_image(filename)
    except Exception, msg:
        raise chameleon.error('missing %s' % fake)
    frame_counter += 1
    img.data = fake_img.data
    trigger_time = time.time()
    return trigger_time, frame_counter, 0

def close(h):
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

def save_pgm(filename, img):
    return chameleon.save_pgm(filename, img)

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
