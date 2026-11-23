# SPDX-License-Identifier: AGPL-3.0-only
from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class StudioMetersTests(unittest.TestCase):
    def test_peaks(self):
        with tempfile.TemporaryDirectory() as d:
            out=Path(d)/'meters'
            subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src'),str(ROOT/'tests/studio_meters.cpp'),'-o',str(out)],check=True)
            subprocess.run([str(out)],check=True)
