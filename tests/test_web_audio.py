# SPDX-License-Identifier: AGPL-3.0-only
"""PCM format, timestamps, bounded overflow, no callback allocation and concurrent handoff."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class WebAudioTests(unittest.TestCase):
    def test_audio_feed(self):
        with tempfile.TemporaryDirectory() as folder:
            binary=Path(folder)/'web-audio'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror','-pthread',
                            '-I'+str(ROOT/'src'),str(ROOT/'tests/web_audio.cpp'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True,timeout=30)
