# SPDX-License-Identifier: AGPL-3.0-only
"""On-demand capture expiry, rate limiting, bounds and atomic frame publication."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class WebScreenTests(unittest.TestCase):
    def test_screen_cache(self):
        with tempfile.TemporaryDirectory() as folder:
            binary = Path(folder) / 'web-screen'
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-pthread',
                            '-I'+str(ROOT/'src'), str(ROOT/'tests/web_screen.cpp'), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True, timeout=20)
