// SPDX-License-Identifier: AGPL-3.0-only
// Keep the recording bus independent of the Mac/phone's speaker layout.
(function(root){
    'use strict';
    function create(context){
        const input=context.createGain();
        input.channelCount=2; input.channelCountMode='explicit'; input.channelInterpretation='discrete';
        const split=context.createChannelSplitter(2), merge=context.createChannelMerger(2);
        split.channelInterpretation='discrete'; merge.channelInterpretation='discrete';
        const destination=context.createMediaStreamDestination();
        destination.channelCount=2; destination.channelCountMode='explicit'; destination.channelInterpretation='discrete';
        input.connect(split);
        split.connect(merge,0,0); split.connect(merge,1,1);
        merge.connect(destination);
        for(const track of destination.stream.getAudioTracks())track.contentHint='music';
        return {input,destination,disconnect(){input.disconnect();split.disconnect();merge.disconnect();}};
    }
    if(typeof module!=='undefined')module.exports={create};
    else root.WorkshopStereoRecording={create};
})(typeof window==='undefined'?globalThis:window);
