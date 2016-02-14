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

#import "ZahnradBackend.h"


#define MAX_VERTEX_MEMORY   (512 * 1024)
#define MAX_ELEMENT_MEMORY  (128 * 1024)


// This is the adapter for demo.c.
// #else does provide a generic version to start
// your own project.


#if 1

#if TARGET_OS_IPHONE

#define zr_edit_string(_ct, _fl, _bu, _le, _ma, _fi) zr_touch_edit_string(_ct, _fl, _bu, _le, _ma, _fi, (zr_hash)__COUNTER__)

#endif


#define DEMO_DO_NOT_DRAW_IMAGES
#include "../demo.c"

@implementation ZahnradBackend (Adapter)

static struct demo gui;


- (struct zr_context*) createContextWithBuffer: (struct zr_buffer*) cmds
                                     allocator: (struct zr_allocator*) alloc
                                   defaultFont: (struct zr_user_font*) defFont
{
    memset(&gui, 0, sizeof(gui));
    
    zr_buffer_init(cmds, alloc, 4096);
    zr_init(&gui.ctx, alloc, defFont);
    
    return &gui.ctx;
}


- (int) fillFrame;
{
    return run_demo(&gui);
    
}


@end


#else


#import "ZahnradBackend.h"


@implementation ZahnradBackend (Adapter)

static struct zr_context context;


- (struct zr_context*) createContextWithBuffer: (struct zr_buffer*) cmds
                                     allocator: (struct zr_allocator*) alloc
                                   defaultFont: (struct zr_user_font*) defFont
{
    memset(&context, 0, sizeof(context));
    
    zr_buffer_init(cmds, alloc, 4096);
    zr_init(&context, alloc, defFont);
    
    return &context;
}


- (int) fillFrame;
{
    return 1;
}

@end


#endif


@implementation ZahnradBackend
{
    struct zr_context* context;
    
    struct zr_allocator alloc;
    struct zr_user_font sysFnt;
    struct zr_font font;
    struct zr_draw_null_texture nullTexture;
    
    struct zr_buffer cmds;
    NSMutableArray* bufferedCommands;
    NSMutableArray* events;
    
    struct
    {
        GLuint vbo, vao, ebo;
        GLuint program;
        GLuint vertexShader;
        GLuint fragmentShader;
        GLint attributePosition;
        GLint attributeUV;
        GLint attributeColor;
        GLint uniformTexture;
        GLint uniformProjection;
        GLuint fontTexture;
    } device;
}


static void* mem_alloc(zr_handle unused, size_t size)
{
    return calloc(1, size);
}


static void mem_free(zr_handle unused, void* ptr)
{
    free(ptr);
}


#pragma mark - Setup -


- (instancetype) initWithView: (GLVIEW*) view;
{
    if (!(self = [super init])) return self;
    
    _view = view;
    
    events = [NSMutableArray new];
    bufferedCommands = [NSMutableArray new];
    
    [self setupGL];
    
    NSURL* fontURL = [[NSBundle mainBundle] URLForResource: @"DroidSans" withExtension: @"ttf"];
    assert(fontURL != nil);
    
    alloc.userdata.ptr = NULL;
    alloc.alloc = mem_alloc;
    alloc.free = mem_free;
    
    sysFnt = [self fontFromUrl: fontURL height: 14 range: zr_font_default_glyph_ranges()];
    
    context = [self createContextWithBuffer: &cmds allocator: &alloc defaultFont: &sysFnt];
    [self fillFrame];
    
    return self;
}


- (void) setupGL
{
    GLint status;
    
    static const GLchar* vss =
#if TARGET_OS_IPHONE
    "#version 300 es\n"
#else
    "#version 150\n"
#endif
    "uniform mat4 ProjMtx;\n"
    "in vec2 Position;\n"
    "in vec2 TexCoord;\n"
    "in vec4 Color;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main() {\n"
    "   Frag_UV = TexCoord;\n"
    "   Frag_Color = Color;\n"
    "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
    "}\n";
    
    static const GLchar* fss =
#if TARGET_OS_IPHONE
    "#version 300 es\n"
#else
    "#version 150\n"
#endif
    "precision mediump float;\n"
    "uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "out vec4 Out_Color;\n"
    "void main(){\n"
    "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
    "}\n";
    
    device.program = glCreateProgram();
    device.vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(device.vertexShader, 1, &vss, 0);
    glCompileShader(device.vertexShader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(device.vertexShader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetShaderInfoLog(device.vertexShader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(device.vertexShader, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    
    device.fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(device.fragmentShader, 1, &fss, 0);
    glCompileShader(device.fragmentShader);
    
#if defined(DEBUG)
    glGetShaderiv(device.fragmentShader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetShaderInfoLog(device.fragmentShader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(device.fragmentShader, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    
    glAttachShader(device.program, device.vertexShader);
    glAttachShader(device.program, device.fragmentShader);
    glLinkProgram(device.program);
    
#if defined(DEBUG)
    glGetProgramiv(device.program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetProgramInfoLog(device.program, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(device.program, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);
    
    device.uniformTexture = glGetUniformLocation(device.program, "Texture");
    device.uniformProjection = glGetUniformLocation(device.program, "ProjMtx");
    device.attributePosition = glGetAttribLocation(device.program, "Position");
    device.attributeUV = glGetAttribLocation(device.program, "TexCoord");
    device.attributeColor = glGetAttribLocation(device.program, "Color");
    
    // buffer setup
    GLsizei vs = sizeof(struct zr_draw_vertex);
    size_t vp = offsetof(struct zr_draw_vertex, position);
    size_t vt = offsetof(struct zr_draw_vertex, uv);
    size_t vc = offsetof(struct zr_draw_vertex, col);
    
    // allocate vertex and element buffer
    glGenBuffers(1, &device.vbo);
    glGenBuffers(1, &device.ebo);
    glGenVertexArrays(1, &device.vao);
    
    glBindVertexArray(device.vao);
    glBindBuffer(GL_ARRAY_BUFFER, device.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device.ebo);

    glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, NULL, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, NULL, GL_STREAM_DRAW);
    
    glEnableVertexAttribArray((GLuint)device.attributePosition);
    glEnableVertexAttribArray((GLuint)device.attributeUV);
    glEnableVertexAttribArray((GLuint)device.attributeColor);
    
    glVertexAttribPointer((GLuint)device.attributePosition, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
    glVertexAttribPointer((GLuint)device.attributeUV, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
    glVertexAttribPointer((GLuint)device.attributeColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


- (struct zr_user_font) fontFromUrl: (NSURL*) url height: (unsigned) fontHeight range: (const zr_rune*) range
{
    struct zr_baked_font baked_font;
    struct zr_user_font user_font;
    struct zr_recti custom;
    
    memset(&baked_font, 0, sizeof(baked_font));
    memset(&user_font, 0, sizeof(user_font));
    memset(&custom, 0, sizeof(custom));
    
    // bake and upload font texture
    NSData* ttfData = [NSData dataWithContentsOfURL: url];
    assert(ttfData != nil);
    
    // setup font configuration
    struct zr_font_config config;
    memset(&config, 0, sizeof(config));
    config.ttf_blob = (void*)ttfData.bytes;
    config.font = &baked_font;
    config.coord_type = ZR_COORD_UV;
    config.range = range;
    config.pixel_snap = zr_false;
    config.size = (float)fontHeight;
    config.spacing = zr_vec2(0, 0);
    config.oversample_h = 1;
    config.oversample_v = 1;
    
    // query needed amount of memory for the font baking process
    int glyph_count;
    size_t tmp_size;
    zr_font_bake_memory(&tmp_size, &glyph_count, &config, 1);
    struct zr_font_glyph* glyphes = (struct zr_font_glyph*)calloc(sizeof(struct zr_font_glyph), (size_t)glyph_count);
    void* tmp = calloc(1, tmp_size);
    
    // pack all glyphes and return needed image width, height and memory size
    int img_width, img_height;
    size_t img_size;
    custom.w = 2; custom.h = 2;
    assert(zr_font_bake_pack(&img_size, &img_width, &img_height, &custom, tmp, tmp_size, &config, 1));
    
    // bake all glyphes and custom white pixel into image
    const char* custom_data = "....";
    void* img = calloc(1, img_size);
    zr_font_bake(img, img_width, img_height, tmp, tmp_size, glyphes, glyph_count, &config, 1);
    zr_font_bake_custom_data(img, img_width, img_height, custom, custom_data, 2, 2, '.', 'X');
    
    // convert alpha8 image into rgba8 image
    void* img_rgba = calloc(4, (size_t)(img_height * img_width));
    zr_font_bake_convert(img_rgba, img_width, img_height, img);
    free(img);
    img = img_rgba;
    
    // upload baked font image
    glGenTextures(1, &device.fontTexture);
    glBindTexture(GL_TEXTURE_2D, device.fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)img_width, (GLsizei)img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    
    free(tmp);
    free(img);
    
    // default white pixel in a texture which is needed to draw primitives
    nullTexture.texture.id = (int)device.fontTexture;
    nullTexture.uv = zr_vec2((custom.x + 0.5f) / (float)img_width, (custom.y + 0.5f) / (float)img_height);
    
    /* setup font with glyphes. IMPORTANT: the font only references the glyphes
     this was done to have the possibility to have multible fonts with one
     total glyph array. Not quite sure if it is a good thing since the
     glyphes have to be freed as well.
     */
    zr_font_init(&font, (float)fontHeight, '?', glyphes, &baked_font, nullTexture.texture);
    
    return zr_font_ref(&font);
}


#pragma mark - Drawing -


- (void) drawFrameWithScale: (CGFloat) scale width: (CGFloat) width height: (CGFloat) height
{
    // save opengl state
    GLint last_prog, last_tex;
    GLint last_ebo, last_vbo, last_vao;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_prog);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_tex);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_vao);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_ebo);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vbo);
    
    // setup global state
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);
    
    // setup program
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, -2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= width;
    ortho[1][1] /= height;
    glUseProgram(device.program);
    glUniform1i(device.uniformTexture, 0);
    glUniformMatrix4fv(device.uniformProjection, 1, GL_FALSE, &ortho[0][0]);
    
    // activate vertex and element buffer
    glBindVertexArray(device.vao);
    glBindBuffer(GL_ARRAY_BUFFER, device.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device.ebo);

    // if draw commands have been buffered we will draw only those
    if (bufferedCommands.count)
    {
        const struct zr_draw_command* cmd;
        const zr_draw_index* offset = NULL;
        
        // iterate over and execute each draw command
        for (NSData* data in bufferedCommands)
        {
            cmd = data.bytes;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)(cmd->clip_rect.x * scale),
                      (GLint)((height - (cmd->clip_rect.y + cmd->clip_rect.h)) * scale),
                      (GLint)(cmd->clip_rect.w * scale),
                      (GLint)(cmd->clip_rect.h * scale));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
    }
    else
    {
        // load draw vertices & elements  into vertex + element buffer
        void* vertices = glMapBufferRange(GL_ARRAY_BUFFER, 0, MAX_VERTEX_MEMORY, GL_MAP_WRITE_BIT);
        void* elements = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, MAX_ELEMENT_MEMORY, GL_MAP_WRITE_BIT);
        
        // fill converting configuration
        struct zr_convert_config config;
        memset(&config, 0, sizeof(config));
        config.global_alpha = 1.0f;
        config.shape_AA = ZR_ANTI_ALIASING_ON;
        config.line_AA = ZR_ANTI_ALIASING_ON;
        config.circle_segment_count = 22;
        config.line_thickness = 1.0f;
        config.null = nullTexture;
        
        // setup buffers to load vertices and elements
        struct zr_buffer vbuf, ebuf;
        zr_buffer_init_fixed(&vbuf, vertices, MAX_VERTEX_MEMORY);
        zr_buffer_init_fixed(&ebuf, elements, MAX_ELEMENT_MEMORY);
        zr_convert(context, &cmds, &vbuf, &ebuf, &config);

        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        
        // convert from command queue into draw list and draw to screen
        const struct zr_draw_command* cmd;
        const zr_draw_index* offset = NULL;
        
        [bufferedCommands removeAllObjects];
        
        // iterate over and execute each draw command
        zr_draw_foreach(cmd, context, &cmds)
        {
            if (cmd->elem_count > 0)
            {
#if ZR_REFRESH_ON_EVENT_ONLY
                [bufferedCommands addObject: [NSData dataWithBytes: cmd length: sizeof(struct zr_draw_command)]];
#endif
                glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
                glScissor((GLint)(cmd->clip_rect.x * scale),
                          (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale),
                          (GLint)(cmd->clip_rect.w * scale),
                          (GLint)(cmd->clip_rect.h * scale));
                glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
                offset += cmd->elem_count;
            }
        }
        
        zr_clear(context);
    }
    
    // restore old state
    glUseProgram((GLuint)last_prog);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_tex);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)last_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)last_ebo);
    glBindVertexArray((GLuint)last_vao);
    glDisable(GL_SCISSOR_TEST);
}


#pragma mark - Events -


- (void) updateFrame
{

#if ZR_REFRESH_ON_EVENT_ONLY
    if (events.count)
#endif
    {
        [self handleEvents];
        [self fillFrame];
    }
}


- (void) handleEvents
{
    NSArray* currentEvents = events.copy;
    [events removeAllObjects];
    
    zr_input_begin(context);
    
    for (NSDictionary* event in currentEvents)
    {
#if TARGET_OS_IPHONE
        CGPoint location = CGPointFromString(event[@"pos"]);
#else
        NSPoint location = NSPointFromString(event[@"pos"]);
        location.y = (int)(self.view.bounds.size.height - location.y);
#endif
        
        int x = (int)location.x;
        int y = (int)location.y;
        int type = [event[@"type"] intValue];
        
        switch (type)
        {
                
#if ZR_REFRESH_ON_EVENT_ONLY || ZR_TOUCH_SCREEN
                
            case 2: case 3: case 4:
                zr_input_motion(context, x, y);
                break;
            case 5:
                zr_input_motion(context, x, y);
                zr_input_end(context);
                zr_input_begin(context);
                zr_input_button(context, ZR_BUTTON_LEFT, x, y, zr_true);
                break;
            case 6:
                zr_input_button(context, ZR_BUTTON_LEFT, x, y, zr_false);
                zr_input_end(context);
                [self fillFrame];
                zr_clear(context);
                zr_input_begin(context);
                zr_input_motion(context, INT_MAX, INT_MAX);
                break;

#else
                
            case 1: case 2: case 3: case 4:
                zr_input_motion(context, x, y);
                break;
            case 5:
                zr_input_button(context, ZR_BUTTON_LEFT, x, y, zr_true);
                break;
            case 6:
                zr_input_button(context, ZR_BUTTON_LEFT, x, y, zr_false);
                break;
            case 7:
                zr_input_button(context, ZR_BUTTON_RIGHT, x, y, zr_true);
                break;
            case 8:
                zr_input_button(context, ZR_BUTTON_RIGHT, x, y, zr_false);
                break;
            case 9:
                zr_input_button(context, ZR_BUTTON_MIDDLE, x, y, zr_true);
                break;
            case 10:
                zr_input_button(context, ZR_BUTTON_MIDDLE, x, y, zr_false);
                break;
            case 11:
                zr_input_scroll(context, -[event[@"deltaY"] floatValue]);
                break;
#endif
                
            case 12:
            case 13:
            {
                int down = type == 12 ? zr_true : zr_false;
                NSString* str = event[@"txt"];
                NSData* data = [str dataUsingEncoding: NSUTF32LittleEndianStringEncoding];
                const uint32_t* c = data.bytes;
                NSInteger n = data.length / sizeof(uint32_t);
                
#if TARGET_OS_IPHONE
                
                if (down)
                {
                    if (n && *c == '\b')
                    {
                        zr_input_key(context, ZR_KEY_BACKSPACE, zr_true);
                        zr_input_end(context);
                        [self fillFrame];
                        zr_clear(context);
                        zr_input_begin(context);
                        zr_input_key(context, ZR_KEY_BACKSPACE, zr_false);
                    }
                    else
                    {
                        for (NSInteger i = 0; i < n; i += 1, c += 1)
                            zr_input_unicode(context, *c);
                    }
                }
                
#else
                
                for (NSInteger i = 0; i < n; i += 1, c += 1)
                {
                    if (*c == 127 || *c < ' ' || *c >= NSUpArrowFunctionKey)
                    {
                        switch (*c)
                        {
                            case NSLeftArrowFunctionKey:
                                zr_input_key(context, ZR_KEY_LEFT, down);
                                break;
                            case NSRightArrowFunctionKey:
                                zr_input_key(context, ZR_KEY_RIGHT, down);
                                break;
                            case 3:
                                zr_input_key(context, ZR_KEY_COPY, down);
                                break;
                            case 22:
                                zr_input_key(context, ZR_KEY_PASTE, down);
                                break;
                            case 24:
                                zr_input_key(context, ZR_KEY_CUT, down);
                                break;
                            case 9:
                                zr_input_key(context, ZR_KEY_TAB, down);
                                break;
                            case 13:
                                zr_input_key(context, ZR_KEY_ENTER, down);
                                break;
                            case 127:
                                zr_input_key(context, ZR_KEY_BACKSPACE, down);
                                break;
                            default:
                                break;
                        }
                    }
                    else if (down)
                    {
                        zr_input_unicode(context, *c);
                    }
                }
#endif
            }
            break;
                
            default:
                break;
        }
    }
    
    zr_input_end(context);
}


- (void) addEvent: (NSDictionary*) event
{
    [bufferedCommands removeAllObjects];
    [events addObject: event];
}


@end


@protocol ZahnradKeyboardDelegate <NSObject>

- (void) showKeyboard: (NSDictionary*) info;
- (void) hideKeyboard;

@end


#undef zr_edit_string


#if TARGET_OS_IPHONE


void zr_backend_show_keyboard(zr_hash zrHash, struct zr_rect zrBounds, struct zr_buffer* zrText)
{
    CGRect frame = CGRectMake(zrBounds.x, zrBounds.y, zrBounds.w, zrBounds.h);
    id ad = [[UIApplication sharedApplication] delegate];
    if ([ad respondsToSelector: @selector(showKeyboard:)])
    {
        char str[zrText->allocated + 1];
        strncpy(str, zrText->memory.ptr, zrText->allocated);
        str[zrText->allocated] = 0;
        NSString* text = [NSString stringWithCString: str encoding: NSUTF8StringEncoding];
        [ad performSelector: @selector(showKeyboard:) withObject: @{@"hash" : @(zrHash), @"text" : text, @"frame" : NSStringFromCGRect(frame)}];
    }
}


void zr_backend_hide_keyboard(void)
{
    id ad = [[UIApplication sharedApplication] delegate];
    if ([ad respondsToSelector: @selector(hideKeyboard)])
        [ad performSelector: @selector(hideKeyboard)];
}


int zr_touch_edit_string(struct zr_context *ctx, zr_flags flags, char *text, zr_size *len, zr_size max, zr_filter filter, zr_hash unique_id)
{
    zr_flags state;
    struct zr_rect bounds;
    
    zr_layout_peek(&bounds, ctx);
    state = zr_edit_string(ctx, flags, text, len, max, filter);
    if (state & ZR_EDIT_ACTIVATED)
    {
        struct zr_buffer buffer;
        zr_buffer_init_fixed(&buffer, text, max);
        buffer.allocated = *len;
        
        zr_backend_show_keyboard((zr_hash)unique_id, bounds, &buffer);
    }
    else if (state & ZR_EDIT_DEACTIVATED)
    {
        zr_backend_hide_keyboard();
    }
    
    return state;
}


#else


void zr_backend_show_keyboard(zr_hash zrHash, struct zr_rect zrBounds, struct zr_buffer* zrText)
{
}


void zr_backend_hide_keyboard(void)
{
}


int zr_touch_edit_string(struct zr_context *ctx, zr_flags flags, char *text, zr_size *len, zr_size max, zr_filter filter, zr_hash unique_id)
{
    return  zr_edit_string(ctx, flags, text, len, max, filter);
}


#endif



