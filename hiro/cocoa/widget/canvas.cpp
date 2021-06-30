#if defined(Hiro_Canvas)

@implementation CocoaCanvas : NSImageView

-(id) initWith:(hiro::mCanvas&)canvasReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    canvas = &canvasReference;
    [self setEditable:NO];  //disable image drag-and-drop functionality
    NSTrackingArea* area = [[[NSTrackingArea alloc] initWithRect:[self frame]
      options:NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect
      owner:self userInfo:nil
    ] autorelease];
    [self addTrackingArea:area];
  }
  return self;
}

-(void) resetCursorRects {
  if(auto mouseCursor = NSMakeCursor(canvas->mouseCursor())) {
    [self addCursorRect:self.bounds cursor:mouseCursor];
  }
}

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
  return DropPathsOperation(sender);
}

-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender {
  auto paths = DropPaths(sender);
  if(!paths) return NO;
  canvas->doDrop(paths);
  return YES;
}

-(void) mouseButton:(NSEvent*)event down:(BOOL)isDown {
  if(isDown) {
    switch([event buttonNumber]) {
    case 0: return canvas->doMousePress(hiro::Mouse::Button::Left);
    case 1: return canvas->doMousePress(hiro::Mouse::Button::Right);
    case 2: return canvas->doMousePress(hiro::Mouse::Button::Middle);
    }
  } else {
    switch([event buttonNumber]) {
    case 0: return canvas->doMouseRelease(hiro::Mouse::Button::Left);
    case 1: return canvas->doMouseRelease(hiro::Mouse::Button::Right);
    case 2: return canvas->doMouseRelease(hiro::Mouse::Button::Middle);
    }
  }
}

-(void) mouseEntered:(NSEvent*)event {
  canvas->doMouseEnter();
}

-(void) mouseExited:(NSEvent*)event {
  canvas->doMouseLeave();
}

-(void) mouseMove:(NSEvent*)event {
  if([event window] == nil) return;
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  canvas->doMouseMove({(s32)location.x, (s32)([self frame].size.height - 1 - location.y)});
}

-(void) mouseDown:(NSEvent*)event {
  [self mouseButton:event down:YES];
}

-(void) mouseUp:(NSEvent*)event {
  [self mouseButton:event down:NO];
}

-(void) mouseDragged:(NSEvent*)event {
  [self mouseMove:event];
}

-(void) rightMouseDown:(NSEvent*)event {
  [self mouseButton:event down:YES];
}

-(void) rightMouseUp:(NSEvent*)event {
  [self mouseButton:event down:NO];
}

-(void) rightMouseDragged:(NSEvent*)event {
  [self mouseMove:event];
}

-(void) otherMouseDown:(NSEvent*)event {
  [self mouseButton:event down:YES];
}

-(void) otherMouseUp:(NSEvent*)event {
  [self mouseButton:event down:NO];
}

-(void) otherMouseDragged:(NSEvent*)event {
  [self mouseMove:event];
}

@end

namespace hiro {

auto pCanvas::construct() -> void {
  @autoreleasepool {
    cocoaView = cocoaCanvas = [[CocoaCanvas alloc] initWith:self()];
    pWidget::construct();
  }
}

auto pCanvas::destruct() -> void {
  @autoreleasepool {
    [cocoaView removeFromSuperview];
    [cocoaView release];
  }
}

auto pCanvas::minimumSize() const -> Size {
  if(auto& icon = state().icon) return {(s32)icon.width(), (s32)icon.height()};
  return {0, 0};
}

auto pCanvas::setAlignment(Alignment) -> void {
  update();
}

auto pCanvas::setColor(Color color) -> void {
  update();
}

auto pCanvas::setDroppable(bool droppable) -> void {
  @autoreleasepool {
    if(droppable) {
      [cocoaCanvas registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
    } else {
      [cocoaCanvas unregisterDraggedTypes];
    }
  }
}

auto pCanvas::setFocusable(bool focusable) -> void {
  //TODO
}

auto pCanvas::setGeometry(Geometry geometry) -> void {
  pWidget::setGeometry(geometry);
  update();
}

auto pCanvas::setGradient(Gradient gradient) -> void {
  update();
}

auto pCanvas::setIcon(const image& icon) -> void {
  update();
}

auto pCanvas::update() -> void {
  _rasterize();
  @autoreleasepool {
    [cocoaView setNeedsDisplay:YES];
  }
}

//todo: support cases where the icon size does not match the canvas size (alignment)
auto pCanvas::_rasterize() -> void {
  @autoreleasepool {
    s32 width = 0;
    s32 height = 0;

    if(auto& icon = state().icon) {
      width = icon.width();
      height = icon.height();
    } else {
      width = pSizable::state().geometry.width();
      height = pSizable::state().geometry.height();
    }
    if(width <= 0 || height <= 0) return;

    if(width != surfaceWidth || height != surfaceHeight) {
      [cocoaView setImage:nil];
      surface = nullptr;
      bitmap = nullptr;
    }

    surfaceWidth = width;
    surfaceHeight = height;

    if(!surface) {
      surface = [[[NSImage alloc] initWithSize:NSMakeSize(width, height)] autorelease];
      bitmap = [[[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:nil
        pixelsWide:width pixelsHigh:height
        bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES
        isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
        bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
        bytesPerRow:(width * 4) bitsPerPixel:32
      ] autorelease];
      [surface addRepresentation:bitmap];
      [cocoaView setImage:surface];
    }

    auto target = (u32*)[bitmap bitmapData];

    if(auto icon = state().icon) {
      icon.transform(0, 32, 255u << 24, 255u << 0, 255u << 8, 255u << 16);  //Cocoa uses ABGR format
      memory::copy(target, icon.data(), icon.size());
    } else if(auto& gradient = state().gradient) {
      auto& colors = gradient.state.colors;
      image fill;
      fill.allocate(width, height);
      fill.gradient(colors[0].value(), colors[1].value(), colors[2].value(), colors[3].value());
      memory::copy(target, fill.data(), fill.size());
    } else {
      u32 color = state().color.value();
      for(auto n : range(width * height)) target[n] = color;
    }
  }
}

}

#endif
