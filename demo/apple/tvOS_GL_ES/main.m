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

    CGPoint cursor;
    UIView* cursorView;
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
    
    UIPanGestureRecognizer* panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action: @selector(pan:)];
    [self.view addGestureRecognizer:panRecognizer];
    
    cursorView = [[UIView alloc] initWithFrame: CGRectMake(100, 100, 10, 10)];
    cursorView.backgroundColor = UIColor.redColor;
    [self.view addSubview: cursorView];
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


- (void) pan: (UIPanGestureRecognizer*) panRecognizer
{
    CGPoint delta = [panRecognizer translationInView: self.view];
    
    cursor.x += delta.x / 33.0;
    cursor.y += delta.y / 33.0;
    
    CGRect bounds = self.view.bounds;
    if (cursor.x < CGRectGetMinX(bounds)) cursor.x = CGRectGetMinX(bounds);
    if (cursor.x > CGRectGetMaxX(bounds)) cursor.x = CGRectGetMaxX(bounds);
    if (cursor.y < CGRectGetMinY(bounds)) cursor.y = CGRectGetMinY(bounds);
    if (cursor.y > CGRectGetMaxY(bounds)) cursor.y = CGRectGetMaxY(bounds);
    
    cursorView.center = cursor;

    [zr addEvent: @{@"type" : @2, @"pos" : NSStringFromCGPoint(cursor)}];
}


- (void) pressesBegan: (NSSet*) presses withEvent: (UIPressesEvent*) event
{
    UIPress* press = [[event allPresses] anyObject];
    if (press.type != UIPressTypeSelect) return;
    
    [zr addEvent: @{@"type" : @5, @"pos" : NSStringFromCGPoint(cursor)}];
}


- (void) pressesEnded: (NSSet*) presses withEvent: (UIPressesEvent*) event
{
    UIPress* press = [[event allPresses] anyObject];
    if (press.type != UIPressTypeSelect) return;

    [zr addEvent: @{@"type" : @6, @"pos" : NSStringFromCGPoint(cursor)}];
}


- (void) pressesCancelled: (NSSet*) presses withEvent: (UIPressesEvent*) event
{
    [self pressesEnded: presses withEvent: event];
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


