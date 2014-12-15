from distutils.core import Command
from setuptools import setup, Extension
import numpy as np
import os, sys, platform
import json

version = '1.3.3'

ext_modules = []
chameleon = None
flea = None

if platform.system() == 'Windows':
    extra_compile_args=["-std=gnu99", "-O3"]
else:
    if platform.machine().find('arm') != -1:
        extra_compile_args=["-std=gnu99", "-O3", "-mfpu=neon"]
    else:
        extra_compile_args=["-std=gnu99", "-O3"]

scanner = Extension('cuav.image.scanner',
                    sources = ['cuav/image/scanner.c', 'cuav/image/imageutil.c'],
		    libraries = ['jpeg'],
                    extra_compile_args=extra_compile_args)
ext_modules.append(scanner)


def build_config(camera_type):
   jsonFile = open("cuav/data/build_config.json", "r")
   data = json.load(jsonFile)
   jsonFile.close()

   data["camera"] = camera_type

   jsonFile = open("cuav/data/build_config.json", "w+")
   jsonFile.write(json.dumps(data))
   jsonFile.close()

def init_chameleon():
    	chameleon = Extension('cuav.camera.chameleon',
                          sources = ['cuav/camera/chameleon_py.c',
                                     'cuav/camera/chameleon.c',
                                     'cuav/camera/chameleon_util.c'],
                          libraries = ['dc1394', 'm', 'usb-1.0'],
                          extra_compile_args=extra_compile_args + ['-O0'])
    	ext_modules.append(chameleon)
	
class Chameleon(Command):
   user_options=[]
   def initialize_options(self):
	pass
   def finalize_options(self):
	pass
   def run(self):
	build_config("chameleon")
	init_chameleon()

def init_flea():
   	flea = Extension('cuav.camera.flea',
                          sources = ['cuav/camera/flea_py.c',
                                     'cuav/camera/flea_lib.c'],
                          libraries = ['flycapture-c'])
                          #,extra_compile_args=extra_compile_args + ['-o flea.so'])
   	ext_modules.append(flea)
	
class Flea(Command):
   user_options=[]
   def initialize_options(self):
	pass
   def finalize_options(self):
	pass
   def run(self):
	build_config("flea")
	init_flea()

build_config("chameleon")
init_chameleon()

setup (name = 'cuav',
        zip_safe=True,
        version = version,
        description = 'CanberraUAV UAV code',
        long_description = '''A set of python libraries and tools developed by CanberraUAV for the Outback Challenge. This includes an image search algorithm with optimisation for ARM processors and a number of mission planning and analysis tools.''',
        url = 'https://github.com/CanberraUAV/cuav',
        author = 'CanberraUAV',
        install_requires = [ 'pymavlink',
                            'MAVProxy' ],
        author_email = 'andrew-cuav@tridgell.net',
        classifiers=['Development Status :: 4 - Beta',
                    'Environment :: Console',
                    'Intended Audience :: Science/Research',
                    'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
                    'Operating System :: OS Independent',
                    'Programming Language :: Python :: 2.7',
                    'Topic :: Scientific/Engineering'
                    ],
        license='GPLv3',
        include_dirs = [np.get_include(),
                       'cuav/camera/include',
			'cuav/camera/flea/include',
			'/usr/include/flycapture'],
        packages = ['cuav', 'cuav.lib', 'cuav.image', 'cuav.camera', 'cuav.uav', 'cuav.modules'],
        scripts = [ 'cuav/tools/geosearch.py', 'cuav/tools/geotag.py',
                   'cuav/tools/cuav_lens.py', 'cuav/tools/agl_mission.py',
                   'cuav/tools/pgm_convert.py',
                   'cuav/tests/cuav_benchmark.py' ],
        package_data = { 'cuav' : [ 'tests/test-8bit.pgm',
                                   'data/chameleon1_arecont0.json',
				   'data/build_config.json',
                                   'camera/include/*.h']},
        ext_modules = ext_modules,
	cmdclass={
		'flea':Flea
		#,'chameleon':Chameleon
	}
)

