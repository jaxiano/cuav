import flea
import numpy

im=numpy.zeros((960, 1280), dtype='uint8')
h=flea.open(1,1,1)
flea.trigger(h, False)
ft, fc, s = flea.capture(h,0,im)
flea.save_pgm('fleatest.pgm', im)
flea.close(h)

