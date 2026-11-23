// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; Melodee by Kerem Kiliç (Ellic Studio).
import AppKit
import AVFoundation
import CoreImage

func failure(_ message: String) -> NSError { NSError(domain: "TapeWorkshop.FinalCut", code: 1, userInfo: [NSLocalizedDescriptionKey: message]) }
func xml(_ text: String) -> String { text.replacingOccurrences(of: "&", with: "&amp;").replacingOccurrences(of: "\"", with: "&quot;").replacingOccurrences(of: "<", with: "&lt;").replacingOccurrences(of: ">", with: "&gt;") }
let context = CIContext(options: [.cacheIntermediates: false])

func transparentMovie(_ source: URL, _ maskURL: URL, _ output: URL, progress: @escaping (Double)->Void) throws {
    let asset = AVURLAsset(url: source)
    guard let video = asset.tracks(withMediaType: .video).first,
          let mask = CIImage(contentsOf: maskURL) else { throw failure("Missing video or mask: \(source.lastPathComponent)") }
    let width = Int(video.naturalSize.width), height = Int(video.naturalSize.height)
    let bounds = CGRect(x: 0, y: 0, width: width, height: height)
    let matte = mask.transformed(by: CGAffineTransform(scaleX: CGFloat(width)/mask.extent.width, y: CGFloat(height)/mask.extent.height))
    let reader = try AVAssetReader(asset: asset)
    let frames = AVAssetReaderTrackOutput(track: video, outputSettings: [kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA])
    frames.alwaysCopiesSampleData = false; reader.add(frames)
    let writer = try AVAssetWriter(outputURL: output, fileType: .mov)
    let visual = AVAssetWriterInput(mediaType: .video, outputSettings: [AVVideoCodecKey: AVVideoCodecType.hevcWithAlpha, AVVideoWidthKey: width, AVVideoHeightKey: height, AVVideoCompressionPropertiesKey: [AVVideoAverageBitRateKey: 6_000_000]])
    visual.expectsMediaDataInRealTime = false
    let pixels = AVAssetWriterInputPixelBufferAdaptor(assetWriterInput: visual, sourcePixelBufferAttributes: [kCVPixelBufferWidthKey as String: width, kCVPixelBufferHeightKey as String: height, kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA, kCVPixelBufferIOSurfacePropertiesKey as String: [:]])
    guard writer.canAdd(visual) else { throw failure("This Mac cannot encode video with transparency.") }; writer.add(visual)
    var audioOutput: AVAssetReaderTrackOutput?, audioInput: AVAssetWriterInput?
    if let track = asset.tracks(withMediaType: .audio).first {
        let out = AVAssetReaderTrackOutput(track: track, outputSettings: [AVFormatIDKey: kAudioFormatLinearPCM, AVLinearPCMBitDepthKey: 16, AVLinearPCMIsFloatKey: false, AVLinearPCMIsBigEndianKey: false, AVLinearPCMIsNonInterleaved: false])
        let input = AVAssetWriterInput(mediaType: .audio, outputSettings: [AVFormatIDKey: kAudioFormatMPEG4AAC, AVSampleRateKey: 48000, AVNumberOfChannelsKey: 2, AVEncoderBitRateKey: 192000])
        input.expectsMediaDataInRealTime = false
        guard reader.canAdd(out), writer.canAdd(input) else { throw failure("Cannot prepare stereo audio.") }
        reader.add(out);writer.add(input);audioOutput=out;audioInput=input
    }
    guard writer.startWriting(),reader.startReading() else { throw writer.error ?? reader.error ?? failure("Cannot read the recording.") }
    writer.startSession(atSourceTime: .zero)
    let group=DispatchGroup();var videoEnded=false;var renderError: Error?
    group.enter()
    visual.requestMediaDataWhenReady(on: DispatchQueue(label: "tw.finalcut.video")) {
        while visual.isReadyForMoreMediaData && !videoEnded {
            guard let sample=frames.copyNextSampleBuffer() else {videoEnded=true;visual.markAsFinished();group.leave();return}
            guard let frame=CMSampleBufferGetImageBuffer(sample),let pool=pixels.pixelBufferPool else {renderError=failure("Cannot decode a video frame.");videoEnded=true;visual.markAsFinished();group.leave();return}
            var buffer: CVPixelBuffer?
            guard CVPixelBufferPoolCreatePixelBuffer(nil,pool,&buffer)==kCVReturnSuccess,let buffer=buffer else {renderError=failure("Insufficient memory for a video frame.");videoEnded=true;visual.markAsFinished();group.leave();return}
            progress(min(0.99, CMSampleBufferGetPresentationTimeStamp(sample).seconds / max(0.01,asset.duration.seconds)))
            let foreground=CIImage(cvPixelBuffer: frame)
            let result=foreground.applyingFilter("CIBlendWithMask",parameters:[kCIInputBackgroundImageKey:CIImage(color:.clear).cropped(to:bounds),kCIInputMaskImageKey:matte])
            context.render(result,to:buffer,bounds:bounds,colorSpace:CGColorSpace(name:CGColorSpace.sRGB))
            if !pixels.append(buffer,withPresentationTime:CMSampleBufferGetPresentationTimeStamp(sample)){renderError=writer.error ?? failure("Video encoding failed.");videoEnded=true;visual.markAsFinished();group.leave();return}
        }
    }
    if let input=audioInput,let out=audioOutput {
        group.enter();var ended=false
        input.requestMediaDataWhenReady(on: DispatchQueue(label:"tw.finalcut.audio")) {
            while input.isReadyForMoreMediaData && !ended {
                guard let sample=out.copyNextSampleBuffer() else {ended=true;input.markAsFinished();group.leave();return}
                if !input.append(sample){ended=true;input.markAsFinished();group.leave();return}
            }
        }
    }
    while group.wait(timeout: .now()+0.5) == .timedOut {
        if writer.status == .failed || reader.status == .failed {reader.cancelReading();writer.cancelWriting();throw writer.error ?? reader.error ?? failure("Movie preparation failed.")}
    }
    if let error=renderError {writer.cancelWriting();throw error}
    if reader.status == .failed {writer.cancelWriting();throw reader.error ?? failure("Recording decode failed.")}
    let done=DispatchSemaphore(value:0);writer.finishWriting{done.signal()};done.wait()
    guard writer.status == .completed else {throw writer.error ?? failure("Transparent movie export failed.")}
}

func prepare(_ folder: URL, destination: URL? = nil, progress: @escaping (String, Double)->Void) throws -> URL {
    let fm=FileManager.default
    for name in ["MPC.mp4","Screen.mp4","MPC-Mask.png","Screen-Mask.png","Background.png"] {
        guard fm.fileExists(atPath:folder.appendingPathComponent(name).path) else {throw failure("Choose the extracted Final Cut recording folder. Missing \(name).")}
    }
    let target=destination ?? folder
    var output=target.appendingPathComponent("Final Cut Ready",isDirectory:true);var suffix=2
    while fm.fileExists(atPath:output.path){output=target.appendingPathComponent("Final Cut Ready \(suffix)",isDirectory:true);suffix+=1}
    try fm.createDirectory(at:output,withIntermediateDirectories:true)
    do {
        for (index,name) in ["MPC","Screen"].enumerated() {try transparentMovie(folder.appendingPathComponent(name+".mp4"),folder.appendingPathComponent(name+"-Mask.png"),output.appendingPathComponent(name+".mov")){fraction in progress("Preparing transparent \(name) video…",(Double(index)+fraction)*0.48)}}
        progress("Building timeline…",0.97)
        try fm.copyItem(at:folder.appendingPathComponent("Background.png"),to:output.appendingPathComponent("Background.png"))
        let mpc=AVURLAsset(url:output.appendingPathComponent("MPC.mov")),screen=AVURLAsset(url:output.appendingPathComponent("Screen.mov"))
        guard let track=mpc.tracks(withMediaType:.video).first else {throw failure("No video was exported.")}
        let frames=max(1,Int(floor(min(mpc.duration.seconds,screen.duration.seconds)*30))),duration="\(frames)/30s"
        func asset(_ id: String,_ name: String,_ file: String,_ audio: Bool,_ still: Bool=false)->String {
            "<asset id=\"\(id)\" name=\"\(name)\" src=\"\(xml(output.appendingPathComponent(file).absoluteString))\" start=\"0s\" duration=\"\(still ? "0s" : duration)\" hasVideo=\"1\" format=\"f\""+(audio ? " hasAudio=\"1\" audioSources=\"1\" audioChannels=\"2\" audioRate=\"48000\"" : "")+"/>"
        }
        let assets=asset("a","MPC","MPC.mov",!mpc.tracks(withMediaType:.audio).isEmpty)+asset("b","Screen","Screen.mov",!screen.tracks(withMediaType:.audio).isEmpty)+asset("bg","Background — replace with a color generator","Background.png",false,true)
        let background="<video ref=\"bg\" name=\"Background\" lane=\"1\" offset=\"0s\" start=\"0s\" duration=\"\(duration)\"/>"
        let layerA="<video ref=\"a\" name=\"MPC\" lane=\"2\" offset=\"0s\" start=\"0s\" duration=\"\(duration)\"><adjust-transform position=\"57 1.4\" scale=\"0.65 0.65\"/></video>"
        let layerB="<video ref=\"b\" name=\"Screen\" lane=\"3\" offset=\"0s\" start=\"0s\" duration=\"\(duration)\"><adjust-transform position=\"-33.9 0.14\" scale=\"0.775 0.775\"/></video>"
        let audio=mpc.tracks(withMediaType:.audio).isEmpty ? "" : "<audio ref=\"a\" lane=\"-2\" offset=\"0s\" start=\"0s\" duration=\"\(duration)\" srcCh=\"1, 2\"/>"
        let document="<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE fcpxml><fcpxml version=\"1.7\"><resources><format id=\"f\" name=\"FFVideoFormat1080p30\" frameDuration=\"1/30s\" width=\"\(Int(track.naturalSize.width))\" height=\"\(Int(track.naturalSize.height))\"/>\(assets)</resources><library><event name=\"Tape Workshop tutorial\"><project name=\"Tutorial — separate layers\"><sequence format=\"f\" duration=\"\(duration)\" tcStart=\"0s\" tcFormat=\"NDF\" audioLayout=\"stereo\" audioRate=\"48k\"><spine><gap name=\"Tutorial layers\" offset=\"0s\" start=\"0s\" duration=\"\(duration)\">\(background)\(layerA)\(layerB)\(audio)</gap></spine></sequence></project></event></library></fcpxml>"
        try document.write(to:output.appendingPathComponent("Tutorial.fcpxml"),atomically:true,encoding:.utf8)
        try "Import Tutorial.fcpxml in Final Cut Pro. The timeline has separate editable MPC, screen, background and stereo audio layers. All layers share the same 30 fps timeline. Replace the Background clip with your own color generator, image or video. Both MOVs preserve transparency without chroma keying. The source files remain untouched. Keep this folder together after importing; use Relink Files if it is moved. Sample-accurate hardware sync and Final Cut import still need testing on your system.\n\nBased on Melodee by Kerem Kiliç (Ellic Studio).\n".write(to:output.appendingPathComponent("READ ME.txt"),atomically:true,encoding:.utf8)
        progress("Ready",1)
        return output
    }catch{try? fm.removeItem(at:output);throw error}
}

final class App: NSObject, NSApplicationDelegate {
    let bar=NSProgressIndicator();let percent=NSTextField(labelWithString:"0%");var window:NSWindow!;let status=NSTextField(wrappingLabelWithString:"Choose the folder extracted from the browser's Final Cut files ZIP. Original recordings are kept. Transparent MOVs require additional disk space.")
    func applicationDidFinishLaunching(_ notification: Notification){
        window=NSWindow(contentRect:NSRect(x:0,y:0,width:570,height:350),styleMask:[.titled,.closable,.miniaturizable],backing:.buffered,defer:false);window.title="Tape Workshop · Final Cut Prep";window.center()
        let title=NSTextField(labelWithString:"Prepare your Final Cut timeline");title.font = .boldSystemFont(ofSize:23);title.frame=NSRect(x:28,y:270,width:520,height:35)
        status.frame=NSRect(x:28,y:150,width:510,height:110);status.font = .systemFont(ofSize:15)
        bar.isIndeterminate=false;bar.minValue=0;bar.maxValue=100;bar.frame=NSRect(x:28,y:105,width:450,height:20);percent.frame=NSRect(x:490,y:102,width:60,height:25);window.contentView?.addSubview(bar);window.contentView?.addSubview(percent)
        let button=NSButton(title:"Choose recording folder…",target:self,action:#selector(choose));button.frame=NSRect(x:28,y:35,width:260,height:40);button.bezelStyle = .rounded
        window.contentView?.addSubview(title);window.contentView?.addSubview(status);window.contentView?.addSubview(button);window.makeKeyAndOrderFront(nil);NSApp.activate(ignoringOtherApps:true)
    }
    @objc func choose(){let picker=NSOpenPanel();picker.canChooseDirectories=true;picker.canChooseFiles=false;picker.allowsMultipleSelection=false
        guard picker.runModal() == .OK,let folder=picker.url else{return}
        let save=NSOpenPanel();save.title="Choose where to save the prepared files";save.prompt="Save here";save.canChooseDirectories=true;save.canChooseFiles=false;save.canCreateDirectories=true
        guard save.runModal() == .OK,let destination=save.url else{return}
        window.contentView?.subviews.compactMap{$0 as? NSButton}.forEach{$0.isEnabled=false}
        DispatchQueue.global(qos:.userInitiated).async{do{let output=try prepare(folder,destination:destination){message,fraction in DispatchQueue.main.async{self.status.stringValue=message;self.bar.doubleValue=fraction*100;self.percent.stringValue="\(Int(fraction*100))%"}};DispatchQueue.main.async{self.status.stringValue="Ready. Import Tutorial.fcpxml into Final Cut Pro. The timeline contains separate editable layers.";NSWorkspace.shared.open(output);self.enable()}}catch{DispatchQueue.main.async{self.status.stringValue=error.localizedDescription;self.enable()}}}
    }
    func enable(){window.contentView?.subviews.compactMap{$0 as? NSButton}.forEach{$0.isEnabled=true}}
    func applicationShouldTerminateAfterLastWindowClosed(_ sender:NSApplication)->Bool{true}
}
if CommandLine.arguments.count==3 && CommandLine.arguments[1]=="--prepare" {
    do{let output=try prepare(URL(fileURLWithPath:CommandLine.arguments[2],isDirectory:true)){print("\(Int($1*100))% \($0)")};print(output.path)}catch{fputs(error.localizedDescription+"\n",stderr);exit(1)}
}else{let app=NSApplication.shared,delegate=App();app.setActivationPolicy(.regular);app.delegate=delegate;app.run()}
