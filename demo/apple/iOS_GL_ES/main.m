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

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#import "ZahnradBackend.h"


@interface GameViewController : GLKViewController
@end


@implementation GameViewController
{
    EAGLContext* context;
    ZahnradBackend* zr;
}


- (void) viewDidLoad
{
    [super viewDidLoad];
    
    context = [[EAGLContext alloc] initWithAPI: kEAGLRenderingAPIOpenGLES3];
    assert(context != nil);
    
    ((GLKView*)self.view).context = context;
    ((GLKView*)self.view).drawableDepthFormat = GLKViewDrawableDepthFormatNone;
    
    [EAGLContext setCurrentContext: context];
    zr = [[ZahnradBackend alloc] initWithView: ((GLKView*)self.view)];
}


- (BOOL) prefersStatusBarHidden
{
    return YES;
}


- (void) update
{
    [zr updateFrame];
}


- (void) glkView: (GLKView*) view drawInRect: (CGRect) rect
{
    CGRect bounds = view.bounds;
    CGFloat scale = view.window.screen.scale;
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    [zr drawFrameWithScale: scale width: bounds.size.width height: bounds.size.height];
}


#pragma mark - User Input -


- (void) touchesMoved: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self.view];
    
    [zr addEvent: @{@"type" : @2, @"pos" : NSStringFromCGPoint(location)}];
}


- (void) touchesBegan: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self.view];
    
    [zr addEvent: @{@"type" : @5, @"pos" : NSStringFromCGPoint(location)}];
}


- (void) touchesEnded: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self.view];
    
    [zr addEvent: @{@"type" : @6, @"pos" : NSStringFromCGPoint(location)}];
}


- (void) touchesCancelled: (NSSet*) touches withEvent: (UIEvent*) event
{
    [self touchesEnded: touches withEvent: event];
}


@end


#pragma mark - App Delegate -


@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow* window;

@end


@implementation AppDelegate


- (BOOL) application: (UIApplication*) application didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{
    _window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
    _window.rootViewController = [GameViewController new];
    [_window makeKeyAndVisible];
    
    return YES;
}


@end


int main(int argc, char* argv[])
{
    @autoreleasepool
    {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}


