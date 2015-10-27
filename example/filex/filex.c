/*
    Copyright (c) 2015 Micha Mettke

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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#ifdef _WIN32
#error "windows is not supported"
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#define NANOVG_GLES2_IMPLEMENTATION
#include "../../demo/nanovg/dep/nanovg.h"
#include "../../demo/nanovg/dep/nanovg_gl.h"
#include "../../demo/nanovg/dep/nanovg_gl_utils.h"
#include "../../demo/nanovg/dep/nanovg.c"

/* macros */
#define MAX_BUFFER  64
#define MAX_MEMORY  (16 * 1024)
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../../zahnrad.h"

#define MAX_PATH_LEN 512
#define MAX_WINDOWS 32
#define MAX_COMMAND_MEMORY (64 * 1024)

/* =================================================================
 *
 *                          UTIL
 *
 * ================================================================= */
static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static size_t
file_size(const char *path)
{
    int result;
    struct stat statbuf;
    result = stat(path, &statbuf);
    if (result != 0)
        die("[Platform]: failed to find file:  %s\n", path);
    return (size_t)statbuf.st_size;
}

static size_t
file_load(const char *path, void *memory, size_t size)
{
    int fd;
    ssize_t res;
    assert(path && size);
    fd = open(path, O_RDONLY);
    if (fd == -1)
        die("Failed to open file: %s (%s)\n", path, strerror(errno));
    res = read(fd, memory, size);
    if (res < 0) die("Failed to call read: %s",strerror(errno));
    close(fd);
    return (size_t)(res);
}

static void
dir_free_list(char **list, size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i)
        free(list[i]);
    free(list);
}

static char**
dir_list(const char *dir, int return_subdirs, size_t *count)
{
    size_t n = 0;
    char buffer[MAX_PATH_LEN];
    char **results = NULL;
    const DIR *none = NULL;
    size_t capacity = 32;
    size_t size;
    DIR *z;

    assert(dir);
    assert(count);
    strncpy(buffer, dir, MAX_PATH_LEN);
    n = strlen(buffer);

    if (n > 0 && (buffer[n-1] != '/')) {
        buffer[n++] = '/';
    }

    size = 0;

    z = opendir(dir);
    if (z != none) {
        int nonempty = 1;
        struct dirent *data = readdir(z);
        nonempty = (data != NULL);
        if (!nonempty) return NULL;

        do {
            DIR *y;
            char *p;
            int is_subdir;
            if (data->d_name[0] == '.')
                continue;

            strncpy(buffer + n, data->d_name, MAX_PATH_LEN-n);
            y = opendir(buffer);
            is_subdir = (y != NULL);
            if (y != NULL) closedir(y);

            if ((return_subdirs && is_subdir) || (!is_subdir && !return_subdirs)){
                if (!size) {
                    results = calloc(sizeof(char*), capacity);
                } else if (size >= capacity) {
                    capacity = capacity * 2;
                    results = realloc(results, capacity * sizeof(char*));
                }
                p = strdup(data->d_name);
                results[size++] = p;
            }
        } while ((data = readdir(z)) != NULL);
    }

    if (z) closedir(z);
    *count = size;
    return results;
}

static void*
xcalloc(size_t siz, size_t n)
{
    void *ptr = calloc(siz, n);
    if (!ptr) die("Out of memory\n");
    return ptr;
}
/* =================================================================
 *
 *                          MEDIA
 *
 * ================================================================= */
struct icon {
    int image;
    struct zr_image img;
};

struct icons {
    struct icon desktop;
    struct icon home;
    struct icon computer;
    struct icon directory;
    /* file icons */
    struct icon default_file;
    struct icon text_file;
    struct icon music_file;
    struct icon font_file;
    struct icon img_file;
    struct icon movie_file;
};

enum file_groups {
    FILE_GROUP_DEFAULT,
    FILE_GROUP_TEXT,
    FILE_GROUP_MUSIC,
    FILE_GROUP_FONT,
    FILE_GROUP_IMAGE,
    FILE_GROUP_MOVIE,
    FILE_GROUP_MAX
};

enum file_types {
    FILE_DEFAULT,
    FILE_TEXT,
    FILE_C_SOURCE,
    FILE_CPP_SOURCE,
    FILE_HEADER,
    FILE_MP3,
    FILE_WAV,
    FILE_OGG,
    FILE_TTF,
    FILE_BMP,
    FILE_PNG,
    FILE_JPEG,
    FILE_PCX,
    FILE_TGA,
    FILE_GIF,
    FILE_MAX
};

struct file_group {
    enum file_groups group;
    const char *name;
    struct icon *icon;
};

struct file {
    enum file_types type;
    const char *suffix;
    enum file_groups group;
};

struct media {
    int font;
    int icon_sheet;
    struct icons icons;
    struct file_group group[FILE_GROUP_MAX];
    struct file files[FILE_MAX];
};

static struct icon
ICON(NVGcontext *vg, const char *path)
{
    struct icon i;
    i.image = nvgCreateImage(vg, path, 0);
    i.img = zr_image_id(i.image);
    return i;
}

static struct file_group
FILE_GROUP(enum file_groups group, const char *name, struct icon *icon)
{
    struct file_group fg;
    fg.group = group;
    fg.name = name;
    fg.icon = icon;
    return fg;
}

static struct file
FILE_DEF(enum file_types type, const char *suffix, enum file_groups group)
{
    struct file fd;
    fd.type = type;
    fd.suffix = suffix;
    fd.group = group;
    return fd;
}

static struct icon*
media_icon_for_file(struct media *media, const char *file)
{
    int i = 0;
    const char *s = file;
    char suffix[4];
    int found = 0;
    memset(suffix, 0, sizeof(suffix));

    /* extract suffix .xxx from file */
    while (*s++ != '\0') {
        if (found && i < 3)
            suffix[i++] = *s;

        if (*s == '.') {
            if (found){
                found = 0;
                break;
            }
            found = 1;
        }
    }

    /* check for all file definition of all groups for fitting suffix*/
    for (i = 0; i < FILE_MAX && found; ++i) {
        struct file *d = &media->files[i];
        {
            const char *f = d->suffix;
            s = suffix;
            while (f && *f && *s && *s == *f) {
                s++; f++;
            }

            /* found correct file definition so */
            if (f && *s == '\0' && *f == '\0')
                return media->group[d->group].icon;
        }
    }
    return &media->icons.default_file;
}

static void
media_init(struct media *media, NVGcontext *vg)
{
    /* load media */
    struct icons *icons = &media->icons;

    /* icons */
    icons->home = ICON(vg, "../icons/home.png");
    icons->directory = ICON(vg, "../icons/directory.png");
    icons->computer = ICON(vg, "../icons/computer.png");
    icons->desktop = ICON(vg, "../icons/desktop.png");
    icons->default_file = ICON(vg, "../icons/default.png");
    icons->text_file = ICON(vg, "../icons/text.png");
    icons->music_file = ICON(vg, "../icons/music.png");
    icons->font_file =  ICON(vg, "../icons/font.png");
    icons->img_file = ICON(vg, "../icons/img.png");
    icons->movie_file = ICON(vg, "../icons/movie.png");

    /* file groups */
    media->group[FILE_GROUP_DEFAULT] = FILE_GROUP(FILE_GROUP_DEFAULT,"default",&icons->default_file);
    media->group[FILE_GROUP_TEXT] = FILE_GROUP(FILE_GROUP_TEXT, "textual", &icons->text_file);
    media->group[FILE_GROUP_MUSIC] = FILE_GROUP(FILE_GROUP_MUSIC, "music", &icons->music_file);
    media->group[FILE_GROUP_FONT] = FILE_GROUP(FILE_GROUP_FONT, "font", &icons->font_file);
    media->group[FILE_GROUP_IMAGE] = FILE_GROUP(FILE_GROUP_IMAGE, "image", &icons->img_file);
    media->group[FILE_GROUP_MOVIE] = FILE_GROUP(FILE_GROUP_MOVIE, "movie", &icons->movie_file);

    /* files */
    media->files[FILE_DEFAULT] = FILE_DEF(FILE_DEFAULT, NULL, FILE_GROUP_DEFAULT);
    media->files[FILE_TEXT] = FILE_DEF(FILE_TEXT, "txt", FILE_GROUP_TEXT);
    media->files[FILE_C_SOURCE] = FILE_DEF(FILE_C_SOURCE, "c", FILE_GROUP_TEXT);
    media->files[FILE_CPP_SOURCE] = FILE_DEF(FILE_CPP_SOURCE, "cpp", FILE_GROUP_TEXT);
    media->files[FILE_HEADER] = FILE_DEF(FILE_HEADER, "h", FILE_GROUP_TEXT);
    media->files[FILE_MP3] = FILE_DEF(FILE_MP3, "mp3", FILE_GROUP_MUSIC);
    media->files[FILE_WAV] = FILE_DEF(FILE_WAV, "wav", FILE_GROUP_MUSIC);
    media->files[FILE_OGG] = FILE_DEF(FILE_OGG, "ogg", FILE_GROUP_MUSIC);
    media->files[FILE_TTF] = FILE_DEF(FILE_TTF, "ttf", FILE_GROUP_FONT);
    media->files[FILE_BMP] = FILE_DEF(FILE_BMP, "bmp", FILE_GROUP_IMAGE);
    media->files[FILE_PNG] = FILE_DEF(FILE_PNG, "png", FILE_GROUP_IMAGE);
    media->files[FILE_JPEG] = FILE_DEF(FILE_JPEG, "jpg", FILE_GROUP_IMAGE);
    media->files[FILE_PCX] = FILE_DEF(FILE_PCX, "pcx", FILE_GROUP_IMAGE);
    media->files[FILE_TGA] = FILE_DEF(FILE_TGA, "tga", FILE_GROUP_IMAGE);
    media->files[FILE_GIF] = FILE_DEF(FILE_GIF, "gif", FILE_GROUP_IMAGE);
}
/* =================================================================
 *
 *                          FILE EXPLORER
 *
 * ================================================================= */
struct file_browser {
    /* path */
    char file[MAX_PATH_LEN];
    char home[MAX_PATH_LEN];
    char desktop[MAX_PATH_LEN];
    char directory[MAX_PATH_LEN];

    /* directory content */
    char **files;
    char **directories;
    size_t file_count;
    size_t dir_count;

    /* gui  */
    void *memory;
    struct media media;
    struct zr_input input;
    struct zr_command_queue queue;
    struct zr_style config;
    struct zr_user_font font;
    struct zr_window window;
    struct zr_vec2 dir;
    struct zr_vec2 sel;
    zr_float ratio_dir;
    zr_float ratio_sel;
};

static void
file_browser_reload_directory_content(struct file_browser *browser, const char *path)
{
    strncpy(browser->directory, path, MAX_PATH_LEN);
    dir_free_list(browser->files, browser->file_count);
    dir_free_list(browser->directories, browser->dir_count);
    browser->files = dir_list(path, 0, &browser->file_count);
    browser->directories = dir_list(path, 1, &browser->dir_count);
}

static void
file_browser_init(struct file_browser *browser, NVGcontext *vg,
    struct zr_user_font *font, int width, int height)
{
    memset(browser, 0, sizeof(*browser));
    media_init(&browser->media, vg);
    {
        /* gui */
        browser->font = *font;
        browser->memory = calloc(1, MAX_COMMAND_MEMORY);
        memset(&browser->input, 0, sizeof(browser->input));
        zr_command_queue_init_fixed(&browser->queue, browser->memory, MAX_COMMAND_MEMORY);
        zr_style_default(&browser->config, ZR_DEFAULT_ALL, &browser->font);
        zr_window_init(&browser->window, zr_rect(0,0,width,height), 0, &browser->queue, &browser->config, &browser->input);
        browser->ratio_dir = 0.75; browser->ratio_sel = 0.25f;
    }
    {
        /* load files and sub-directory list */
        const char *home = getenv("HOME");
        if (!home) home = getpwuid(getuid())->pw_dir;
        {
            size_t l;
            strncpy(browser->home, home, MAX_PATH_LEN);
            l = strlen(browser->home);
            strcpy(browser->home + l, "/");
            strcpy(browser->directory, browser->home);
        }
        {
            size_t l;
            strcpy(browser->desktop, browser->home);
            l = strlen(browser->desktop);
            strcpy(browser->desktop + l, "desktop/");
        }
        browser->files = dir_list(browser->directory, 0, &browser->file_count);
        browser->directories = dir_list(browser->directory, 1, &browser->dir_count);
    }
}

static void
file_browser_free(struct file_browser *browser)
{
    if (browser->files)
        dir_free_list(browser->files, browser->file_count);
    if (browser->directories)
        dir_free_list(browser->directories, browser->dir_count);
    browser->files = NULL;
    browser->directories = NULL;
}

static int
file_browser_run(struct file_browser *browser, int width, int height)
{
    struct zr_context context;
    struct media *media = &browser->media;
    struct icons *icons = &media->icons;
    struct zr_rect total_space;

    browser->window.bounds.w = width;
    browser->window.bounds.h = height;
    zr_begin(&context, &browser->window);
    {
        struct zr_context sub;
        zr_float row_layout[3];
        /* output path directory selector in the menubar */
        zr_menubar_begin(&context);
        {
            char *d = browser->directory;
            char *begin = d + 1;
            zr_layout_row_dynamic(&context, 25, 6);
            zr_style_push_property(&browser->config, ZR_PROPERTY_ITEM_SPACING, zr_vec2(0, 4));
            while (*d++) {
                if (*d == '/') {
                    *d = '\0';
                    if (zr_button_text(&context, begin, ZR_BUTTON_DEFAULT)) {
                        *d++ = '/'; *d = '\0';
                        file_browser_reload_directory_content(browser, browser->directory);
                        break;
                    }
                    *d = '/';
                    begin = d + 1;
                }
            }
            zr_style_pop_property(&browser->config);
        }
        zr_menubar_end(&context);

        /* window layout */
        total_space = zr_space(&context);
        row_layout[0] = (total_space.w - 8) * browser->ratio_sel;
        row_layout[1] = 8;
        row_layout[2] = (total_space.w - 8) * browser->ratio_dir;
        zr_layout_row(&context, ZR_STATIC, total_space.h, 3, row_layout);
        zr_style_push_property(&browser->config, ZR_PROPERTY_ITEM_SPACING, zr_vec2(0, 4));

        /* output special important directory list in own window */
        zr_group_begin(&context, &sub, NULL, ZR_WINDOW_NO_SCROLLBAR, browser->sel);
        {
            struct zr_image home = icons->home.img;
            struct zr_image desktop = icons->desktop.img;
            struct zr_image computer = icons->computer.img;

            zr_layout_row_dynamic(&sub, 40, 1);
            zr_style_push_property(&browser->config, ZR_PROPERTY_ITEM_SPACING, zr_vec2(0, 0));
            if (zr_button_text_image(&sub, home, "home", ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
                file_browser_reload_directory_content(browser, browser->home);
            if (zr_button_text_image(&sub,desktop,"desktop",ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
                file_browser_reload_directory_content(browser, browser->desktop);
            if (zr_button_text_image(&sub,computer,"computer",ZR_TEXT_CENTERED,ZR_BUTTON_DEFAULT))
                file_browser_reload_directory_content(browser, "/");
            zr_style_pop_property(&browser->config);
        }
        zr_group_end(&context, &sub, &browser->sel);

        {
            /* scaler */
            struct zr_rect bounds;
            struct zr_input *in = &browser->input;
            zr_layout_peek(&bounds, &context);
            zr_spacing(&context, 1);
            if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
            {
                zr_float sel = row_layout[0] + in->mouse.delta.x;
                zr_float dir = row_layout[2] - in->mouse.delta.x;
                browser->ratio_sel = sel / (total_space.w - 8);
                browser->ratio_dir = dir / (total_space.w - 8);
            }
        }

        /* output directory content window */
        zr_group_begin(&context, &sub, NULL, 0, browser->dir);
        {
            int index = -1;
            size_t i = 0, j = 0, k = 0;
            size_t rows = 0, cols = 0;
            size_t count = browser->dir_count + browser->file_count;

            cols = 4;
            rows = count / cols;
            for (i = 0; i <= rows; i += 1) {
                {
                    /* draw one row of icons */
                    size_t n = j + cols;
                    zr_layout_row_dynamic(&sub, 130, cols);
                    zr_style_push_color(&browser->config, ZR_COLOR_BUTTON, zr_rgb(45, 45, 45));
                    zr_style_push_color(&browser->config, ZR_COLOR_BORDER, zr_rgb(45, 45, 45));
                    for (; j < count && j < n; ++j) {
                        if (j < browser->dir_count) {
                            /* draw and execute directory buttons */
                            if (zr_button_image(&sub,icons->directory.img,ZR_BUTTON_DEFAULT))
                                index = (int)j;
                        } else {
                            /* draw and execute files buttons */
                            struct icon *icon;
                            size_t fileIndex = ((size_t)j - browser->dir_count);
                            icon = media_icon_for_file(media,browser->files[fileIndex]);
                            if (zr_button_image(&sub, icon->img, ZR_BUTTON_DEFAULT)) {
                                strncpy(browser->file, browser->directory, MAX_PATH_LEN);
                                n = strlen(browser->file);
                                strncpy(browser->file + n, browser->files[fileIndex], MAX_PATH_LEN - n);
                                return 0;
                            }
                        }
                    }
                    zr_style_pop_color(&browser->config);
                    zr_style_pop_color(&browser->config);
                }
                {
                    /* draw one row of labels */
                    zr_style_push_property(&browser->config, ZR_PROPERTY_ITEM_SPACING, zr_vec2(4, 4));
                    size_t n = k + cols;
                    zr_layout_row_dynamic(&sub, 20, cols);
                    for (; k < count && k < n; k++) {
                        if (k < browser->dir_count) {
                            zr_label(&sub, browser->directories[k], ZR_TEXT_CENTERED);
                        } else {
                            size_t t = k-browser->dir_count;
                            zr_label(&sub,browser->files[t],ZR_TEXT_CENTERED);
                        }
                    }
                    zr_style_pop_property(&browser->config);
                }
            }

            if (index != -1) {
                size_t n = strlen(browser->directory);
                strncpy(browser->directory + n, browser->directories[index], MAX_PATH_LEN - n);
                n = strlen(browser->directory);
                if (n < MAX_PATH_LEN - 1) {
                    browser->directory[n] = '/';
                    browser->directory[n+1] = '\0';
                }
                file_browser_reload_directory_content(browser, browser->directory);
            }
        }
        zr_group_end(&context, &sub, &browser->dir);
        zr_style_pop_property(&browser->config);
    }
    zr_end(&context, &browser->window);
    return 1;
}
/* =================================================================
 *
 *                          APP
 *
 * ================================================================= */
static zr_size
font_get_width(zr_handle handle, const zr_char *text, zr_size len)
{
    zr_size width;
    float bounds[4];
    NVGcontext *ctx = (NVGcontext*)handle.ptr;
    nvgTextBounds(ctx, 0, 0, text, &text[len], bounds);
    width = (zr_size)(bounds[2] - bounds[0]);
    return width;
}

static void
draw(NVGcontext *nvg, struct zr_command_queue *queue, int width, int height)
{
    const struct zr_command *cmd;
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);

    nvgBeginFrame(nvg, width, height, ((float)width/(float)height));
    zr_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case ZR_COMMAND_NOP: break;
        case ZR_COMMAND_SCISSOR: {
            const struct zr_command_scissor *s = zr_command(scissor, cmd);
            nvgScissor(nvg, s->x, s->y, s->w, s->h);
        } break;
        case ZR_COMMAND_LINE: {
            const struct zr_command_line *l = zr_command(line, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, l->begin.x, l->begin.y);
            nvgLineTo(nvg, l->end.x, l->end.y);
            nvgFillColor(nvg, nvgRGBA(l->color.r, l->color.g, l->color.b, l->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_CURVE: {
            const struct zr_command_curve *q = zr_command(curve, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, q->begin.x, q->begin.y);
            nvgBezierTo(nvg, q->ctrl[0].x, q->ctrl[0].y, q->ctrl[1].x,
                q->ctrl[1].y, q->end.x, q->end.y);
            nvgStrokeColor(nvg, nvgRGBA(q->color.r, q->color.g, q->color.b, q->color.a));
            nvgStrokeWidth(nvg, 3);
            nvgStroke(nvg);
        } break;
        case ZR_COMMAND_RECT: {
            const struct zr_command_rect *r = zr_command(rect, cmd);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, r->x, r->y, r->w, r->h, r->rounding);
            nvgFillColor(nvg, nvgRGBA(r->color.r, r->color.g, r->color.b, r->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_CIRCLE: {
            const struct zr_command_circle *c = zr_command(circle, cmd);
            nvgBeginPath(nvg);
            nvgCircle(nvg, c->x + (c->w/2.0f), c->y + c->w/2.0f, c->w/2.0f);
            nvgFillColor(nvg, nvgRGBA(c->color.r, c->color.g, c->color.b, c->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_TRIANGLE: {
            const struct zr_command_triangle *t = zr_command(triangle, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, t->a.x, t->a.y);
            nvgLineTo(nvg, t->b.x, t->b.y);
            nvgLineTo(nvg, t->c.x, t->c.y);
            nvgLineTo(nvg, t->a.x, t->a.y);
            nvgFillColor(nvg, nvgRGBA(t->color.r, t->color.g, t->color.b, t->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_TEXT: {
            const struct zr_command_text *t = zr_command(text, cmd);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, t->x, t->y, t->w, t->h, 0);
            nvgFillColor(nvg, nvgRGBA(t->background.r, t->background.g,
                t->background.b, t->background.a));
            nvgFill(nvg);

            nvgBeginPath(nvg);
            nvgFillColor(nvg, nvgRGBA(t->foreground.r, t->foreground.g,
                t->foreground.b, t->foreground.a));
            nvgTextAlign(nvg, NVG_ALIGN_MIDDLE);
            nvgText(nvg, t->x, t->y + t->h * 0.5f, t->string, &t->string[t->length]);
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_IMAGE: {
            int w, h;
            NVGpaint imgpaint;
            const struct zr_command_image *i = zr_command(image, cmd);
            nvgImageSize(nvg, i->img.handle.id, &w, &h);
            nvgBeginPath(nvg);
            nvgRect(nvg, i->x, i->y, i->w, i->h);
            nvgFillPaint(nvg, nvgImagePattern(nvg,
                i->x, i->y, i->w, i->h, 0, i->img.handle.id, 1));
            nvgFill(nvg);

        } break;
        case ZR_COMMAND_ARC:
        default: break;
        }
    }
    zr_command_queue_clear(queue);

    nvgResetScissor(nvg);
    nvgEndFrame(nvg);
    glPopAttrib();
}

static void
key(struct zr_input *in, SDL_Event *evt, zr_bool down)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        zr_input_key(in, ZR_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        zr_input_key(in, ZR_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        zr_input_key(in, ZR_KEY_ENTER, down);
    else if (sym == SDLK_BACKSPACE)
        zr_input_key(in, ZR_KEY_BACKSPACE, down);
    else if (sym == SDLK_LEFT)
        zr_input_key(in, ZR_KEY_LEFT, down);
    else if (sym == SDLK_RIGHT)
        zr_input_key(in, ZR_KEY_RIGHT, down);
    else if (sym == SDLK_c)
        zr_input_key(in, ZR_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_v)
        zr_input_key(in, ZR_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_x)
        zr_input_key(in, ZR_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
}

static void
motion(struct zr_input *in, SDL_Event *evt)
{
    const zr_int x = evt->motion.x;
    const zr_int y = evt->motion.y;
    zr_input_motion(in, x, y);
}

static void
btn(struct zr_input *in, SDL_Event *evt, zr_bool down)
{
    const zr_int x = evt->button.x;
    const zr_int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        zr_input_button(in, ZR_BUTTON_LEFT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_RIGHT)
        zr_input_button(in, ZR_BUTTON_RIGHT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_MIDDLE)
        zr_input_button(in, ZR_BUTTON_MIDDLE, x, y, down);
}

static void
text(struct zr_input *in, SDL_Event *evt)
{
    zr_glyph glyph;
    memcpy(glyph, evt->text.text, ZR_UTF_SIZE);
    zr_input_glyph(in, glyph);
}

static void
resize(SDL_Event *evt)
{
    if (evt->window.event != SDL_WINDOWEVENT_RESIZED) return;
    glViewport(0, 0, evt->window.data1, evt->window.data2);
}

int
main(int argc, char *argv[])
{
    int x,y,width, height;
    SDL_Window *win;
    SDL_GLContext glContext;
    NVGcontext *vg = NULL;

    int running = 1;
    unsigned int started;
    unsigned int dt;
    struct zr_user_font font;
    struct file_browser browser;
    const char *font_path;
    int icon_sheet;

    font_path = argv[1];
    if (argc < 2)
        die("missing argument!: <font> <icons>");

    /* SDL */
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("File Explorer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &width, &height);
    SDL_GetWindowPosition(win, &x, &y);

    /* OpenGL */
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        die("[GLEW] failed setup\n");
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    /* nanovg */
    vg = nvgCreateGLES2(NVG_ANTIALIAS|NVG_DEBUG);
    if (!vg) die("[NVG]: failed to init\n");
    nvgCreateFont(vg, "fixed", font_path);
    nvgFontFace(vg, "fixed");
    nvgFontSize(vg, 10);
    nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);

    /* GUI */
    memset(&browser, 0, sizeof browser);
    font.userdata.ptr = vg;
    nvgTextMetrics(vg, NULL, NULL, &font.height);
    font.width = font_get_width;
    file_browser_init(&browser, vg, &font, width, height);

    while (running) {
        /* Input */
        SDL_Event evt;
        started = SDL_GetTicks();
        zr_input_begin(&browser.input);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_WINDOWEVENT) resize(&evt);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP) key(&browser.input, &evt, zr_false);
            else if (evt.type == SDL_KEYDOWN) key(&browser.input, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN) btn(&browser.input, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONUP) btn(&browser.input, &evt, zr_false);
            else if (evt.type == SDL_MOUSEMOTION) motion(&browser.input, &evt);
            else if (evt.type == SDL_TEXTINPUT) text(&browser.input, &evt);
            else if (evt.type == SDL_MOUSEWHEEL) zr_input_scroll(&browser.input, evt.wheel.y);
        }
        zr_input_end(&browser.input);

        SDL_GetWindowSize(win, &width, &height);
        running = file_browser_run(&browser, width, height);

        /* Draw */
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        draw(vg, &browser.queue, width, height);
        SDL_GL_SwapWindow(win);
    }

cleanup:
    /* Cleanup */
    free(browser.memory);
    nvgDeleteGLES2(vg);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

