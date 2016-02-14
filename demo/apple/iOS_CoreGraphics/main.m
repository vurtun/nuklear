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

#define DEMO_DO_NOT_DRAW_IMAGES
#include "zahnrad.h"
#include "../../demo.c"


static void* mem_alloc(zr_handle unused, size_t size)
{
    return calloc(1, size);
}


static void mem_free(zr_handle unused, void* ptr)
{
    free(ptr);
}


NSString* stringFromZR(const char* text, zr_size len)
{
    char cstr[len + 1];
    strncpy(cstr, text, len);
    cstr[len] = 0;
    return [NSString stringWithUTF8String: cstr];
}


static zr_size fontTextWidth(zr_handle handle, float height, const char* text, zr_size len)
{
    UIFont* font = (__bridge UIFont*) handle.ptr;
    NSString* aString = stringFromZR(text, len);
    NSDictionary* attributes = @{ NSFontAttributeName: font};
    CGSize stringSize = [aString sizeWithAttributes: attributes];
    
    return ceil(stringSize.width);
}


#pragma mark - ZRView -


@interface ZRView : UIView
{
    UIFont* font;
    struct zr_user_font usrfnt;
    struct zr_allocator alloc;

    struct demo gui;
}

@end


@implementation ZRView


- (instancetype) initWithFrame: (CGRect) frame
{
    if (!(self = [super initWithFrame: frame]))
        return self;
    
    alloc.userdata.ptr = NULL;
    alloc.alloc = mem_alloc;
    alloc.free = mem_free;
    
    font = [UIFont systemFontOfSize: 11];
    font = [UIFont fontWithName: @"Menlo-Regular" size: 11];
    
    NSDictionary* attributes = @{NSFontAttributeName: font};
    CGSize stringSize = [@"W" sizeWithAttributes: attributes];
    
    memset(&usrfnt, 0, sizeof(usrfnt));
    usrfnt.height = round(stringSize.height);
    usrfnt.width = fontTextWidth;
    usrfnt.userdata.ptr = (__bridge void*)(font);
    
    zr_init(&gui.ctx, &alloc, &usrfnt);
    zr_clear(&gui.ctx);
    run_demo(&gui);
    
    return self;
}


#define setColor(_c) \
CGContextSetRGBStrokeColor(context, _c->color.r / 255.0, _c->color.g / 255.0, _c->color.b / 255.0, _c->color.a / 255.0); \
CGContextSetRGBFillColor(context, _c->color.r / 255.0, _c->color.g / 255.0, _c->color.b / 255.0, _c->color.a / 255.0);


- (void) drawRect: (CGRect) rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    UIGraphicsPushContext(context);
    
    UIColor* color = [UIColor blackColor];
    CGContextSetFillColorWithColor(context, color.CGColor);
    CGContextFillRect(context, rect);
    
    const struct zr_command* cmd;
    
    CGContextSaveGState(context);
    
    zr_foreach(cmd, &gui.ctx)
    {
        switch (cmd->type)
        {
            case ZR_COMMAND_SCISSOR:
            {
                const struct zr_command_scissor* s = zr_command(scissor, cmd);
                CGRect cr = CGRectMake(s->x, s->y, s->w, s->h);
                CGContextRestoreGState(context);
                CGContextSaveGState(context);
                CGContextClipToRect(context, cr);
            }   break;
                
            case ZR_COMMAND_LINE:
            {
                const struct zr_command_line* l = zr_command(line, cmd);
                setColor(l);
                CGContextBeginPath(context);
                CGContextMoveToPoint(context, l->begin.x + .5, l->begin.y + .5);
                CGContextAddLineToPoint(context, l->end.x + .5, l->end.y + .5);
                CGContextStrokePath(context);
            }   break;
                
            case ZR_COMMAND_RECT:
            {
                const struct zr_command_rect* r = zr_command(rect, cmd);
                CGRect cr = CGRectMake(r->x, r->y, r->w, r->h);
                setColor(r);
                if (!r->rounding) CGContextFillRect(context, cr);
                else
                {
                    CGMutablePathRef p = CGPathCreateMutable();
                    CGPathAddRoundedRect(p, nil, cr, r->rounding, r->rounding);
                    CGPathCloseSubpath(p);
                    CGContextBeginPath(context);
                    CGContextAddPath(context, p);
                    CGContextDrawPath(context, kCGPathFill);
                    CGPathRelease(p);
                }
            }   break;
                
            case ZR_COMMAND_CIRCLE:
            {
                const struct zr_command_circle* c = zr_command(circle, cmd);
                CGRect cr = CGRectMake(c->x, c->y, c->w, c->h);
                setColor(c);
                CGContextFillEllipseInRect(context, cr);
            }   break;
                
            case ZR_COMMAND_TRIANGLE:
            {
                const struct zr_command_triangle* t = zr_command(triangle, cmd);
                setColor(t);
                CGContextBeginPath(context);
                CGContextMoveToPoint   (context, t->a.x, t->a.y);
                CGContextAddLineToPoint(context, t->b.x, t->b.y);
                CGContextAddLineToPoint(context, t->c.x, t->c.y);
                CGContextClosePath(context);
                CGContextFillPath(context);
            }   break;
                
            case ZR_COMMAND_TEXT:
            {
                const struct zr_command_text* t = zr_command(text, cmd);
                CGRect cr = CGRectMake(t->x, t->y, t->w, t->h);
                CGContextSetRGBFillColor(context, t->background.r / 255.0, t->background.g / 255.0, t->background.b / 255.0, t->background.a / 255.0);
                CGContextFillRect(context, cr);
                NSString* aString = stringFromZR(t->string, t->length);
                UIColor* fg = [UIColor colorWithRed: t->foreground.r / 255.0 green: t->foreground.g / 255.0 blue: t->foreground.b / 255.0 alpha: t->foreground.a / 255.0];
                UIColor* bg = [UIColor colorWithRed: t->background.r / 255.0 green: t->background.g / 255.0 blue: t->background.b / 255.0 alpha: t->background.a / 255.0];
                NSDictionary* attributes = @{ NSFontAttributeName: font, NSForegroundColorAttributeName: fg, NSBackgroundColorAttributeName: bg };
                CGSize size = [aString sizeWithAttributes: attributes];
                CGRect xr = CGRectMake(cr.origin.x, cr.origin.y + (cr.size.height - size.height) / 2.0, cr.size.width, size.height);
                [aString drawInRect: xr withAttributes: attributes];
            }   break;
                
            case ZR_COMMAND_CURVE:
            case ZR_COMMAND_IMAGE:
            case ZR_COMMAND_ARC:
            case ZR_COMMAND_NOP:
            default:
                break;
        }
    }
    
    UIGraphicsPopContext();
}


- (void) touchesBegan: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self];
    
    zr_input_begin(&gui.ctx);
    zr_input_motion(&gui.ctx, location.x, location.y);
    zr_input_end(&gui.ctx);
    
    zr_clear(&gui.ctx);
    zr_input_begin(&gui.ctx);
    zr_input_button(&gui.ctx, ZR_BUTTON_LEFT, location.x, location.y, zr_true);
    zr_input_end(&gui.ctx);
    
    run_demo(&gui);
    [self setNeedsDisplay];
}


- (void) touchesMoved: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self];
    
    zr_clear(&gui.ctx);
    
    zr_input_begin(&gui.ctx);
    zr_input_motion(&gui.ctx, location.x, location.y);
    zr_input_end(&gui.ctx);
    
    run_demo(&gui);
    [self setNeedsDisplay];
}


- (void) touchesEnded: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self];
    
    zr_clear(&gui.ctx);
    zr_input_begin(&gui.ctx);
    zr_input_motion(&gui.ctx, location.x, location.y);
    zr_input_button(&gui.ctx, ZR_BUTTON_LEFT, location.x, location.y, zr_false);
    zr_input_end(&gui.ctx);
    run_demo(&gui);
    
    zr_clear(&gui.ctx);
    zr_input_begin(&gui.ctx);
    zr_input_motion(&gui.ctx, 0, 0);
    zr_input_end(&gui.ctx);

    run_demo(&gui);
    [self setNeedsDisplay];
}


- (void) touchesCancelled: (NSSet*) touches withEvent: (UIEvent*) event
{
    [self touchesEnded: touches withEvent: event];
}


@end


#pragma mark - ViewController -


@interface ViewController : UIViewController

@property (strong, nonatomic)  ZRView* zrView;

@end


@implementation ViewController


- (void) viewDidLoad
{
    [super viewDidLoad];
    
    _zrView = [[ZRView alloc] initWithFrame: self.view.bounds];
    [self.view addSubview: _zrView];
}


@end


#pragma mark - App Delegate -


@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) ViewController* viewController;

@end


@implementation AppDelegate


- (BOOL) application: (UIApplication*) application didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{
    _window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
    _viewController = [ViewController new];
    _window.rootViewController = _viewController;
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


