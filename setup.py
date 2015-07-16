from distutils.core import Command
from setuptools import setup, Extension
import numpy as np
import os, sys, platform, shutil
import json

version = '1.3.4'

ext_modules = []
chameleon = None
flea = None
tau = None

if platform.system() == 'Windows':
    extra_compile_args=["-std=gnu99", "-O3"]
else:
    if platform.machine().find('arm') != -1:
        extra_compile_args=["-std=gnu99", "-O3", "-mfpu=neon"]
    else:
        extra_compile_args=["-std=gnu99", "-O3"]

scanner = Extension('cuav.image.scanner',
                    sources = ['cuav/image/scanner.c', 'cuav/image/imageutil.c', 'cuav/image/pic.c', 'cuav/image/pnm.c', 'cuav/image/pngutil.c'],
		    library_dirs = ['/usr/local/libpng-1.6.17/lib'],
		    libraries = ['jpeg','png'],
                    extra_compile_args=extra_compile_args)
ext_modules.append(scanner)


def init_chameleon():
    	chameleon = Extension('cuav.camera.chameleon',
                          sources = ['cuav/camera/chameleon_py.c',
                                     'cuav/camera/chameleon.c',
                                     'cuav/camera/chameleon_util.c'],
                          libraries = ['dc1394', 'm', 'usb-1.0'],
                          extra_compile_args=extra_compile_args + ['-O0'])
    	ext_modules.append(chameleon)

def init_flea():
   	flea = Extension('cuav.camera.flea',
                          sources = ['cuav/camera/flea_py.c',
                                     'cuav/camera/flea_lib.c'],
                          libraries = ['flycapture-c'])
                          #,extra_compile_args=extra_compile_args + ['-o flea.so'])
   	ext_modules.append(flea)

def init_tau():
	tau = Extension('cuav.camera.tau')
	ext_modules.append(tau)

def build_config(camera_type):
    dst = "cuav/modules/settings.py"

    if camera_type == "flea":
        init_flea()
        src = "cuav/settings/settings_flea.py"
    elif camera_type == "tau":
	init_flea()
	src = "cuav/settings/settings_tau.py"
    else:
        init_chameleon()
        src = "cuav/settings/settings_chameleon.py"

    shutil.copyfile(src, dst)

class Chameleon(Command):
   user_options=[]
   def initialize_options(self):
	pass
   def finalize_options(self):
	pass
   def run(self):
	build_config("chameleon")

class Flea(Command):
   user_options=[]
   def initialize_options(self):
	pass
   def finalize_options(self):
	pass
   def run(self):
	build_config("flea")

class Tau(Command):
   user_options=[]
   def initialize_options(self):
	pass
   def finalize_options(self):
	pass
   def run(self):
	build_config("tau")

build_config("chameleon")

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
			'/usr/include/flycapture',
			'/usr/local/libpng-1.6.17/include'],
        packages = ['cuav', 'cuav.lib', 'cuav.image', 'cuav.camera', 'cuav.uav', 'cuav.modules'],
        scripts = [ 'cuav/tools/geosearch.py', 'cuav/tools/geotag.py',
                   'cuav/tools/cuav_lens.py', 'cuav/tools/agl_mission.py',
                   'cuav/tools/pgm_convert.py',
                   'cuav/tests/cuav_benchmark.py' ],
        package_data = { 'cuav' : [ 'tests/test-8bit.pgm',
				    'tests/test-tau.pnm',
                                   'data/chameleon1_arecont0.json',
				   'data/flea.json',
                                   'camera/include/*.h']},
        ext_modules = ext_modules,
	cmdclass={
		'flea':Flea
		#'tau':Tau
		#,'chameleon':Chameleon
	}
)

