// Regression: exercise the actual recording compositor, not just layout math.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const presentation=require('../src/melodee/web/static/tutorial-presentation.js');
const source=fs.readFileSync('src/melodee/web/static/app.js','utf8');
const draw=source.slice(source.indexOf('  function drawRecording() {'),source.indexOf('  function stopRecording('));
for(const mode of ['both','device','screen','transition']){
 const events=[];let pose=mode==='transition'?presentation.interpolate(presentation.layout('both'),presentation.layout('device'),.5):presentation.layout(mode);
 const live={},device={};const gradient={addColorStop(){}};
 const paint=new Proxy({globalAlpha:.25,globalCompositeOperation:'destination-over',createLinearGradient:()=>gradient,drawImage:img=>events.push(img===device?'device':img===live?'lcd':'other'),clearRect:()=>events.push('clear')},{get:(o,k)=>k in o?o[k]:()=>{}});
 const env={recordingCanvas:{width:1920,height:1080,getContext:()=>paint},canvas:live,devicePicture:device,scene:{value:'studio'},sceneTime:()=>0,sceneInput:()=>null,waveformToggle:{checked:false},cameraNow:()=>({zoom:1,x:0,y:0,yaw:0}),layoutNow:()=>pose,page:{querySelector:()=>null},
  $:id=>({'#device-screen':{},'#device-style':{value:'realistic'},'#tutorial-action':{textContent:'Stop'}}[id]),
  presentation:{...presentation,background(){},contactShadow:(_,box)=>events.push('shadow:'+box.opacity),screen:()=>events.push('large-screen')},window:{WorkshopScenes:{draw:()=>false}}};
 vm.createContext(env);vm.runInContext(draw+';drawRecording();',env);
 assert.equal(events[0],'clear','Clear preceding video frame before composition');
 assert.equal(paint.globalCompositeOperation,'source-over');
 assert.equal(events.includes('large-screen'),mode!=='device','MPC-only must omit large screen');
 assert.equal(events.includes('device'),mode!=='screen','Screen-only must omit hardware');
 assert.equal(events.includes('lcd'),mode!=='screen','MPC LCD remains visible within the device');
 assert(!events.includes('shadow:0'),'Hidden panels cannot cast a shadow');
}
console.log('Recording views passed: both, MPC only, screen only, smooth transition and frame clearing');
