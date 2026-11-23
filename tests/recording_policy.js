// SPDX-License-Identifier: AGPL-3.0-only
const policy=require('../src/melodee/web/static/recording-policy.js');
const assert=require('node:assert/strict');
assert.equal(policy.limit(60*60*1000,1900000000),'');
assert.equal(policy.limit(2*60*60*1000-1,3720000000),'');
assert.match(policy.limit(2*60*60*1000,3720000000),/Two-hour/);
assert.equal(policy.limit(1000,policy.maxBytes-1),'');
assert.match(policy.limit(1000,policy.maxBytes),/file-size/);
assert.equal(policy.limit(19*60*1000,700000000,false),'');
assert.match(policy.limit(20*60*1000,320000000,false),/twenty-minute/);
assert.match(policy.limit(1000,768*1024*1024,false),/768 MB/);
assert.equal(policy.clock(61000),'01:01');assert.equal(policy.clock(3600000),'1:00:00');assert.equal(policy.clock(7200000),'2:00:00');
console.log('PASS: one/two-hour limits, file-size guard, short fallback and hour clock');

const take=policy.createClock(1000);
assert.equal(take.elapsed(2000),1000);
take.pause(3000);take.pause(4000);
assert.equal(take.elapsed(100000),2000);
take.resume(103000);take.resume(104000);
assert.equal(take.elapsed(105000),4000);
take.pause(106000);
assert.equal(take.elapsed(200000),5000);
take.resume(206000);
assert.equal(take.elapsed(207000),6000);
assert.equal(policy.limit(take.elapsed(207000),0),'');
console.log('PASS: paused time excluded, repeated pause/resume, stop while paused');
