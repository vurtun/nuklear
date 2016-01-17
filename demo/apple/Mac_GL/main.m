/*
 Copyright (c) 2016 richi
 
 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.
 
 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:
 
 1.  The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2.  Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3.  This notice may not be removed or altered from any source distribution.
 */


#import "ZahnradBackend.h"


@interface ZahnradView : NSOpenGLView

@end


@implementation ZahnradView
{
    ZahnradBackend* zr;

    NSTrackingArea* currentTrackingArea;
    CVDisplayLinkRef displayLink;
}


#pragma mark - Setup -


- (instancetype) initWithFrame: (NSRect) frameRect
{
    NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFANoRecovery,
        0
    };
    
    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: pixelFormatAttributes];
    assert(self = [super initWithFrame: frameRect pixelFormat: pixelFormat]);
    
    [self updateTrackingArea];
    
    [[self openGLContext] makeCurrentContext];
    zr = [[ZahnradBackend alloc] initWithView: self];
    
    return self;
}


#if !SIMULATE_TOUCH


static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    return [(__bridge ZahnradView*) displayLinkContext drawFrameForTime: outputTime];
}


- (void) prepareOpenGL
{
    // Synchronize buffer swaps with vertical refresh rate
    GLint swapInt = 1;
    [[self openGLContext] setValues: &swapInt forParameter: NSOpenGLCPSwapInterval];
    
    // Create a display link capable of being used with all active displays
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    
    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback(displayLink, &displayLinkCallback, (__bridge void* _Nullable)(self));
    
    // Set the display link for the current renderer
    CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
    
    // Activate the display link
    CVDisplayLinkStart(displayLink);
}


#endif


- (void) glLocked: (dispatch_block_t) block
{
    CGLLockContext(self.openGLContext.CGLContextObj);
    
    @try
    {
        block();
    }
    @catch (NSException *exception)
    {
        NSLog(@"%@", exception);
    }
    @finally
    {
        CGLUnlockContext(self.openGLContext.CGLContextObj);
    }
}


#pragma mark - Drawing -


- (void) drawFrame
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    [zr updateFrame];
    [zr drawFrameWithScale: 1.0 width: self.bounds.size.width height: self.bounds.size.height];

    [[self openGLContext] flushBuffer];
}


- (CVReturn) drawFrameForTime: (const CVTimeStamp*) outputTime
{
    @autoreleasepool
    {
        [self glLocked: ^{
            [[self openGLContext] makeCurrentContext];
            [self drawFrame];
        }];
    }
    
    return kCVReturnSuccess;
}


- (void) drawRect: (NSRect) dirtyRect
{
    [self glLocked: ^{
        [[self openGLContext] makeCurrentContext];
        [super drawRect: dirtyRect];
        [self drawFrame];
    }];
}


#pragma mark - User Input -


- (void) addEvent: (NSDictionary*) event
{
    [self glLocked: ^{
        [zr addEvent: event];
    }];

#if SIMULATE_TOUCH
    
    [self setNeedsDisplay: YES];

#endif
}


- (void) mouseMoved: (NSEvent*) theEvent
{
#if !SIMULATE_TOUCH
    
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @1, @"pos" : NSStringFromPoint(location)}];

#endif
}


- (void) mouseDragged: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @2, @"pos" : NSStringFromPoint(location)}];
}


- (void) rightMouseDragged: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @3, @"pos" : NSStringFromPoint(location)}];
}


- (void) otherMouseDragged: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @4, @"pos" : NSStringFromPoint(location)}];
}


- (void) mouseDown: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @5, @"pos" : NSStringFromPoint(location)}];
}


- (void) mouseUp: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @6, @"pos" : NSStringFromPoint(location)}];
}


- (void) rightMouseDown: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @7, @"pos" : NSStringFromPoint(location)}];
}


- (void) rightMouseUp: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @8, @"pos" : NSStringFromPoint(location)}];
}


- (void) otherMouseDown: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @9, @"pos" : NSStringFromPoint(location)}];
}


- (void) otherMouseUp: (NSEvent*) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [self addEvent: @{@"type" : @10, @"pos" : NSStringFromPoint(location)}];
}


- (void) scrollWheel: (NSEvent*) theEvent
{
    [self addEvent: @{@"type" : @11, @"deltaX" : @(theEvent.deltaX), @"deltaY" : @(theEvent.deltaY)}];
}


- (void) keyDown: (NSEvent*) theEvent
{
    NSString* str = theEvent.characters;
    
    if (str.length)
        [self addEvent: @{@"type" : @12, @"txt" : str, @"mod" : @(theEvent.modifierFlags)}];
}


- (void) keyUp: (NSEvent*) theEvent
{
    NSString* str = theEvent.characters;
    
    if (str.length)
        [self addEvent: @{@"type" : @13, @"txt" : str, @"mod" : @(theEvent.modifierFlags)}];
}


- (BOOL) acceptsFirstResponder
{
    return YES;
}


- (void) removeCurrentTrackingArea
{
    if (currentTrackingArea)
    {
        [self removeTrackingArea: currentTrackingArea];
        currentTrackingArea = nil;
    }
}


- (void) updateTrackingArea
{
    [self removeCurrentTrackingArea];

#if !SIMULATE_TOUCH
    
    currentTrackingArea = [[NSTrackingArea alloc]
                           initWithRect: self.bounds
                           options: NSTrackingMouseMoved | NSTrackingActiveInActiveApp
                           owner: self
                           userInfo: nil];
    
    [self addTrackingArea: currentTrackingArea];
    
#endif
}


- (void) reshape
{
    [self updateTrackingArea];
    NSRect bounds = [self bounds];
    
    [self glLocked: ^{
        [[self openGLContext] makeCurrentContext];
        glViewport((GLint)NSMinX(bounds), (GLint)NSMinY(bounds), (GLint)NSWidth(bounds), (GLint)NSHeight(bounds));
    }];
}


- (void) update
{
    [self glLocked: ^{
        [super update];
    }];
}


@end


#pragma mark - App Delegate -


@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>

@property (strong) IBOutlet NSWindow* window;

@end


@implementation AppDelegate


- (void) applicationDidFinishLaunching: (NSNotification*) aNotification
{
    NSString* appName = [[NSProcessInfo processInfo] processName];
    
    NSLog(@"Starting %@", appName);
    
    NSMenu* appMenu = [NSMenu new];
    [appMenu addItem: [[NSMenuItem alloc] initWithTitle: [@"Quit " stringByAppendingString: appName]
                                                 action: @selector(terminate:)
                                          keyEquivalent: @"q"]];
    NSMenuItem* appMenuItem = [NSMenuItem new];
    [appMenuItem setSubmenu: appMenu];
    NSMenu* menubar = [NSMenu new];
    [menubar addItem: appMenuItem];
    [NSApp setMainMenu: menubar];
    
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    
    _window = [[NSWindow alloc] initWithContentRect: frame
                                          styleMask: NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
                                            backing: NSBackingStoreBuffered
                                              defer: NO];
    
    [_window setContentView: [[ZahnradView alloc] initWithFrame: frame]];
    [_window setDelegate: self];
    
    [_window setTitle: appName];
    [_window cascadeTopLeftFromPoint: NSMakePoint(20, 20)];
    [_window makeKeyAndOrderFront: nil];
}


- (void) windowWillClose: (NSNotification*) notification
{
    [NSApp terminate: self];
}


@end


int main()
{
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
        
        AppDelegate* appDelegate = [AppDelegate new];
        [NSApp setDelegate: appDelegate];
        
        [NSApp run];
    }
    
    return EXIT_SUCCESS;
}


