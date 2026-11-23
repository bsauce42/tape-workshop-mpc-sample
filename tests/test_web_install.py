# SPDX-License-Identifier: AGPL-3.0-only
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class WebInstallTests(unittest.TestCase):
    def test_requests_match_verified_offer_and_cancel(self):
        with tempfile.TemporaryDirectory() as folder:
            binary=Path(folder)/'install'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror','-pthread','-I'+str(ROOT/'src'),str(ROOT/'tests/web_install.cpp'),str(ROOT/'src/melodee/shell/update.cpp'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True,timeout=20)
