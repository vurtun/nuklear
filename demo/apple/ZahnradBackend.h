/*
 Copyright (c) 2016 Micha Mettke
 Macintosh / Objective-C adaptation (c) 2016 richi
 
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

#import <Foundation/Foundation.h>

#import "zahnrad.h"

#define SIMULATE_TOUCH 0

#if (TARGET_OS_IPHONE && (!TARGET_OS_TV)) || SIMULATE_TOUCH
    #define ZR_REFRESH_ON_EVENT_ONLY 0
    #define ZR_TOUCH_SCREEN 1
#endif

#if TARGET_OS_IPHONE
    #import <UIKit/UIKit.h>
    #import <GLKit/GLKit.h>
    #import <OpenGLES/ES3/gl.h>
    #import <OpenGLES/ES3/glext.h>
    #define GLVIEW  GLKView
#else
    #import <Cocoa/Cocoa.h>
    #import <OpenGL/OpenGL.h>
    #import <OpenGL/gl.h>
    #import <OpenGL/gl3.h>
    #define GLVIEW  NSOpenGLView
#endif


@interface ZahnradBackend : NSObject


@property (readonly) GLVIEW* view;

- (instancetype) initWithView: (GLVIEW*) view;

- (void) addEvent: (NSDictionary*) event;
- (void) updateFrame;
- (void) drawFrameWithScale: (CGFloat) scale width: (CGFloat) width height: (CGFloat) height;

@end


@interface ZahnradBackend (Adapter)


- (struct zr_context*) createContextWithBuffer: (struct zr_buffer*) cmds
                                     allocator: (struct zr_allocator*) alloc
                                   defaultFont: (struct zr_user_font*) defFont;

- (int) fillFrame;


@end


void zr_backend_show_keyboard(zr_hash hash, struct zr_rect bounds, struct zr_buffer* text);
void zr_backend_hide_keyboard(void);

int zr_touch_edit_string(struct zr_context *ctx, zr_flags flags, char *text, zr_size *len, zr_size max, zr_filter filter, zr_hash unique_id);



