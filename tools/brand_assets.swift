// SPDX-License-Identifier: AGPL-3.0-only
// Deterministic asset fitting for the supplied Tape Workshop wordmark.
// Preserves the supplied artwork; no redrawing or generated lettering.
import Foundation
import CoreGraphics
import ImageIO
import UniformTypeIdentifiers
let args=CommandLine.arguments
if args.count != 3 { fatalError("Usage: swift tools/brand_assets.swift LOGO.jpg OUTPUT_DIRECTORY") }
let input=URL(fileURLWithPath:args[1]),folder=URL(fileURLWithPath:args[2])
try FileManager.default.createDirectory(at:folder,withIntermediateDirectories:true)
guard let source=CGImageSourceCreateWithURL(input as CFURL,nil),let original=CGImageSourceCreateImageAtIndex(source,0,nil) else {fatalError("Cannot read supplied logo")}
let sx=CGFloat(original.width)/1280,sy=CGFloat(original.height)/427
func crop(_ x:CGFloat,_ y:CGFloat,_ w:CGFloat,_ h:CGFloat)->CGImage { original.cropping(to:CGRect(x:x*sx,y:y*sy,width:w*sx,height:h*sy))! }
func canvas(_ width:Int,_ height:Int,_ draw:(CGContext)->Void)->CGImage {
 let context=CGContext(data:nil,width:width,height:height,bitsPerComponent:8,bytesPerRow:width*4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGImageAlphaInfo.premultipliedLast.rawValue)!
 context.setFillColor(CGColor(red:0,green:0,blue:0,alpha:1));context.fill(CGRect(x:0,y:0,width:width,height:height));context.interpolationQuality = .high;draw(context);return context.makeImage()!
}
func write(_ image:CGImage,_ name:String){let url=folder.appendingPathComponent(name);let output=CGImageDestinationCreateWithURL(url as CFURL,UTType.png.identifier as CFString,1,nil)!;CGImageDestinationAddImage(output,image,nil);if !CGImageDestinationFinalize(output){fatalError("Cannot save asset")}}
let wordmark=crop(62,132,1170,158),cassette=crop(70,140,180,112),beam=crop(250,225,950,58)
write(wordmark,"wordmark.png")
let splash=canvas(320,240){c in c.draw(wordmark,in:CGRect(x:8,y:99.5,width:304,height:41.05))}
write(splash,"splash.png")
write(canvas(320,240){_ in},"black.png")
let icon=canvas(1024,1024){c in c.draw(cassette,in:CGRect(x:100,y:288,width:824,height:512.7));c.draw(beam,in:CGRect(x:72,y:210,width:880,height:53.7))}
for size in [16,32,64,128,180,192,256,512,1024]{write(canvas(size,size){c in c.draw(icon,in:CGRect(x:0,y:0,width:size,height:size))},"icon-\(size).png")}
print("Fitted supplied wordmark, centered black splash and cassette app icons")
