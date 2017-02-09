##############################################################################
## This file is part of 'SLAC Generic DAQ Software'
## It is subject to the license terms in the LICENSE.txt file found in the
## top-level directory of this distribution and at:
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
## No part of 'SLAC Generic DAQ Software', including this file,
## may be copied, modified, propagated, or distributed except according to
## the terms contained in the LICENSE.txt file.
##############################################################################
from distutils.core import setup, Extension

module1 = Extension('pythonDaq',
                    include_dirs = ['../generic/'],
                    libraries = ['z','m','rt'],
                    sources = ['pythonDaq.cpp'])

setup (name = 'PackageName',
       version = '1.0',
       description = 'This is a demo package',
       ext_modules = [module1])
