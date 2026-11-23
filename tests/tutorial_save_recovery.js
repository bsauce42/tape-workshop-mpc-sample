// Exercise the actual display loop with a suspended audio clock after Save As.
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const source=fs.readFileSync('src/melodee/web/static/app.js','utf8');
const loop=source.slice(source.indexOf('  function presentLive() {'),source.indexOf('  requestAnimationFrame(presentLive);\n  ctx.imageSmoothingEnabled'));
let seen=[];
const env={requestAnimationFrame(){},paused:false,document:{hidden:false},audioOn:true,audioOrigin:0,audioContext:{state:'suspended',currentTime:0},audioBase:0,audioRate:48000,performance:{now:()=>1000},videoQueue:[{sample:999999,received:500}],hardwareQueue:[],dispose(){},displayFrame:f=>seen.push(f),showHardware(){}};
vm.createContext(env);vm.runInContext(loop+';presentLive()',env);assert.equal(seen.length,1,'stalled clock must release old video frame');
seen=[];env.audioContext.state='running';env.audioContext.currentTime=1;env.videoQueue=[{sample:96000,received:900}];vm.runInContext('presentLive()',env);assert.equal(seen.length,0,'healthy clock must preserve audio synchronization');
console.log('Save recovery display regression passed');
