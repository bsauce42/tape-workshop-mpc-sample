// SPDX-License-Identifier: AGPL-3.0-only
import AppKit
let image=NSImage(size:NSSize(width:1024,height:1024));image.lockFocus()
let rect=NSRect(x:40,y:40,width:944,height:944)
NSColor(calibratedRed:0.035,green:0.025,blue:0.06,alpha:1).setFill();NSBezierPath(roundedRect:rect,xRadius:210,yRadius:210).fill()
let body=NSBezierPath(roundedRect:NSRect(x:170,y:360,width:684,height:390),xRadius:65,yRadius:65)
NSColor(calibratedRed:0.66,green:0.30,blue:0.98,alpha:1).setFill();body.fill()
NSColor.black.setFill();NSBezierPath(roundedRect:NSRect(x:225,y:455,width:574,height:220),xRadius:45,yRadius:45).fill()
for x in [320,704] {NSColor.white.setFill();NSBezierPath(ovalIn:NSRect(x:x-65,y:500,width:130,height:130)).fill();NSColor.black.setFill();NSBezierPath(ovalIn:NSRect(x:x-26,y:539,width:52,height:52)).fill()}
NSColor(calibratedRed:0.1,green:0.9,blue:0.95,alpha:1).setFill();NSBezierPath(roundedRect:NSRect(x:430,y:548,width:164,height:32),xRadius:8,yRadius:8).fill()
let attributes:[NSAttributedString.Key:Any]=[.font:NSFont.boldSystemFont(ofSize:82),.foregroundColor:NSColor.white]
let text="CUT PREP" as NSString;text.draw(at:NSPoint(x:(1024-text.size(withAttributes:attributes).width)/2,y:200),withAttributes:attributes)
image.unlockFocus();let bitmap=NSBitmapImageRep(data:image.tiffRepresentation!)!;try bitmap.representation(using:.png,properties:[:])!.write(to:URL(fileURLWithPath:CommandLine.arguments[1]))
