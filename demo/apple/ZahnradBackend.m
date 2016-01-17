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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "../../zahnrad.h"
#include "../demo.c"

#undef MAX
#undef MIN


#import "ZahnradBackend.h"

#define MAX_VERTEX_MEMORY   (512 * 1024)
#define MAX_ELEMENT_MEMORY  (128 * 1024)


static void* mem_alloc(zr_handle unused, size_t size)
{
    return calloc(1, size);
}


static void mem_free(zr_handle unused, void* ptr)
{
    free(ptr);
}


struct device
{
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
    struct zr_draw_null_texture null;
    struct zr_buffer cmds;
};


@implementation ZahnradBackend
{
    struct demo gui;
    struct device device;
    struct zr_font font;
    struct zr_user_font usrfnt;
    struct zr_allocator alloc;
    
    NSMutableArray* events;
    NSMutableArray* drawCommands;
}


#pragma mark - Setup -


- (instancetype) initWithView: (GLVIEW*) view;
{
    if (!(self = [super init])) return self;
    
    _view = view;
    
    events = [NSMutableArray new];
    drawCommands = [NSMutableArray new];
    
    [self setupGL];
    
    NSURL* fontURL = [[NSBundle mainBundle] URLForResource: @"DroidSans" withExtension: @"ttf"];
    assert(fontURL != nil);
    
    alloc.userdata.ptr = NULL;
    alloc.alloc = mem_alloc;
    alloc.free = mem_free;
    
    zr_buffer_init(&device.cmds, &alloc, 4096);
    usrfnt = [self fontFromUrl: fontURL height: 14 range: zr_font_default_glyph_ranges()];
    zr_init(&gui.ctx, &alloc, &usrfnt);
    
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
    
    device.prog = glCreateProgram();
    device.vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(device.vert_shdr, 1, &vss, 0);
    glCompileShader(device.vert_shdr);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(device.vert_shdr, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetShaderInfoLog(device.vert_shdr, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(device.vert_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    
    device.frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(device.frag_shdr, 1, &fss, 0);
    glCompileShader(device.frag_shdr);
    
#if defined(DEBUG)
    glGetShaderiv(device.frag_shdr, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetShaderInfoLog(device.frag_shdr, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(device.frag_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    
    glAttachShader(device.prog, device.vert_shdr);
    glAttachShader(device.prog, device.frag_shdr);
    glLinkProgram(device.prog);
    
#if defined(DEBUG)
    glGetProgramiv(device.prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetProgramInfoLog(device.prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(device.prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);
    
    device.uniform_tex = glGetUniformLocation(device.prog, "Texture");
    device.uniform_proj = glGetUniformLocation(device.prog, "ProjMtx");
    device.attrib_pos = glGetAttribLocation(device.prog, "Position");
    device.attrib_uv = glGetAttribLocation(device.prog, "TexCoord");
    device.attrib_col = glGetAttribLocation(device.prog, "Color");
    
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
    
    glEnableVertexAttribArray((GLuint)device.attrib_pos);
    glEnableVertexAttribArray((GLuint)device.attrib_uv);
    glEnableVertexAttribArray((GLuint)device.attrib_col);
    
    glVertexAttribPointer((GLuint)device.attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
    glVertexAttribPointer((GLuint)device.attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
    glVertexAttribPointer((GLuint)device.attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    
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
    glGenTextures(1, &device.font_tex);
    glBindTexture(GL_TEXTURE_2D, device.font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)img_width, (GLsizei)img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    
    free(tmp);
    free(img);
    
    // default white pixel in a texture which is needed to draw primitives
    device.null.texture.id = (int)device.font_tex;
    device.null.uv = zr_vec2((custom.x + 0.5f) / (float)img_width,
                             (custom.y + 0.5f) / (float)img_height);
    
    /* setup font with glyphes. IMPORTANT: the font only references the glyphes
     this was done to have the possibility to have multible fonts with one
     total glyph array. Not quite sure if it is a good thing since the
     glyphes have to be freed as well.
     */
    zr_font_init(&font, (float)fontHeight, '?', glyphes, &baked_font, device.null.texture);
    
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
    glUseProgram(device.prog);
    glUniform1i(device.uniform_tex, 0);
    glUniformMatrix4fv(device.uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    
    // activate vertex and element buffer
    glBindVertexArray(device.vao);
    glBindBuffer(GL_ARRAY_BUFFER, device.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device.ebo);

    // if draw commands have been buffered we will draw only those
    if (drawCommands.count)
    {
        const struct zr_draw_command* cmd;
        const zr_draw_index* offset = NULL;
        
        // iterate over and execute each draw command
        for (NSData* data in drawCommands)
        {
            cmd = data.bytes;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)(cmd->clip_rect.x * scale),
                      (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale),
                      (GLint)(cmd->clip_rect.w * scale),
                      (GLint)(cmd->clip_rect.h * scale));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
    }
    else
    {
        // load draw vertexes & elements  into vertex + element buffer
        void* vertexes = glMapBufferRange(GL_ARRAY_BUFFER, 0, MAX_VERTEX_MEMORY, GL_MAP_WRITE_BIT);
        void* elements = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, MAX_ELEMENT_MEMORY, GL_MAP_WRITE_BIT);
        
        // fill converting configuration
        struct zr_convert_config config;
        memset(&config, 0, sizeof(config));
        config.shape_AA = ZR_ANTI_ALIASING_ON;
        config.line_AA = ZR_ANTI_ALIASING_ON;
        config.circle_segment_count = 22;
        config.line_thickness = 1.0f;
        config.null = device.null;
        
        // setup buffers to load vertexes and elements
        struct zr_buffer vbuf, ebuf;
        zr_buffer_init_fixed(&vbuf, vertexes, MAX_VERTEX_MEMORY);
        zr_buffer_init_fixed(&ebuf, elements, MAX_ELEMENT_MEMORY);
        zr_convert(&gui.ctx, &device.cmds, &vbuf, &ebuf, &config);

        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        
        // convert from command queue into draw list and draw to screen
        const struct zr_draw_command* cmd;
        const zr_draw_index* offset = NULL;
        
        [drawCommands removeAllObjects];
        
        // iterate over and execute each draw command
        zr_draw_foreach(cmd, &gui.ctx, &device.cmds)
        {
            if (cmd->elem_count > 0)
            {
#if ZR_REFRESH_ON_EVENT_ONLY
                [drawCommands addObject: [NSData dataWithBytes: cmd length: sizeof(struct zr_draw_command)]];
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
        
        zr_clear(&gui.ctx);
   }
    
    // restore old state
    glUseProgram((GLuint)last_prog);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_tex);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)last_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)last_ebo);
    glBindVertexArray((GLuint)last_vao);
    glDisable(GL_SCISSOR_TEST);
}


- (void) fillFrame
{
    run_demo(&gui);
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
    
    zr_input_begin(&gui.ctx);
    
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
                zr_input_motion(&gui.ctx, x, y);
                break;
            case 5:
                zr_input_motion(&gui.ctx, x, y);
                zr_input_end(&gui.ctx);
                zr_input_begin(&gui.ctx);
                zr_input_button(&gui.ctx, ZR_BUTTON_LEFT, x, y, zr_true);
                break;
            case 6:
                zr_input_button(&gui.ctx, ZR_BUTTON_LEFT, x, y, zr_false);
                zr_input_end(&gui.ctx);
                [self fillFrame];
                zr_clear(&gui.ctx);
                zr_input_begin(&gui.ctx);
                zr_input_motion(&gui.ctx, INT_MAX, INT_MAX);
                break;

#else
                
            case 1: case 2: case 3: case 4:
                zr_input_motion(&gui.ctx, x, y);
                break;
            case 5:
                zr_input_button(&gui.ctx, ZR_BUTTON_LEFT, x, y, zr_true);
                break;
            case 6:
                zr_input_button(&gui.ctx, ZR_BUTTON_LEFT, x, y, zr_false);
                break;
            case 7:
                zr_input_button(&gui.ctx, ZR_BUTTON_RIGHT, x, y, zr_true);
                break;
            case 8:
                zr_input_button(&gui.ctx, ZR_BUTTON_RIGHT, x, y, zr_false);
                break;
            case 9:
                zr_input_button(&gui.ctx, ZR_BUTTON_MIDDLE, x, y, zr_true);
                break;
            case 10:
                zr_input_button(&gui.ctx, ZR_BUTTON_MIDDLE, x, y, zr_false);
                break;
            case 11:
                zr_input_scroll(&gui.ctx, -[event[@"deltaY"] floatValue]);
                break;

#if !TARGET_OS_IPHONE

            case 12:
            case 13:
            {
                int down = type == 12 ? zr_true : zr_false;
                NSString* str = event[@"txt"];
                NSData* data = [str dataUsingEncoding: NSUTF32LittleEndianStringEncoding];
                const uint32_t* c = data.bytes;
                NSInteger n = data.length / sizeof(uint32_t);
                for (NSInteger i = 0; i < n; i += 1, c += 1)
                {
                    if (*c == 127 || *c < ' ' || *c >= NSUpArrowFunctionKey)
                    {
                        switch (*c)
                        {
                            case NSLeftArrowFunctionKey:
                                zr_input_key(&gui.ctx, ZR_KEY_LEFT, down);
                                break;
                            case NSRightArrowFunctionKey:
                                zr_input_key(&gui.ctx, ZR_KEY_RIGHT, down);
                                break;
                            case 3:
                                zr_input_key(&gui.ctx, ZR_KEY_COPY, down);
                                break;
                            case 22:
                                zr_input_key(&gui.ctx, ZR_KEY_PASTE, down);
                                break;
                            case 24:
                                zr_input_key(&gui.ctx, ZR_KEY_CUT, down);
                                break;
                            case 9:
                                zr_input_key(&gui.ctx, ZR_KEY_TAB, down);
                                break;
                            case 13:
                                zr_input_key(&gui.ctx, ZR_KEY_ENTER, down);
                                break;
                            case 127:
                                zr_input_key(&gui.ctx, ZR_KEY_BACKSPACE, down);
                                break;
                            default:
                                break;
                        }
                    }
                    else if (down)
                    {
                        zr_input_unicode(&gui.ctx, *c);
                    }
                }
            }
            break;
#endif
#endif
                
            default:
                break;
        }
    }
    
    zr_input_end(&gui.ctx);
}


- (void) addEvent: (NSDictionary*) event
{
    [drawCommands removeAllObjects];
    [events addObject: event];
}


@end
