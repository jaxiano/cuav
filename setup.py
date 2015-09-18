from distutils.core import Command
from setuptools import setup, Extension
import setuptools.command.build_py
import numpy as np
import os, sys, platform, shutil
import json

version = '1.3.4'

ext_modules = []
flea = None
tau = None
sensor = None

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


def init_flea():
   	flea = Extension('cuav.camera.libflea',
                          sources = ['cuav/camera/flea_py.c',
                                     'cuav/camera/flea_lib.c'],
                          libraries = ['flycapture-c'])
                          #,extra_compile_args=extra_compile_args + ['-o flea.so'])
   	ext_modules.append(flea)

def init_tau():
	tau = Extension('',
			sources = [],
			libraries = [])
	ext_modules.append(tau)

def init_ids():
	ids = Extension('',
			sources = [],
			libraries = [])
	ext_modules.append(ids)

def build_config(camera_type):
    global sensor

    dst = "cuav/modules/settings.py"
    sensor = camera_type

    print '====== camera_type: %s' % sensor 

    if camera_type == "tau":
        init_tau()
        src = "cuav/settings/settings_tau.py"
    elif camera_type == "ids":
        init_ids()
        src = "cuav/settings/settings_ids.py"
    else:
        init_flea()
        src = "cuav/settings/settings_flea.py"

    shutil.copyfile(src, dst)

class BuildPyCommand(setuptools.command.build_py.build_py):
    def run(self):
	global sensor

	if sensor is None:
		self.run_command('flea')
	setuptools.command.build_py.build_py.run(self)

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

class IDS(Command):
   user_options=[]
   def initialize_options(self):
	pass
   def finalize_options(self):
	pass
   def run(self):
	build_config("ids")

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
				    'tests/test-flea.pgm',
				    'tests/test-tau.png',
                    'tests/test-ids.png',
                    'data/chameleon1_arecont0.json',
                    'data/flea.json',
                    'data/ids.json',
                    'camera/include/*.h']},
        ext_modules = ext_modules,
	cmdclass={
		'flea':Flea
		,'tau':Tau
        ,'ids':IDS
		,'build_py':BuildPyCommand
	}
)

