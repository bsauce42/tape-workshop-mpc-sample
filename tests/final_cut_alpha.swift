import AVFoundation
import Foundation
for name in ["MPC","Screen"] {
 let asset=AVURLAsset(url:URL(fileURLWithPath:CommandLine.arguments[1]).appendingPathComponent(name+".mov")),track=asset.tracks(withMediaType:.video)[0]
 let reader=try AVAssetReader(asset:asset),out=AVAssetReaderTrackOutput(track:track,outputSettings:[kCVPixelBufferPixelFormatTypeKey as String:kCVPixelFormatType_32BGRA]);reader.add(out);reader.startReading()
 let sample=out.copyNextSampleBuffer()!,buffer=CMSampleBufferGetImageBuffer(sample)!
 CVPixelBufferLockBaseAddress(buffer,.readOnly);let w=CVPixelBufferGetWidth(buffer),h=CVPixelBufferGetHeight(buffer),row=CVPixelBufferGetBytesPerRow(buffer),bytes=CVPixelBufferGetBaseAddress(buffer)!.assumingMemoryBound(to:UInt8.self)
 let edge=bytes[3],center=bytes[(h/2)*row+(w/2)*4+3]
 print(name,"duration",asset.duration.seconds,"size",w,h,"alpha outside/center",edge,center,"audio tracks",asset.tracks(withMediaType:.audio).count)
 precondition(edge<10 && center>240,"Transparency failed")
 CVPixelBufferUnlockBaseAddress(buffer,.readOnly);reader.cancelReading()
}
