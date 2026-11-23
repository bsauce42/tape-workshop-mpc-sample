# SPDX-License-Identifier: AGPL-3.0-only
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
spec = importlib.util.spec_from_file_location('installer', Path(__file__).resolve().parents[1]/'tools/install_plugins.py')
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)

class PluginInstallerTests(unittest.TestCase):
    def test_copy_checks_and_os_preservation(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)
            card = p/'card'
            (card/'autorun').mkdir(parents=True)
            (card/'autorun/keep').write_text('unchanged')
            h = bytearray(20)
            h[:6] = b'\x7fELF\x02\x01'
            struct.pack_into('<HH', h, 16, 3, 183)
            src = p/'test.clap'
            src.write_bytes(h)
            installer.install(card, [src], True)
            self.assertFalse((card/'Melodee').exists())
            installer.install(card, [src])
            self.assertEqual((card/'Melodee/Plugins/test.clap').read_bytes(), h)
            with self.assertRaises(ValueError):
                installer.install(card, [src])
            struct.pack_into('<H', h, 18, 62)
            bad = p/'pc.clap'
            bad.write_bytes(h)
            with self.assertRaises(ValueError):
                installer.install(card, [bad])
            self.assertEqual((card/'autorun/keep').read_text(), 'unchanged')
            source = p/'valid.clap'
            struct.pack_into('<H', h, 18, 183)
            source.write_bytes(h)
            link = p/'linked.clap'
            link.symlink_to(source)
            with self.assertRaises(ValueError):
                installer.install(card, [link])
            folder = p/'other-card'
            folder.mkdir()
            (folder/'Melodee').symlink_to(card/'Melodee')
            with self.assertRaises(ValueError):
                installer.install(folder, [source])

if __name__ == '__main__':
    unittest.main()
