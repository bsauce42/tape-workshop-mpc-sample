# SPDX-License-Identifier: AGPL-3.0-only
import importlib.util
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
CRYPTO = importlib.util.find_spec('cryptography') is not None
if CRYPTO:
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
    from cryptography.hazmat.primitives import serialization
    import workshop_release

@unittest.skipUnless(CRYPTO,'release signing tests require cryptography')
class WorkshopReleaseTests(unittest.TestCase):
    def fixture(self, root, key):
        package=root/'package'
        for name in ['sample-base','restore-stock.sh','sd-boot.sh','sd-worker.sh','sd-update.sh','SHA256SUMS']:
            p=package/'autorun/melodee'/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'checked fixture')
        folder = package/'autorun/melodee'
        (folder/'voice').mkdir()
        (folder/'voice/decoder.so').write_bytes(b'fixture voice decoder')
        (folder/'SHA256SUMS').write_text(''.join(hashlib.sha256((folder/name).read_bytes()).hexdigest() + '  ' + name + '\n'
            for name in ('sample-base', 'restore-stock.sh', 'sd-boot.sh', 'sd-worker.sh', 'sd-update.sh')))
        (package/'autorun/melodee/app').write_bytes(b'melodee\n')
        (package/'autorun/melodee/VERSION').write_bytes(b'v0.3.0-tapeworkshop-community.79 fixture\n')
        for name in ['autorun.sh','autorun.json','autorun.check']:(package/'autorun'/name).write_bytes(b'fixture')
        path=root/'key.pem';path.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
        return package,path
    def test_signed_zip_and_tampered_file(self):
        key=Ed25519PrivateKey.generate()
        with tempfile.TemporaryDirectory() as folder,patch.object(workshop_release,'public_key',return_value=key.public_key()):
            root=Path(folder);package,path=self.fixture(root,key);out=root/'release.zip'
            self.assertEqual(workshop_release.sign(package,out,path),'v0.3.0-tapeworkshop-community.79')
            self.assertEqual(workshop_release.verify(out),'v0.3.0-tapeworkshop-community.79')
            with zipfile.ZipFile(out) as z:files={n:z.read(n) for n in z.namelist()}
            files['autorun/melodee/sample-base']=b'tampered'
            with zipfile.ZipFile(root/'bad.zip','w') as z:
                for n,data in files.items():z.writestr(n,data)
            with self.assertRaisesRegex(ValueError,'Damaged file'):workshop_release.verify(root/'bad.zip')
    def test_wrong_key_refused_and_paths_checked(self):
        key=Ed25519PrivateKey.generate()
        with tempfile.TemporaryDirectory() as folder,patch.object(workshop_release,'public_key',return_value=Ed25519PrivateKey.generate().public_key()):
            root=Path(folder);package,path=self.fixture(root,key)
            with self.assertRaisesRegex(ValueError,'does not match'):workshop_release.sign(package,root/'release.zip',path)
        for name in ['../evil','autorun/../evil','autorun//bad','autorun/.bad/../bad','autorun/bad path']:
            self.assertFalse(workshop_release.safe(name))
        self.assertTrue(workshop_release.safe('autorun/melodee/sample-base'))
