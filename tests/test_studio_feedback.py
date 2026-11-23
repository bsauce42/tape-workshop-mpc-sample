# SPDX-License-Identifier: AGPL-3.0-only
from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class StudioFeedbackTests(unittest.TestCase):
    def test_ring(self):
        with tempfile.TemporaryDirectory() as d:
            out=Path(d)/'feedback'
            subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src'),'-I'+str(ROOT/'base/src'),str(ROOT/'tests/studio_feedback.cpp'),'-o',str(out)],check=True)
            subprocess.run([str(out)],check=True)
