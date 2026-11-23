// SPDX-License-Identifier: AGPL-3.0-only
// Run with Node; uses the production browser storage client in a sandbox.
(async () => {
    const assert = require('node:assert/strict'), fs = require('node:fs'), vm = require('node:vm');
    const script = fs.readFileSync(require('node:path').join(__dirname, '../src/melodee/web/static/expansion-save.js'), 'utf8');
    function fixture(options = {}) {
        const calls = [], progress = [];
        class XHR {
            constructor() { this.upload = {}; }
            open(method, url) { calls.push({method, url, headers: {}}); }
            setRequestHeader(key, value) { calls.at(-1).headers[key] = value; }
            send(blob) {
                calls.at(-1).blob = blob;
                if (options.disconnected) { this.onerror(); return; }
                this.upload.onprogress({lengthComputable: true, loaded: 5, total: 10});
                this.upload.onload();
                this.status = options.status || 200;
                this.responseText = JSON.stringify(options.reply || {name: 'Kit 2.xpn'});
                this.onload();
            }
        }
        const sandbox = {window: {}, Blob, XMLHttpRequest: XHR,
            fetch: async (url, init) => {
                calls.push({url, ...init});
                const status = url.startsWith('/mkdir') ? (options.mkdir || 200) : (options.list || 200);
                return {ok: status === 200, status, json: async () => ({error: 'Test failure'})};
            }};
        vm.runInNewContext(script, sandbox);
        return {calls, progress, save: () => sandbox.window.WorkshopPackStorage.save(new Uint8Array([1, 2, 3]), 'Kit.xpn', n => progress.push(n))};
    }
    for (const mkdir of [200, 409]) {
        const f = fixture({mkdir});
        assert.equal(await f.save(), 'Expansion Packs/Kit 2.xpn');
        assert.deepEqual(f.progress, [50, 100]);
        assert.equal(f.calls[0].url, '/mkdir?path=&name=Expansion%20Packs');
        assert.equal(f.calls[1].url, '/list?path=Expansion%20Packs');
        assert.equal(f.calls[2].url, '/upload?path=Expansion%20Packs&name=Kit.xpn');
        assert.equal(f.calls[2].headers['X-Melodee'], '1');
        assert.deepEqual([...new Uint8Array(await f.calls[2].blob.arrayBuffer())], [1, 2, 3]);
    }
    for (const options of [{mkdir: 507}, {mkdir: 409, list: 404}, {status: 507, reply: {error: 'SD card full'}}, {disconnected: true}, {reply: {}}]) {
        const f = fixture(options);
        await assert.rejects(f.save());
        if (options.mkdir === 507 || options.list === 404) assert.ok(f.calls.length < 3);
    }
    console.log('PASS: direct expansion save, existing folder, returned duplicate filename, archive bytes, progress, full card, invalid folder, disconnect and unconfirmed save');
})();
