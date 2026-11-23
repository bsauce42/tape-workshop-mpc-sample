// SPDX-License-Identifier: AGPL-3.0-only
(function(root) {
  "use strict";
  const repository = "bsauce42/tape-workshop-mpc-sample";
  function version(text) {
    const match = /^v?(\d{1,9})\.(\d{1,9})\.(\d{1,9})-tapeworkshop-community\.(\d{1,9})$/.exec(text || "");
    return match ? match.slice(1).map(Number) : null;
  }
  function compare(a,b) {
    a=version(a); b=version(b);
    if (!a || !b) return null;
    for(let n=0;n<4;++n) if(a[n]!==b[n]) return a[n]>b[n]?1:-1;
    return 0;
  }
  function offer(release,current) {
    if (!release || release.draft || release.prerelease) return null;
    const parsed=version(release.tag_name), order=compare(release.tag_name,current);
    if (!parsed || order===null) return null;
    const name="Tape-Workshop-v"+parsed[3]+"-Signed-Update.zip";
    const asset=Array.isArray(release.assets) ? release.assets.find(a => a.name===name && Number.isSafeInteger(a.id) && a.id>0 && Number.isSafeInteger(a.size) && a.size>0 && a.size<=268435456) : null;
    return {version:release.tag_name, title:String(release.name || release.tag_name).slice(0,160), order,
      releaseUrl:"https://github.com/"+repository+"/releases/tag/"+encodeURIComponent(release.tag_name),
      name, size:asset?.size || 0,
      api:asset ? "https://api.github.com/repos/"+repository+"/releases/assets/"+asset.id : null,
      mirror:asset ? "https://raw.githubusercontent.com/"+repository+"/updates/"+name : null,
      download:asset ? "https://github.com/"+repository+"/releases/download/"+encodeURIComponent(release.tag_name)+"/"+name : null};
  }
  const api={repository,version,compare,offer};
  if (typeof module!=="undefined" && module.exports) module.exports=api;
  else root.TapeWorkshopReleases=api;
})(typeof window!=="undefined" ? window : globalThis);
