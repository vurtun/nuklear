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


@interface GameViewController : GLKViewController <UITextFieldDelegate>
@end


@implementation GameViewController
{
    NSUInteger keyboardHash;
    UITextField* textInputField;
    
    CGPoint cursor;
    UIView* cursorView;

    EAGLContext* context;
    ZahnradBackend* zr;
    
}


- (void) viewDidLoad
{
    [super viewDidLoad];
    
    keyboardHash = -1;
    
    context = [[EAGLContext alloc] initWithAPI: kEAGLRenderingAPIOpenGLES3];
    assert(context != nil);
    
    ((GLKView*)self.view).context = context;
    ((GLKView*)self.view).drawableDepthFormat = GLKViewDrawableDepthFormatNone;
    
    [EAGLContext setCurrentContext: context];
    zr = [[ZahnradBackend alloc] initWithView: ((GLKView*)self.view)];
    
    UIPanGestureRecognizer* panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action: @selector(pan:)];
    [self.view addGestureRecognizer:panRecognizer];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        cursor = CGPointMake(95, 70);
        cursorView = [[UIView alloc] initWithFrame: CGRectMake(cursor.x, cursor.y, 10, 10)];
        cursorView.backgroundColor = UIColor.redColor;
        [self.view addSubview: cursorView];
        [zr addEvent: @{@"type" : @2, @"pos" : NSStringFromCGPoint(cursor)}];
    });
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
    
#if 0
    
    CGFloat s = 33.0;
    
    cursor.x += delta.x / s;
    cursor.y += delta.y / s;

#else
    
    CGFloat s = 150.0;
    
    if (delta.x != 0.0)
        cursor.x += pow(2.0, fabs(delta.x / s)) * (delta.x > 0.0 ? 1.0 : -1.0);
    if (delta.y != 0.0)
        cursor.y += pow(2.0, fabs(delta.y / s)) * (delta.y > 0.0 ? 1.0 : -1.0);
#endif
    
    
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



- (BOOL) textFieldShouldReturn: (UITextField*) textField
{
    return YES;
}


- (void) textFieldDidEndEditing: (UITextField*) textField
{
    [textInputField resignFirstResponder];
    [textInputField removeFromSuperview];
    textInputField = nil;

    [zr addEvent: @{@"type" : @12, @"txt" : textField.text, @"mod" : @0}];
}


- (void) showKeyboard: (NSDictionary*) info
{
    NSUInteger hash = [info[@"hash"] unsignedIntegerValue];
    
    if (hash != keyboardHash)
    {
        keyboardHash = hash;
        if (!textInputField)
        {
            CGRect frame = CGRectFromString(info[@"frame"]);
            textInputField = [[UITextField alloc] initWithFrame: frame];
            textInputField.delegate = self;
            textInputField.text = info[@"text"];
            [self.view addSubview: textInputField];
            [textInputField becomeFirstResponder];
        }
    }
}


- (void) hideKeyboard
{
    [textInputField resignFirstResponder];
    [textInputField removeFromSuperview];
    textInputField = nil;

    keyboardHash = -1;
}


@end


#pragma mark - App Delegate -


@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) GameViewController* gameViewController;

@end


@implementation AppDelegate


- (void) showKeyboard: (NSDictionary*) info
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.gameViewController showKeyboard: info];
    });
}


- (void) hideKeyboard
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.gameViewController hideKeyboard];
    });
}


- (BOOL) application: (UIApplication*) application didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{
    _window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
    _gameViewController = [GameViewController new];
    _window.rootViewController = _gameViewController;
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


