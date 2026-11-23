# SPDX-License-Identifier: AGPL-3.0-only
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class WebControlsTests(unittest.TestCase):
    def test_physical_controls(self):
        with tempfile.TemporaryDirectory() as folder:
            binary = Path(folder) / 'controls'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror',
                            '-I'+str(ROOT/'src'),'-I'+str(ROOT/'base/src'),str(ROOT/'tests/web_controls.cpp'),'-o',str(binary)],check=True)
            a,b = [json.loads(line) for line in subprocess.check_output([str(binary)],text=True).splitlines()]
            self.assertEqual(a['buttons'][4],[1,50])
            self.assertEqual(a['controls'][3],[91,40])
            self.assertEqual(a['controls'][1],[-1,60000])
            self.assertEqual(a['pads'][15],[0,20]) # short press remains visible after release
            self.assertEqual(a['wheel'],-2)
            self.assertEqual(b['controls'][1],[42,60000])
            self.assertEqual(a['pad_hits'][15],1)
            self.assertEqual(b['pad_hits'][15],2)
            self.assertEqual(sum(b['pad_hits']),2)
            self.assertEqual(len(b['buttons']),24)
            self.assertEqual(len(b['pads']),16)
