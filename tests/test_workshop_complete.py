# SPDX-License-Identifier: AGPL-3.0-only
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('workshop_complete', ROOT / 'tools/workshop_complete.py')
complete = importlib.util.module_from_spec(spec)
spec.loader.exec_module(complete)


class CompleteReleaseTests(unittest.TestCase):
    def fixture(self, directory):
        root = Path(directory)
        plugins = root / 'Plugins'
        inventory = json.loads((ROOT / 'tools/workshop_plugins.json').read_text())['bundles']
        for bundle in inventory:
            folder = plugins / bundle['folder']
            folder.mkdir(parents=True)
            (folder / 'plugin-meta.xml').write_text('<PLUGIN/>')
            (folder / 'LICENSE.txt').write_text('fixture license')
            (folder / 'resource.txt').write_text('required resource')
            for name in bundle['libraries']:
                data = bytearray(20)
                data[:6] = b'\x7fELF\x02\x01'
                data[18] = 183
                (folder / name).write_bytes(data)
        runtime = root / 'runtime.zip'
        with zipfile.ZipFile(runtime, 'w') as z:
            z.writestr('autorun/melodee/VERSION', 'v0.5.1-tapeworkshop-community.106 fixture')
            z.writestr('autorun/melodee/sample-base', b'exact OS bytes')
        sources = []
        for name in ('plugins-source.tar.gz', 'os-source.tar.gz', 'guides.zip'):
            path = root / name
            path.write_bytes(b'fixture source')
            sources.append(path)
        return plugins, inventory, runtime, sources, root / 'complete.zip'

    def test_all_plugins_resources_and_exact_os_are_bundled(self):
        with tempfile.TemporaryDirectory() as directory:
            plugins, inventory, runtime, sources, output = self.fixture(directory)
            count, _ = complete.complete(runtime, plugins, *sources, output)
            self.assertEqual(count, len(inventory))
            with zipfile.ZipFile(output) as z:
                self.assertEqual(z.read('autorun/melodee/sample-base'), b'exact OS bytes')
                for item in inventory:
                    self.assertEqual(z.read('Melodee/Plugins/' + item['folder'] + '/resource.txt'), b'required resource')
                self.assertEqual(len(json.loads(z.read('PLUGIN-INVENTORY.json'))['bundles']), len(inventory))
            self.assertTrue(output.with_suffix('.zip.sha256').exists())

    def test_missing_plugin_and_wrong_architecture_are_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            plugins, inventory, runtime, sources, output = self.fixture(directory)
            metadata = plugins / inventory[0]['folder'] / 'plugin-meta.xml'
            metadata.unlink()
            with self.assertRaisesRegex(ValueError, 'Missing plug-in metadata'):
                complete.complete(runtime, plugins, *sources, output)
            self.assertFalse(output.exists())
            metadata.write_text('<PLUGIN/>')
            binary = plugins / inventory[0]['folder'] / inventory[0]['libraries'][0]
            binary.write_bytes(b'not ARM64')
            with self.assertRaisesRegex(ValueError, 'Expected Linux ARM64'):
                complete.complete(runtime, plugins, *sources, output)

    def test_firmware_14_launcher_preserves_runtime_and_is_executable(self):
        with tempfile.TemporaryDirectory() as directory:
            plugins, _, runtime, sources, output = self.fixture(directory)
            launcher = Path(directory) / 'autorun.sh'
            launcher.write_bytes(b'#!/bin/sh\nexec /bin/sh autorun/autorun.sh\n')
            complete.complete(runtime, plugins, *sources, output, launcher)
            with zipfile.ZipFile(output) as z:
                self.assertEqual(z.read('autorun.sh'), launcher.read_bytes())
                self.assertEqual(z.read('autorun/melodee/sample-base'), b'exact OS bytes')
                self.assertTrue((z.getinfo('autorun.sh').external_attr >> 16) & 0o111)


if __name__ == '__main__':
    unittest.main()
