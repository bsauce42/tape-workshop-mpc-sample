# SPDX-License-Identifier: AGPL-3.0-only
"""Tape Workshop install adapters; base templates remain untouched.

Preserves the base install transaction, self-test and rollback. Pins this
fork's key/channel, and uses the launching package's recorded autorun path.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(text, before, after):
    if text.count(before) != 1:
        raise ValueError('Installer template changed: ' + before[:70])
    return text.replace(before, after, 1)


def worker():
    text = (ROOT/'base/install/sd-worker.sh').read_text()
    return replace_once(text, 'chmod 755 "$RELEASE/sample-base"',
        '# Record the actual package, including boot-menu installations.\n'
        'origin=$(CDPATH= cd -- "$DIR/.." && pwd -P)\n'
        'printf \'%s\\n\' "$origin" > "$RELEASE/tape-workshop-autorun"\n'
        'chmod 755 "$RELEASE/sample-base"')


def updater(public_key):
    if not public_key.startswith('-----BEGIN PUBLIC KEY-----\n'):
        raise ValueError('Expected public key PEM')
    text = (ROOT/'base/install/sd-update.sh').read_text()
    text = replace_once(text, '    "$OPENSSL" pkeyutl -verify -pubin -inkey "$3"',
        '    [ "$2" = melodee ] || return 1\n'
        '    cat > "$4/trusted.pem" <<\'TAPE_WORKSHOP_PUBLIC_KEY\'\n' + public_key +
        'TAPE_WORKSHOP_PUBLIC_KEY\n'
        '    "$OPENSSL" pkeyutl -verify -pubin -inkey "$4/trusted.pem"')
    text = replace_once(text, '    echo "$v"',
        '    printf \'%s\\n\' "$v" | grep -Eq \'^v[0-9]+\\.[0-9]+\\.[0-9]+-tapeworkshop-community\\.[0-9]+$\' || return 1\n'
        '    echo "$v"')
    text = replace_once(text, '    version_of "$work/zip/autorun/$2/VERSION"',
        '    take "$1" "autorun/$2/app" "$work" || exit 1\n'
        '    listed "$work" "autorun/$2/app" || exit 1\n'
        '    [ "$(cat "$work/zip/autorun/$2/app")" = melodee ] || exit 1\n'
        '    version_of "$work/zip/autorun/$2/VERSION"')
    text = replace_once(text, '    AUTORUN=$CARD/autorun\n    TARGET=$AUTORUN/$PACKAGE',
        '    AUTORUN=$CARD/autorun\n'
        '    origin_bad=\n'
        '    # Only the active app\'s recorded location may redirect this transaction.\n'
        '    # Canonicalize both paths and retain the existing startup ownership check.\n'
        '    if [ -f "$ACTIVE/tape-workshop-autorun" ]; then\n'
        '        origin=$(cat "$ACTIVE/tape-workshop-autorun")\n'
        '        card_real=$(CDPATH= cd -- "$CARD" && pwd -P)\n'
        '        origin_real=$(CDPATH= cd -- "$origin" && pwd -P) || origin_bad=1\n'
        '        case "$origin_real" in\n'
        '            "$card_real/autorun"|"$card_real/autorun/apps/"*/autorun) AUTORUN=$origin_real ;;\n'
        '            *) origin_bad=1 ;;\n'
        '        esac\n'
        '    fi\n'
        '    TARGET=$AUTORUN/$PACKAGE')
    text = replace_once(text, '    status checking\n\n    # Verify the list',
        '    status checking\n    [ -z "$origin_bad" ] || fail other-app\n\n    # Verify the list')
    # Distinguish the public release identity from the runtime registry name.
    text = replace_once(text, '    [ "$(cat "$NEW/app")" = "$APP" ] || fail other-app',
        '    [ "$APP" = melodee ] && [ "$(cat "$NEW/app")" = melodee ] || fail other-app')
    return text
