#!/usr/bin/env python3

import sys
import os
import subprocess

os.chdir(os.path.join(os.path.dirname(sys.argv[0]), 'apitrace'))
if (not os.path.exists('build')):
    os.mkdir('build')
subprocess.run(['cmake',
                '-S', '.',
                '-GNinja',
                '-DCMAKE_BUILD_TYPE=Debug',
                '-DENABLE_WAFFLE=on',
                '-B', 'build'])
subprocess.run(['cmake', '--build', 'build'])
