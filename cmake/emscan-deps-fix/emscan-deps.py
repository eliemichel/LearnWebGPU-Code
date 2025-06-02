#!/usr/bin/env python3
# Copyright 2025 The Emscripten Authors.  All rights reserved.
# Emscripten is available under two separate licenses, the MIT license and the
# University of Illinois/NCSA Open Source License.  Both these licenses can be
# found in the LICENSE file.

"""emscan-deps - clang-scan-deps helper script

This script acts as a frontend replacement for clang-scan-deps.

NB: This is a patched version of emscan-deps that filters out the --use-port argument
"""

import sys
import os

# Patched: We add the original emsdk root to the import path
from os.path import join
sys.path.insert(0, join(os.environ["EMSDK"], "upstream", "emscripten"))

import emcc
from tools import shared

args = sys.argv[1:]
args = [ x for x in args if not x.startswith("--use-port") ] # Patched
args += emcc.get_cflags(tuple(args))
shared.exec_process([shared.CLANG_SCAN_DEPS] + args)
