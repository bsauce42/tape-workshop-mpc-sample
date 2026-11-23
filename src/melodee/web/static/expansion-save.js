// SPDX-License-Identifier: AGPL-3.0-only
'use strict';
(() => {
    const folder = 'Expansion Packs';
    async function responseError(response) {
        try { return (await response.json()).error || 'Could not save to the MPC'; }
        catch (_) { return 'Could not save to the MPC (' + response.status + ')'; }
    }
    async function save(data, name, progress = () => {}) {
        const created = await fetch('/mkdir?path=&name=' + encodeURIComponent(folder), {
            method: 'POST', headers: {'X-Melodee': '1'}, body: ''
        });
        if (!created.ok && created.status !== 409) throw Error(await responseError(created));
        // A name conflict may be a file rather than a folder; verify before uploading.
        const listing = await fetch('/list?path=' + encodeURIComponent(folder), {cache: 'no-store'});
        if (!listing.ok) throw Error(await responseError(listing));
        return new Promise((resolve, reject) => {
            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/upload?path=' + encodeURIComponent(folder) + '&name=' + encodeURIComponent(name));
            xhr.setRequestHeader('X-Melodee', '1');
            xhr.setRequestHeader('Content-Type', 'application/zip');
            xhr.upload.onprogress = event => {
                if (event.lengthComputable) progress(Math.round(event.loaded * 100 / event.total));
            };
            xhr.upload.onload = () => progress(100);
            xhr.onload = () => {
                let result;
                try { result = JSON.parse(xhr.responseText); } catch (_) { result = {}; }
                if (xhr.status >= 200 && xhr.status < 300 && result.name) resolve(folder + '/' + result.name);
                else reject(Error(result.error || 'The MPC did not confirm that the pack was saved'));
            };
            xhr.onerror = () => reject(Error('MPC disconnected. Check Files before retrying; the pack may have finished saving.'));
            xhr.onabort = () => reject(Error('Save cancelled'));
            xhr.send(new Blob([data], {type: 'application/zip'}));
        });
    }
    window.WorkshopPackStorage = {save, folder};
})();
