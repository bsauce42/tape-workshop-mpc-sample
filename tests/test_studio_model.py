# SPDX-License-Identifier: AGPL-3.0-only
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class StudioTests(unittest.TestCase):
    def test_transactional_edits(self):
        with tempfile.TemporaryDirectory() as folder:
            binary=Path(folder)/'studio'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src'),'-I'+str(ROOT/'base/src'),str(ROOT/'tests/studio_model.cpp'),str(ROOT/'src/melodee/model/pattern.cpp'),'-o',str(binary)],check=True)
            result=subprocess.run([str(binary)],capture_output=True,text=True,check=True)
            self.assertIn('PASS:',result.stdout)
