# SPDX-License-Identifier: AGPL-3.0-only
import re, struct, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class WebAssets(unittest.TestCase):
 def test_live_scripts_exist_in_device_bundle(self):
  with tempfile.TemporaryDirectory() as temp:
   out=Path(temp)/"web.bin"
   subprocess.run(["python3",str(ROOT/"tools/web_bundle.py"),str(out)],check=True,capture_output=True)
   blob=out.read_bytes();self.assertEqual(blob[:4],b"MLWB");at=4;files={}
   while at<len(blob):
    parts=[]
    for n in range(3):
     size=struct.unpack_from("<I",blob,at)[0];at+=4;parts.append(blob[at:at+size]);at+=size
    files[parts[0].decode()]=parts[2]
   self.assertIn("/display-only.js",files)
   for src in re.findall(r'<script[^>]+src="([^"?]+)',files["/live"].decode()):
    self.assertIn(src,files,src+" missing from device bundle")
   self.assertIn(b"/screen-frame",files["/display-only.js"])
