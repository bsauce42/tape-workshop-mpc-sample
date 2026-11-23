# SPDX-License-Identifier: AGPL-3.0-only
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'tools'), str(ROOT/'base/tests')]
import workshop_install
import test_sd_update as base_tests

VERSION = 'v0.3.0-tapeworkshop-community.93'


def fixture(temp):
    f = base_tests.Fixture(temp)
    # Reuse the base transaction sandbox, but install our generated adapter.
    adapted = workshop_install.updater(f.public.read_text())
    sandboxed = f.script.read_text()
    for before, after in [('RUN=/run\n', 'RUN='), ('SYSTEMCTL=/bin/systemctl\n', 'SYSTEMCTL='),
                          ('OPENSSL=openssl\n', 'OPENSSL='), ('READY_SECONDS=30\n', 'READY_SECONDS=')]:
        line = next(x for x in sandboxed.splitlines(True) if x.startswith(after))
        adapted = workshop_install.replace_once(adapted, before, line)
    old = 'mv -Tf '
    # The only platform-specific command replacement in the base fixture.
    adapted = adapted.replace(old, str(f.root/'move-T')+' ')
    f.script.write_text(adapted)
    return f


@unittest.skipUnless(base_tests.TOOLS, 'requires OpenSSL 3, unzip and sha256sum')
class WorkshopInstallTests(unittest.TestCase):
    def test_own_signature_and_channel_required(self):
        with tempfile.TemporaryDirectory() as temp:
            f = fixture(temp)
            self.assertEqual(f.check(f.release(VERSION)).returncode, 0)
            self.assertNotEqual(f.check(f.release('v0.3.0')).returncode, 0)
            self.assertNotEqual(f.check(f.release(VERSION, app='demos')).returncode, 0)
            other = f.root/'other.pem'
            import subprocess
            subprocess.run([base_tests.OPENSSL,'genpkey','-algorithm','ed25519','-out',str(other)],check=True,capture_output=True)
            other_public = f.root/'other-public.pem'
            subprocess.run([base_tests.OPENSSL,'pkey','-in',str(other),'-pubout','-out',str(other_public)],check=True,capture_output=True)
            # Caller cannot select a different trusted key.
            self.assertNotEqual(f.check(f.release(VERSION,key=other),key=other_public).returncode,0)
            result,status=f.install(f.release(VERSION,tamper='autorun/melodee/sample-base'))
            self.assertEqual(status['error'],'damaged')
            self.assertFalse(f.calls.exists())

    def test_nested_install_preserves_boot_menu_and_other_apps(self):
        with tempfile.TemporaryDirectory() as temp:
            f = fixture(temp)
            f.installed('v0.3.0-tapeworkshop-community.88')
            top=f.card/'autorun'; nested=top/'apps/tape-workshop/autorun';nested.parent.mkdir(parents=True)
            top.rename(f.root/'old-autorun')
            nested.mkdir(parents=True)
            import shutil
            shutil.copytree(f.root/'old-autorun',nested,dirs_exist_ok=True)
            (top/'autorun.sh').write_text('# boot menu\n')
            other=top/'apps/other-os';other.mkdir();(other/'keep').write_text('unchanged')
            (f.old/'tape-workshop-autorun').write_text(str(nested)+'\n')
            result,status=f.install(f.release(VERSION))
            self.assertEqual(status['state'],'done',result.stderr)
            self.assertTrue((nested/'melodee/VERSION').read_text().startswith(VERSION))
            self.assertEqual((top/'autorun.sh').read_text(),'# boot menu\n')
            self.assertEqual((other/'keep').read_text(),'unchanged')

    def test_other_autorun_and_external_origin_rejected(self):
        for external in (False,True):
            with self.subTest(external=external),tempfile.TemporaryDirectory() as temp:
                f=fixture(temp);top=f.card/'autorun';top.mkdir();(top/'autorun.sh').write_text('# boot menu\n')
                if external:
                    outside=f.root/'external';outside.mkdir();(f.old/'tape-workshop-autorun').write_text(str(outside)+'\n')
                    # Early rejection must not write the card, even without a status file.
                    import subprocess
                    r=subprocess.run(['sh',str(f.script),'install',str(f.release(VERSION)),str(f.card),'melodee','melodee',str(f.public)],env={**os.environ,'FIX':str(f.root)},capture_output=True)
                    self.assertNotEqual(r.returncode,0)
                else:
                    _,status=f.install(f.release(VERSION));self.assertEqual(status['error'],'other-app')
                self.assertEqual((top/'autorun.sh').read_text(),'# boot menu\n');self.assertFalse(f.calls.exists())

    def test_standalone_install_and_self_test_failure(self):
        for fail in (False, True):
            with self.subTest(fail=fail), tempfile.TemporaryDirectory() as temp:
                f = fixture(temp)
                f.installed('v0.3.0-tapeworkshop-community.88')
                if fail:
                    (f.root/'self-test-fails').write_text('')
                result, status = f.install(f.release(VERSION))
                self.assertEqual(status['error'] if fail else status['state'],
                                 'self-test' if fail else 'done', result.stderr)
                expected = 'v0.3.0-tapeworkshop-community.88' if fail else VERSION
                self.assertTrue((f.card/'autorun/melodee/VERSION').read_text().startswith(expected))

    def test_nested_failed_start_rolls_back_only_nested_app(self):
        with tempfile.TemporaryDirectory() as temp:
            f=fixture(temp);f.installed('v0.3.0-tapeworkshop-community.88')
            top=f.card/'autorun';old=f.root/'old-autorun';top.rename(old)
            nested=top/'apps/tape/autorun';nested.mkdir(parents=True)
            import shutil
            shutil.copytree(old,nested,dirs_exist_ok=True)
            (top/'autorun.sh').write_text('# boot menu\n');(f.old/'tape-workshop-autorun').write_text(str(nested)+'\n')
            (f.root/'never-ready').write_text('')
            result,status=f.install(f.release(VERSION))
            self.assertEqual(status['error'],'no-start',result.stderr)
            self.assertTrue((nested/'melodee/VERSION').read_text().startswith('v0.3.0-tapeworkshop-community.88'))
            self.assertEqual((top/'autorun.sh').read_text(),'# boot menu\n')
