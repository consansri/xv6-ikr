
#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/fcntl.h"
#include "xv6-user/user.h"

#include <math.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define WIDTH 300
#define HEIGHT 220
#define BACKGROUND_COLOR 0xFF181818

#define PI 3.14159265359

// Copyright 2022 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.




#define OLIVEC_AA_RES 2

#define OLIVEC_SWAP(T, a, b) do { T t = a; a = b; b = t; } while (0)
#define OLIVEC_SIGN(T, x) ((T)((x) > 0) - (T)((x) < 0))
#define OLIVEC_ABS(T, x) (OLIVEC_SIGN(T, x)*(x))

typedef struct {
    size_t width, height;
    const char *glyphs;
} Olivec_Font;

#define OLIVEC_DEFAULT_FONT_HEIGHT 6
#define OLIVEC_DEFAULT_FONT_WIDTH 6

// TODO: allocate proper descender and acender areas for the default font
static char olivec_default_glyphs[128][OLIVEC_DEFAULT_FONT_HEIGHT][OLIVEC_DEFAULT_FONT_WIDTH] = {
    ['a'] = {
        {0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0},
        {0, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
    },
    ['b'] = {
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 0, 0},
    },
    ['c'] = {
        {0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['d'] = {
        {0, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
    },
    ['e'] = {
        {0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 1, 0},
        {1, 0, 0, 0, 0},
        {0, 1, 1, 1, 0},
    },
    ['f'] = {
        {0, 0, 1, 1, 0},
        {0, 1, 0, 0, 0},
        {1, 1, 1, 1, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
    },
    ['g'] = {
        {0, 1, 1, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['h'] = {
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
    },
    ['i'] = {
        {0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
    },
    ['j'] = {
        {0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {1, 0, 1, 0, 0},
        {0, 1, 1, 0, 0},
    },
    ['k'] = {
        {1, 0, 0, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 1, 0, 0},
        {1, 1, 0, 0, 0},
        {1, 0, 1, 0, 0},
        {1, 0, 0, 1, 0},
    },
    ['l'] = {
        {0, 1, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 1, 1, 0},
    },
    ['m'] = {
        {0, 0, 0, 0, 0},
        {0, 1, 0, 1, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
    },
    ['n'] = {
        {0, 0, 0, 0, 0},
        {0, 1, 1, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
    },
    ['o'] = {
        {0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['p'] = {
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
    },
    ['q'] = {
        {0, 1, 1, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 0, 0, 1, 0},
    },
    ['r'] = {
        {0, 0, 0, 0, 0},
        {1, 0, 1, 1, 0},
        {1, 1, 0, 0, 1},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
    },
    ['s'] = {
        {0, 0, 0, 0, 0},
        {0, 1, 1, 1, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {1, 1, 1, 0, 0},
    },
    ['t'] = {
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {1, 1, 1, 1, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['u'] = {
        {0, 0, 0, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
    },
    ['v'] = {
        {0, 0, 0, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['w'] = {
        {0, 0, 0, 0, 0},
        {1, 0, 0, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
        {0, 1, 1, 1, 1},
    },
    ['x'] = {
        {0, 0, 0, 0, 0},
        {1, 0, 1, 0, 0},
        {1, 0, 1, 0, 0},
        {0, 1, 0, 0, 0},
        {1, 0, 1, 0, 0},
        {1, 0, 1, 0, 0},
    },
    ['y'] = {
        {0, 0, 0, 0, 0},
        {1, 0, 1, 0, 0},
        {1, 0, 1, 0, 0},
        {1, 0, 1, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
    },
    ['z'] = {
        {0, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
    },

    ['A'] = {0},
    ['B'] = {0},
    ['C'] = {0},
    ['D'] = {0},
    ['E'] = {0},
    ['F'] = {0},
    ['G'] = {0},
    ['H'] = {0},
    ['I'] = {0},
    ['J'] = {0},
    ['K'] = {0},
    ['L'] = {0},
    ['M'] = {0},
    ['N'] = {0},
    ['O'] = {0},
    ['P'] = {0},
    ['Q'] = {0},
    ['R'] = {0},
    ['S'] = {0},
    ['T'] = {0},
    ['U'] = {0},
    ['V'] = {0},
    ['W'] = {0},
    ['X'] = {0},
    ['Y'] = {0},
    ['Z'] = {0},

    ['0'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['1'] = {
        {0, 0, 1, 0, 0},
        {0, 1, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 1, 1, 0},
    },
    ['2'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
    },
    ['3'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['4'] = {
        {0, 0, 1, 1, 0},
        {0, 1, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 1, 0},
        {0, 0, 0, 1, 0},
    },
    ['5'] = {
        {1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {0, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['6'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['7'] = {
        {1, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
    },
    ['8'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},

    },
    ['9'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },

    [','] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 1, 0},
        {0, 0, 1, 0, 0},
    },

    ['.'] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
    },
    ['-'] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
    },
};

static Olivec_Font olivec_default_font = {
    .glyphs = &olivec_default_glyphs[0][0][0],
    .width = OLIVEC_DEFAULT_FONT_WIDTH,
    .height = OLIVEC_DEFAULT_FONT_HEIGHT,
};



float _pow(float a, float b) {
    float c = 1;
    for (int i=0; i<b; i++)
        c *= a;
    return c;
}

float _fact(float x) {
    float ret = 1;
    for (int i=1; i<=x; i++) 
        ret *= i;
    return ret;
}

float _sin(float x) {
    float y = x;
    float s = -1;
    for (int i=3; i<=100; i+=2) {
        y+=s*(_pow(x,i)/_fact(i));
        s *= -1;
    }  
    return y;
}
float _cos(float x) {
    float y = 1;
    float s = -1;
    for (int i=2; i<=100; i+=2) {
        y+=s*(_pow(x,i)/_fact(i));
        s *= -1;
    }  
    return y;
}
float _tan(float x) {
     return (_sin(x)/_cos(x));  
}


float _sqrt(float x)
{
    float y;
    int p,square,c;

    /* find the surrounding perfect squares */
    p = 0;
    do
    {
        p++;
        square = (p+1) * (p+1);
    }
    while( x > square );

    /* process the root */
    y = (float)p;
    c = 0;
    while(c<10)
    {
        /* divide and average */
        y = (x/y + y)/2;
        /* test for success */
        if( y*y == x)
            return(y);
        c++;
    }
    return(y);
}


float atanf(float x) {	
	return asinf(x / sqrtf(x * x + 1));
}

float atan2f(float y, float x) {
	
	if (y == 0.0) {
		if (x >= 0.0) {
			return 0.0;
		}
		else {
			return M_PI;
		}
	}
	else if (y > 0.0) {
		if (x == 0.0) {
			return M_PI_2;
		}
		else if (x > 0.0) {
			return atanf(y / x);
		}
		else {
			return M_PI - atanf(y / x);
		}
	}
	else {
		if (x == 0.0) {
			return M_PI + M_PI_2;
		}
		else if (x > 0.0) {
			return 2 * M_PI - atanf(y / x);
		}
		else {
			return M_PI + atanf(y / x);
		}
	}
}
// WARNING! Always initialize your Canvas with a color that has Non-Zero Alpha Channel!
// A lot of functions use `olivec_blend_color()` function to blend with the Background
// which preserves the original Alpha of the Background. So you may easily end up with
// a result that is perceptually transparent if the Alpha is Zero.
typedef struct {
    uint32_t *pixels;
    size_t width;
    size_t height;
    size_t stride;
} Olivec_Canvas;

#define OLIVEC_CANVAS_NULL ((Olivec_Canvas) {0})
#define OLIVEC_PIXEL(oc, x, y) (oc).pixels[(y)*(oc).stride + (x)]

 Olivec_Canvas olivec_canvas(uint32_t *pixels, size_t width, size_t height, size_t stride);
 Olivec_Canvas olivec_subcanvas(Olivec_Canvas oc, int x, int y, int w, int h);
 bool olivec_in_bounds(Olivec_Canvas oc, int x, int y);
 void olivec_blend_color(uint32_t *c1, uint32_t c2);
 void olivec_fill(Olivec_Canvas oc, uint32_t color);
 void olivec_rect(Olivec_Canvas oc, int x, int y, int w, int h, uint32_t color);
 void olivec_frame(Olivec_Canvas oc, int x, int y, int w, int h, size_t thiccness, uint32_t color);
 void olivec_circle(Olivec_Canvas oc, int cx, int cy, int r, uint32_t color);
 void olivec_ellipse(Olivec_Canvas oc, int cx, int cy, int rx, int ry, uint32_t color);
// TODO: lines with different thiccness
 void olivec_line(Olivec_Canvas oc, int x1, int y1, int x2, int y2, uint32_t color);
 bool olivec_normalize_triangle(size_t width, size_t height, int x1, int y1, int x2, int y2, int x3, int y3, int *lx, int *hx, int *ly, int *hy);
 bool olivec_barycentric(int x1, int y1, int x2, int y2, int x3, int y3, int xp, int yp, int *u1, int *u2, int *det);
 void olivec_triangle(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);
 void olivec_triangle3c(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t c1, uint32_t c2, uint32_t c3);
 void olivec_triangle3z(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, float z1, float z2, float z3);
 void olivec_triangle3uv(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, float tx1, float ty1, float tx2, float ty2, float tx3, float ty3, float z1, float z2, float z3, Olivec_Canvas texture);
 void olivec_triangle3uv_bilinear(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, float tx1, float ty1, float tx2, float ty2, float tx3, float ty3, float z1, float z2, float z3, Olivec_Canvas texture);
 void olivec_text(Olivec_Canvas oc, const char *text, int x, int y, Olivec_Font font, size_t size, uint32_t color);
 void olivec_sprite_blend(Olivec_Canvas oc, int x, int y, int w, int h, Olivec_Canvas sprite);
 void olivec_sprite_copy(Olivec_Canvas oc, int x, int y, int w, int h, Olivec_Canvas sprite);
 void olivec_sprite_copy_bilinear(Olivec_Canvas oc, int x, int y, int w, int h, Olivec_Canvas sprite);
 uint32_t olivec_pixel_bilinear(Olivec_Canvas sprite, int nx, int ny, int w, int h);

typedef struct {
    // Safe ranges to iterate over.
    int x1, x2;
    int y1, y2;

    // Original uncut ranges some parts of which may be outside of the canvas boundaries.
    int ox1, ox2;
    int oy1, oy2;
} Olivec_Normalized_Rect;

// The point of this function is to produce two ranges x1..x2 and y1..y2 that are guaranteed to be safe to iterate over the canvas of size pixels_width by pixels_height without any boundary checks.
//
// Olivec_Normalized_Rect nr = {0};
// if (olivec_normalize_rect(x, y, w, h, WIDTH, HEIGHT, &nr)) {
//     for (int x = nr.x1; x <= nr.x2; ++x) {
//         for (int y = nr.y1; y <= nr.y2; ++y) {
//             OLIVEC_PIXEL(oc, x, y) = 0x69696969;
//         }
//     }
// } else {
//     // Rectangle is invisible cause it's completely out-of-bounds
// }
 bool olivec_normalize_rect(int x, int y, int w, int h,
                                     size_t canvas_width, size_t canvas_height,
                                     Olivec_Normalized_Rect *nr);


 Olivec_Canvas olivec_canvas(uint32_t *pixels, size_t width, size_t height, size_t stride)
{
    Olivec_Canvas oc = {
        .pixels = pixels,
        .width  = width,
        .height = height,
        .stride = stride,
    };
    return oc;
}

 bool olivec_normalize_rect(int x, int y, int w, int h,
                                     size_t canvas_width, size_t canvas_height,
                                     Olivec_Normalized_Rect *nr)
{
    // No need to render empty rectangle
    if (w == 0) return false;
    if (h == 0) return false;

    nr->ox1 = x;
    nr->oy1 = y;

    // Convert the rectangle to 2-points representation
    nr->ox2 = nr->ox1 + OLIVEC_SIGN(int, w)*(OLIVEC_ABS(int, w) - 1);
    if (nr->ox1 > nr->ox2) OLIVEC_SWAP(int, nr->ox1, nr->ox2);
    nr->oy2 = nr->oy1 + OLIVEC_SIGN(int, h)*(OLIVEC_ABS(int, h) - 1);
    if (nr->oy1 > nr->oy2) OLIVEC_SWAP(int, nr->oy1, nr->oy2);

    // Cull out invisible rectangle
    if (nr->ox1 >= (int) canvas_width) return false;
    if (nr->ox2 < 0) return false;
    if (nr->oy1 >= (int) canvas_height) return false;
    if (nr->oy2 < 0) return false;

    nr->x1 = nr->ox1;
    nr->y1 = nr->oy1;
    nr->x2 = nr->ox2;
    nr->y2 = nr->oy2;

    // Clamp the rectangle to the boundaries
    if (nr->x1 < 0) nr->x1 = 0;
    if (nr->x2 >= (int) canvas_width) nr->x2 = (int) canvas_width - 1;
    if (nr->y1 < 0) nr->y1 = 0;
    if (nr->y2 >= (int) canvas_height) nr->y2 = (int) canvas_height - 1;

    return true;
}

 Olivec_Canvas olivec_subcanvas(Olivec_Canvas oc, int x, int y, int w, int h)
{
    Olivec_Normalized_Rect nr = {0};
    if (!olivec_normalize_rect(x, y, w, h, oc.width, oc.height, &nr)) return OLIVEC_CANVAS_NULL;
    oc.pixels = &OLIVEC_PIXEL(oc, nr.x1, nr.y1);
    oc.width = nr.x2 - nr.x1 + 1;
    oc.height = nr.y2 - nr.y1 + 1;
    return oc;
}

// TODO: custom pixel formats
// Maybe we can store pixel format info in Olivec_Canvas
#define OLIVEC_RED(color)   (((color)&0x000000FF)>>(8*0))
#define OLIVEC_GREEN(color) (((color)&0x0000FF00)>>(8*1))
#define OLIVEC_BLUE(color)  (((color)&0x00FF0000)>>(8*2))
#define OLIVEC_ALPHA(color) (((color)&0xFF000000)>>(8*3))
#define OLIVEC_RGBA(r, g, b, a) ((((r)&0xFF)<<(8*0)) | (((g)&0xFF)<<(8*1)) | (((b)&0xFF)<<(8*2)) | (((a)&0xFF)<<(8*3)))

 void olivec_blend_color(uint32_t *c1, uint32_t c2)
{
    uint32_t r1 = OLIVEC_RED(*c1);
    uint32_t g1 = OLIVEC_GREEN(*c1);
    uint32_t b1 = OLIVEC_BLUE(*c1);
    uint32_t a1 = OLIVEC_ALPHA(*c1);

    uint32_t r2 = OLIVEC_RED(c2);
    uint32_t g2 = OLIVEC_GREEN(c2);
    uint32_t b2 = OLIVEC_BLUE(c2);
    uint32_t a2 = OLIVEC_ALPHA(c2);

    r1 = (r1*(255 - a2) + r2*a2)/255; if (r1 > 255) r1 = 255;
    g1 = (g1*(255 - a2) + g2*a2)/255; if (g1 > 255) g1 = 255;
    b1 = (b1*(255 - a2) + b2*a2)/255; if (b1 > 255) b1 = 255;

    *c1 = OLIVEC_RGBA(r1, g1, b1, a1);
}

 void olivec_fill(Olivec_Canvas oc, uint32_t color)
{
    for (size_t y = 0; y < oc.height; ++y) {
        for (size_t x = 0; x < oc.width; ++x) {
            OLIVEC_PIXEL(oc, x, y) = color;
        }
    }
}

 void olivec_rect(Olivec_Canvas oc, int x, int y, int w, int h, uint32_t color)
{
    Olivec_Normalized_Rect nr = {0};
    if (!olivec_normalize_rect(x, y, w, h, oc.width, oc.height, &nr)) return;
    for (int x = nr.x1; x <= nr.x2; ++x) {
        for (int y = nr.y1; y <= nr.y2; ++y) {
            olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), color);
        }
    }
}

 void olivec_frame(Olivec_Canvas oc, int x, int y, int w, int h, size_t t, uint32_t color)
{
    if (t == 0) return; // Nothing to render

    // Convert the rectangle to 2-points representation
    int x1 = x;
    int y1 = y;
    int x2 = x1 + OLIVEC_SIGN(int, w)*(OLIVEC_ABS(int, w) - 1);
    if (x1 > x2) OLIVEC_SWAP(int, x1, x2);
    int y2 = y1 + OLIVEC_SIGN(int, h)*(OLIVEC_ABS(int, h) - 1);
    if (y1 > y2) OLIVEC_SWAP(int, y1, y2);

    olivec_rect(oc, x1 - t/2, y1 - t/2, (x2 - x1 + 1) + t/2*2, t, color);  // Top
    olivec_rect(oc, x1 - t/2, y1 - t/2, t, (y2 - y1 + 1) + t/2*2, color);  // Left
    olivec_rect(oc, x1 - t/2, y2 + t/2, (x2 - x1 + 1) + t/2*2, -t, color); // Bottom
    olivec_rect(oc, x2 + t/2, y1 - t/2, -t, (y2 - y1 + 1) + t/2*2, color); // Right
}

 void olivec_ellipse(Olivec_Canvas oc, int cx, int cy, int rx, int ry, uint32_t color)
{
    Olivec_Normalized_Rect nr = {0};
    int rx1 = rx + OLIVEC_SIGN(int, rx);
    int ry1 = ry + OLIVEC_SIGN(int, ry);
    if (!olivec_normalize_rect(cx - rx1, cy - ry1, 2*rx1, 2*ry1, oc.width, oc.height, &nr)) return;

    for (int y = nr.y1; y <= nr.y2; ++y) {
        for (int x = nr.x1; x <= nr.x2; ++x) {
            float nx = (x + 0.5 - nr.x1)/(2.0f*rx1);
            float ny = (y + 0.5 - nr.y1)/(2.0f*ry1);
            float dx = nx - 0.5;
            float dy = ny - 0.5;
            if (dx*dx + dy*dy <= 0.5*0.5) {
                OLIVEC_PIXEL(oc, x, y) = color;
            }
        }
    }
}

 void olivec_circle(Olivec_Canvas oc, int cx, int cy, int r, uint32_t color)
{
    Olivec_Normalized_Rect nr = {0};
    int r1 = r + OLIVEC_SIGN(int, r);
    if (!olivec_normalize_rect(cx - r1, cy - r1, 2*r1, 2*r1, oc.width, oc.height, &nr)) return;

    for (int y = nr.y1; y <= nr.y2; ++y) {
        for (int x = nr.x1; x <= nr.x2; ++x) {
            int count = 0;
            for (int sox = 0; sox < OLIVEC_AA_RES; ++sox) {
                for (int soy = 0; soy < OLIVEC_AA_RES; ++soy) {
                    // TODO: switch to 64 bits to make the overflow less likely
                    // Also research the probability of overflow
                    int res1 = (OLIVEC_AA_RES + 1);
                    int dx = (x*res1*2 + 2 + sox*2 - res1*cx*2 - res1);
                    int dy = (y*res1*2 + 2 + soy*2 - res1*cy*2 - res1);
                    if (dx*dx + dy*dy <= res1*res1*r*r*2*2) count += 1;
                }
            }
            uint32_t alpha = ((color&0xFF000000)>>(3*8))*count/OLIVEC_AA_RES/OLIVEC_AA_RES;
            uint32_t updated_color = (color&0x00FFFFFF)|(alpha<<(3*8));
            olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), updated_color);
        }
    }
}

 bool olivec_in_bounds(Olivec_Canvas oc, int x, int y)
{
    return 0 <= x && x < (int) oc.width && 0 <= y && y < (int) oc.height;
}

// TODO: AA for line
 void olivec_line(Olivec_Canvas oc, int x1, int y1, int x2, int y2, uint32_t color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    // If both of the differences are 0 there will be a division by 0 below.
    if (dx == 0 && dy == 0) {
        if (olivec_in_bounds(oc, x1, y1)) {
            olivec_blend_color(&OLIVEC_PIXEL(oc, x1, y1), color);
        }
        return;
    }

    if (OLIVEC_ABS(int, dx) > OLIVEC_ABS(int, dy)) {
        if (x1 > x2) {
            OLIVEC_SWAP(int, x1, x2);
            OLIVEC_SWAP(int, y1, y2);
        }

        for (int x = x1; x <= x2; ++x) {
            int y = dy*(x - x1)/dx + y1;
            // TODO: move boundary checks out side of the loops in olivec_draw_line
            if (olivec_in_bounds(oc, x, y)) {
                olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), color);
            }
        }
    } else {
        if (y1 > y2) {
            OLIVEC_SWAP(int, x1, x2);
            OLIVEC_SWAP(int, y1, y2);
        }

        for (int y = y1; y <= y2; ++y) {
            int x = dx*(y - y1)/dy + x1;
            // TODO: move boundary checks out side of the loops in olivec_draw_line
            if (olivec_in_bounds(oc, x, y)) {
                olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), color);
            }
        }
    }
}

 uint32_t mix_colors2(uint32_t c1, uint32_t c2, int u1, int det)
{
    // TODO: estimate how much overflows are an issue in integer only environment
    int64_t r1 = OLIVEC_RED(c1);
    int64_t g1 = OLIVEC_GREEN(c1);
    int64_t b1 = OLIVEC_BLUE(c1);
    int64_t a1 = OLIVEC_ALPHA(c1);

    int64_t r2 = OLIVEC_RED(c2);
    int64_t g2 = OLIVEC_GREEN(c2);
    int64_t b2 = OLIVEC_BLUE(c2);
    int64_t a2 = OLIVEC_ALPHA(c2);

    if (det != 0) {
        int u2 = det - u1;
        int64_t r4 = (r1*u2 + r2*u1)/det;
        int64_t g4 = (g1*u2 + g2*u1)/det;
        int64_t b4 = (b1*u2 + b2*u1)/det;
        int64_t a4 = (a1*u2 + a2*u1)/det;

        return OLIVEC_RGBA(r4, g4, b4, a4);
    }

    return 0;
}

 uint32_t mix_colors3(uint32_t c1, uint32_t c2, uint32_t c3, int u1, int u2, int det)
{
    // TODO: estimate how much overflows are an issue in integer only environment
    int64_t r1 = OLIVEC_RED(c1);
    int64_t g1 = OLIVEC_GREEN(c1);
    int64_t b1 = OLIVEC_BLUE(c1);
    int64_t a1 = OLIVEC_ALPHA(c1);

    int64_t r2 = OLIVEC_RED(c2);
    int64_t g2 = OLIVEC_GREEN(c2);
    int64_t b2 = OLIVEC_BLUE(c2);
    int64_t a2 = OLIVEC_ALPHA(c2);

    int64_t r3 = OLIVEC_RED(c3);
    int64_t g3 = OLIVEC_GREEN(c3);
    int64_t b3 = OLIVEC_BLUE(c3);
    int64_t a3 = OLIVEC_ALPHA(c3);

    if (det != 0) {
        int u3 = det - u1 - u2;
        int64_t r4 = (r1*u1 + r2*u2 + r3*u3)/det;
        int64_t g4 = (g1*u1 + g2*u2 + g3*u3)/det;
        int64_t b4 = (b1*u1 + b2*u2 + b3*u3)/det;
        int64_t a4 = (a1*u1 + a2*u2 + a3*u3)/det;

        return OLIVEC_RGBA(r4, g4, b4, a4);
    }

    return 0;
}

// NOTE: we imply u3 = det - u1 - u2
 bool olivec_barycentric(int x1, int y1, int x2, int y2, int x3, int y3, int xp, int yp, int *u1, int *u2, int *det)
{
    *det = ((x1 - x3)*(y2 - y3) - (x2 - x3)*(y1 - y3));
    *u1  = ((y2 - y3)*(xp - x3) + (x3 - x2)*(yp - y3));
    *u2  = ((y3 - y1)*(xp - x3) + (x1 - x3)*(yp - y3));
    int u3 = *det - *u1 - *u2;
    return (
               (OLIVEC_SIGN(int, *u1) == OLIVEC_SIGN(int, *det) || *u1 == 0) &&
               (OLIVEC_SIGN(int, *u2) == OLIVEC_SIGN(int, *det) || *u2 == 0) &&
               (OLIVEC_SIGN(int, u3) == OLIVEC_SIGN(int, *det) || u3 == 0)
           );
}

 bool olivec_normalize_triangle(size_t width, size_t height, int x1, int y1, int x2, int y2, int x3, int y3, int *lx, int *hx, int *ly, int *hy)
{
    *lx = x1;
    *hx = x1;
    if (*lx > x2) *lx = x2;
    if (*lx > x3) *lx = x3;
    if (*hx < x2) *hx = x2;
    if (*hx < x3) *hx = x3;
    if (*lx < 0) *lx = 0;
    if ((size_t) *lx >= width) return false;;
    if (*hx < 0) return false;;
    if ((size_t) *hx >= width) *hx = width-1;

    *ly = y1;
    *hy = y1;
    if (*ly > y2) *ly = y2;
    if (*ly > y3) *ly = y3;
    if (*hy < y2) *hy = y2;
    if (*hy < y3) *hy = y3;
    if (*ly < 0) *ly = 0;
    if ((size_t) *ly >= height) return false;;
    if (*hy < 0) return false;;
    if ((size_t) *hy >= height) *hy = height-1;

    return true;
}

 void olivec_triangle3c(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3,
                                 uint32_t c1, uint32_t c2, uint32_t c3)
{
    int lx, hx, ly, hy;
    if (olivec_normalize_triangle(oc.width, oc.height, x1, y1, x2, y2, x3, y3, &lx, &hx, &ly, &hy)) {
        for (int y = ly; y <= hy; ++y) {
            for (int x = lx; x <= hx; ++x) {
                int u1, u2, det;
                if (olivec_barycentric(x1, y1, x2, y2, x3, y3, x, y, &u1, &u2, &det)) {
                    olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), mix_colors3(c1, c2, c3, u1, u2, det));
                }
            }
        }
    }
}

 void olivec_triangle3z(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, float z1, float z2, float z3)
{
    int lx, hx, ly, hy;
    if (olivec_normalize_triangle(oc.width, oc.height, x1, y1, x2, y2, x3, y3, &lx, &hx, &ly, &hy)) {
        for (int y = ly; y <= hy; ++y) {
            for (int x = lx; x <= hx; ++x) {
                int u1, u2, det;
                if (olivec_barycentric(x1, y1, x2, y2, x3, y3, x, y, &u1, &u2, &det)) {
                    float z = z1*u1/det + z2*u2/det + z3*(det - u1 - u2)/det;
                    OLIVEC_PIXEL(oc, x, y) = *(uint32_t*)&z;
                }
            }
        }
    }
}

 void olivec_triangle3uv(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, float tx1, float ty1, float tx2, float ty2, float tx3, float ty3, float z1, float z2, float z3, Olivec_Canvas texture)
{
    int lx, hx, ly, hy;
    if (olivec_normalize_triangle(oc.width, oc.height, x1, y1, x2, y2, x3, y3, &lx, &hx, &ly, &hy)) {
        for (int y = ly; y <= hy; ++y) {
            for (int x = lx; x <= hx; ++x) {
                int u1, u2, det;
                if (olivec_barycentric(x1, y1, x2, y2, x3, y3, x, y, &u1, &u2, &det)) {
                    int u3 = det - u1 - u2;
                    float z = z1*u1/det + z2*u2/det + z3*(det - u1 - u2)/det;
                    float tx = tx1*u1/det + tx2*u2/det + tx3*u3/det;
                    float ty = ty1*u1/det + ty2*u2/det + ty3*u3/det;

                    int texture_x = tx/z*texture.width;
                    if (texture_x < 0) texture_x = 0;
                    if ((size_t) texture_x >= texture.width) texture_x = texture.width - 1;

                    int texture_y = ty/z*texture.height;
                    if (texture_y < 0) texture_y = 0;
                    if ((size_t) texture_y >= texture.height) texture_y = texture.height - 1;
                    OLIVEC_PIXEL(oc, x, y) = OLIVEC_PIXEL(texture, (int)texture_x, (int)texture_y);
                }
            }
        }
    }
}

 void olivec_triangle3uv_bilinear(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, float tx1, float ty1, float tx2, float ty2, float tx3, float ty3, float z1, float z2, float z3, Olivec_Canvas texture)
{
    int lx, hx, ly, hy;
    if (olivec_normalize_triangle(oc.width, oc.height, x1, y1, x2, y2, x3, y3, &lx, &hx, &ly, &hy)) {
        for (int y = ly; y <= hy; ++y) {
            for (int x = lx; x <= hx; ++x) {
                int u1, u2, det;
                if (olivec_barycentric(x1, y1, x2, y2, x3, y3, x, y, &u1, &u2, &det)) {
                    int u3 = det - u1 - u2;
                    float z = z1*u1/det + z2*u2/det + z3*(det - u1 - u2)/det;
                    float tx = tx1*u1/det + tx2*u2/det + tx3*u3/det;
                    float ty = ty1*u1/det + ty2*u2/det + ty3*u3/det;

                    float texture_x = tx/z*texture.width;
                    if (texture_x < 0) texture_x = 0;
                    if (texture_x >= (float) texture.width) texture_x = texture.width - 1;

                    float texture_y = ty/z*texture.height;
                    if (texture_y < 0) texture_y = 0;
                    if (texture_y >= (float) texture.height) texture_y = texture.height - 1;

                    int precision = 100;
                    OLIVEC_PIXEL(oc, x, y) = olivec_pixel_bilinear(
                                                 texture,
                                                 texture_x*precision, texture_y*precision,
                                                 precision, precision);
                }
            }
        }
    }
}

// TODO: AA for triangle
 void olivec_triangle(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color)
{
    int lx, hx, ly, hy;
    if (olivec_normalize_triangle(oc.width, oc.height, x1, y1, x2, y2, x3, y3, &lx, &hx, &ly, &hy)) {
        for (int y = ly; y <= hy; ++y) {
            for (int x = lx; x <= hx; ++x) {
                int u1, u2, det;
                if (olivec_barycentric(x1, y1, x2, y2, x3, y3, x, y, &u1, &u2, &det)) {
                    olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), color);
                }
            }
        }
    }
}

 void olivec_text(Olivec_Canvas oc, const char *text, int tx, int ty, Olivec_Font font, size_t glyph_size, uint32_t color)
{
    for (size_t i = 0; *text; ++i, ++text) {
        int gx = tx + i*font.width*glyph_size;
        int gy = ty;
        const char *glyph = &font.glyphs[(*text)*sizeof(char)*font.width*font.height];
        for (int dy = 0; (size_t) dy < font.height; ++dy) {
            for (int dx = 0; (size_t) dx < font.width; ++dx) {
                int px = gx + dx*glyph_size;
                int py = gy + dy*glyph_size;
                if (0 <= px && px < (int) oc.width && 0 <= py && py < (int) oc.height) {
                    if (glyph[dy*font.width + dx]) {
                        olivec_rect(oc, px, py, glyph_size, glyph_size, color);
                    }
                }
            }
        }
    }
}

 void olivec_sprite_blend(Olivec_Canvas oc, int x, int y, int w, int h, Olivec_Canvas sprite)
{
    if (sprite.width == 0) return;
    if (sprite.height == 0) return;

    Olivec_Normalized_Rect nr = {0};
    if (!olivec_normalize_rect(x, y, w, h, oc.width, oc.height, &nr)) return;

    int xa = nr.ox1;
    if (w < 0) xa = nr.ox2;
    int ya = nr.oy1;
    if (h < 0) ya = nr.oy2;
    for (int y = nr.y1; y <= nr.y2; ++y) {
        for (int x = nr.x1; x <= nr.x2; ++x) {
            size_t nx = (x - xa)*((int) sprite.width)/w;
            size_t ny = (y - ya)*((int) sprite.height)/h;
            olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), OLIVEC_PIXEL(sprite, nx, ny));
        }
    }
}

 void olivec_sprite_copy(Olivec_Canvas oc, int x, int y, int w, int h, Olivec_Canvas sprite)
{
    if (sprite.width == 0) return;
    if (sprite.height == 0) return;

    // TODO: consider introducing flip parameter instead of relying on negative width and height
    // Similar to how SDL_RenderCopyEx does that
    Olivec_Normalized_Rect nr = {0};
    if (!olivec_normalize_rect(x, y, w, h, oc.width, oc.height, &nr)) return;

    int xa = nr.ox1;
    if (w < 0) xa = nr.ox2;
    int ya = nr.oy1;
    if (h < 0) ya = nr.oy2;
    for (int y = nr.y1; y <= nr.y2; ++y) {
        for (int x = nr.x1; x <= nr.x2; ++x) {
            size_t nx = (x - xa)*((int) sprite.width)/w;
            size_t ny = (y - ya)*((int) sprite.height)/h;
            OLIVEC_PIXEL(oc, x, y) = OLIVEC_PIXEL(sprite, nx, ny);
        }
    }
}

// TODO: olivec_pixel_bilinear does not check for out-of-bounds
// But maybe it shouldn't. Maybe it's a responsibility of the caller of the function.
 uint32_t olivec_pixel_bilinear(Olivec_Canvas sprite, int nx, int ny, int w, int h)
{
    int px = nx%w;
    int py = ny%h;

    int x1 = nx/w, x2 = nx/w;
    int y1 = ny/h, y2 = ny/h;
    if (px < w/2) {
        // left
        px += w/2;
        x1 -= 1;
        if (x1 < 0) x1 = 0;
    } else {
        // right
        px -= w/2;
        x2 += 1;
        if ((size_t) x2 >= sprite.width) x2 = sprite.width - 1;
    }

    if (py < h/2) {
        // top
        py += h/2;
        y1 -= 1;
        if (y1 < 0) y1 = 0;
    } else {
        // bottom
        py -= h/2;
        y2 += 1;
        if ((size_t) y2 >= sprite.height) y2 = sprite.height - 1;
    }

    return mix_colors2(mix_colors2(OLIVEC_PIXEL(sprite, x1, y1),
                                   OLIVEC_PIXEL(sprite, x2, y1),
                                   px, w),
                       mix_colors2(OLIVEC_PIXEL(sprite, x1, y2),
                                   OLIVEC_PIXEL(sprite, x2, y2),
                                   px, w),
                       py, h);
}

 void olivec_sprite_copy_bilinear(Olivec_Canvas oc, int x, int y, int w, int h, Olivec_Canvas sprite)
{
    // TODO: support negative size in olivec_sprite_copy_bilinear()
    if (w <= 0) return;
    if (h <= 0) return;

    Olivec_Normalized_Rect nr = {0};
    if (!olivec_normalize_rect(x, y, w, h, oc.width, oc.height, &nr)) return;

    for (int y = nr.y1; y <= nr.y2; ++y) {
        for (int x = nr.x1; x <= nr.x2; ++x) {
            size_t nx = (x - nr.ox1)*sprite.width;
            size_t ny = (y - nr.oy1)*sprite.height;
            OLIVEC_PIXEL(oc, x, y) = olivec_pixel_bilinear(sprite, nx, ny, w, h);
        }
    }
}


// TODO: Benchmarking
// TODO: SIMD implementations
// TODO: bezier curves
// TODO: olivec_ring
// TODO: fuzzer
// TODO: Stencil


Olivec_Canvas vc_render(float dt);


static uint32_t pixels[WIDTH*HEIGHT];
static float zbuffer[WIDTH*HEIGHT] = {0};
static float angle = 0;


#define vertices_count 118
static const float vertices[][3] = {
    {0.048039, 0.563912, -0.799652},
    {-0.421985, 0.563912, -0.646932},
    {-0.712475, 0.563912, -0.247106},
    {-0.712475, 0.563912, 0.247106},
    {-0.421985, 0.563912, 0.646932},
    {0.048039, 0.563912, 0.799652},
    {0.518062, 0.563912, 0.646932},
    {0.808553, 0.563912, 0.247106},
    {0.808553, 0.563912, -0.247106},
    {0.518062, 0.563912, -0.646932},
    {0.048039, 0.156786, -0.716293},
    {-0.372988, 0.156786, -0.579494},
    {-0.633196, 0.156786, -0.221347},
    {-0.633196, 0.156786, 0.221347},
    {-0.372988, 0.156786, 0.579494},
    {0.048039, 0.156786, 0.716293},
    {0.469066, 0.156786, 0.579494},
    {0.729275, 0.156786, 0.221347},
    {0.729275, 0.156786, -0.221347},
    {0.469066, 0.156786, -0.579494},
    {0.048039, -0.211548, -0.574568},
    {-0.289684, -0.211548, -0.464836},
    {-0.498408, -0.211548, -0.177551},
    {-0.498408, -0.211548, 0.177551},
    {-0.289684, -0.211548, 0.464836},
    {0.048039, -0.211548, 0.574568},
    {0.385762, -0.211548, 0.464836},
    {0.594486, -0.211548, 0.177551},
    {0.594486, -0.211548, -0.177551},
    {0.385762, -0.211548, -0.464836},
    {0.048039, -0.502768, -0.373209},
    {-0.171328, -0.502768, -0.301932},
    {-0.306904, -0.502768, -0.115327},
    {-0.306904, -0.502768, 0.115327},
    {-0.171328, -0.502768, 0.301932},
    {0.048039, -0.502768, 0.373209},
    {0.267406, -0.502768, 0.301932},
    {0.402982, -0.502768, 0.115328},
    {0.402982, -0.502768, -0.115328},
    {0.267406, -0.502768, -0.301932},
    {0.048039, -0.572172, -0.373209},
    {-0.171328, -0.572172, -0.301932},
    {-0.306904, -0.572172, -0.115327},
    {-0.306904, -0.572172, 0.115327},
    {-0.171328, -0.572172, 0.301932},
    {0.048039, -0.572172, 0.373209},
    {0.267406, -0.572172, 0.301932},
    {0.402982, -0.572172, 0.115328},
    {0.402982, -0.572172, -0.115328},
    {0.267406, -0.572172, -0.301932},
    {0.048039, 0.572172, -0.740687},
    {-0.387326, 0.572172, -0.599228},
    {-0.656396, 0.572172, -0.228885},
    {-0.656396, 0.572172, 0.228885},
    {-0.387326, 0.572172, 0.599228},
    {0.048039, 0.572172, 0.740687},
    {0.483404, 0.572172, 0.599228},
    {0.752474, 0.572172, 0.228885},
    {0.752474, 0.572172, -0.228885},
    {0.483404, 0.572172, -0.599228},
    {0.048039, 0.173190, -0.656666},
    {-0.337940, 0.173190, -0.531254},
    {-0.576487, 0.173190, -0.202921},
    {-0.576487, 0.173190, 0.202921},
    {-0.337940, 0.173190, 0.531254},
    {0.048039, 0.173190, 0.656666},
    {0.434017, 0.173190, 0.531254},
    {0.672565, 0.173190, 0.202921},
    {0.672565, 0.173190, -0.202921},
    {0.434017, 0.173190, -0.531254},
    {0.048039, -0.181146, -0.518781},
    {-0.256893, -0.181146, -0.419702},
    {-0.445351, -0.181146, -0.160312},
    {-0.445351, -0.181146, 0.160312},
    {-0.256893, -0.181146, 0.419702},
    {0.048039, -0.181146, 0.518781},
    {0.352971, -0.181146, 0.419702},
    {0.541429, -0.181146, 0.160312},
    {0.541429, -0.181146, -0.160313},
    {0.352971, -0.181146, -0.419702},
    {0.048039, -0.461316, -0.317240},
    {-0.138430, -0.461316, -0.256653},
    {-0.253675, -0.461316, -0.098033},
    {-0.253675, -0.461316, 0.098033},
    {-0.138430, -0.461316, 0.256653},
    {0.048039, -0.461316, 0.317240},
    {0.234508, -0.461316, 0.256653},
    {0.349753, -0.461316, 0.098033},
    {0.349753, -0.461316, -0.098033},
    {0.234508, -0.461316, -0.256653},
    {-0.622369, 0.355320, 0.302164},
    {-0.446753, 0.355206, 0.543556},
    {-0.604765, 0.260004, 0.294812},
    {-0.434281, 0.259893, 0.529151},
    {-0.734604, 0.344487, 0.406394},
    {-0.580657, 0.344370, 0.618054},
    {-0.720060, 0.262493, 0.395674},
    {-0.565908, 0.262374, 0.607616},
    {-0.808553, 0.270591, 0.474901},
    {-0.668662, 0.270500, 0.667188},
    {-0.750136, 0.232905, 0.432313},
    {-0.610051, 0.232813, 0.624867},
    {-0.799774, 0.138711, 0.488986},
    {-0.679203, 0.138623, 0.654715},
    {-0.739960, 0.165137, 0.445553},
    {-0.619387, 0.165049, 0.611284},
    {-0.642884, -0.164652, 0.405466},
    {-0.551090, -0.164718, 0.531662},
    {-0.611781, -0.120237, 0.382960},
    {-0.520074, -0.120302, 0.509037},
    {-0.473486, -0.325173, 0.291530},
    {-0.390344, -0.325227, 0.405814},
    {-0.475693, -0.276800, 0.293283},
    {-0.392711, -0.276854, 0.407345},
    {-0.414984, -0.268429, 0.228796},
    {-0.312556, -0.268496, 0.369589},
    {-0.389406, -0.316347, 0.210393},
    {-0.287114, -0.316413, 0.350998},
};

#define texcoords_count 0
static const float texcoords[1][3] = {0};
#define normals_count 0
static const float normals[1][3] = {0};
#define faces_count 224
static const int faces[224][9] = {
    {26, 17, 16, 0, 0, 0, 0, 0, 0},
    {9, 10, 0, 0, 0, 0, 0, 0, 0},
    {16, 7, 6, 0, 0, 0, 0, 0, 0},
    {13, 4, 3, 0, 0, 0, 0, 0, 0},
    {0, 11, 1, 0, 0, 0, 0, 0, 0},
    {7, 18, 8, 0, 0, 0, 0, 0, 0},
    {14, 5, 4, 0, 0, 0, 0, 0, 0},
    {1, 12, 2, 0, 0, 0, 0, 0, 0},
    {18, 9, 8, 0, 0, 0, 0, 0, 0},
    {15, 6, 5, 0, 0, 0, 0, 0, 0},
    {12, 3, 2, 0, 0, 0, 0, 0, 0},
    {23, 34, 24, 0, 0, 0, 0, 0, 0},
    {23, 14, 13, 0, 0, 0, 0, 0, 0},
    {20, 11, 10, 0, 0, 0, 0, 0, 0},
    {17, 28, 18, 0, 0, 0, 0, 0, 0},
    {24, 15, 14, 0, 0, 0, 0, 0, 0},
    {21, 12, 11, 0, 0, 0, 0, 0, 0},
    {28, 19, 18, 0, 0, 0, 0, 0, 0},
    {25, 16, 15, 0, 0, 0, 0, 0, 0},
    {22, 13, 12, 0, 0, 0, 0, 0, 0},
    {29, 10, 19, 0, 0, 0, 0, 0, 0},
    {33, 44, 34, 0, 0, 0, 0, 0, 0},
    {20, 31, 21, 0, 0, 0, 0, 0, 0},
    {27, 38, 28, 0, 0, 0, 0, 0, 0},
    {34, 25, 24, 0, 0, 0, 0, 0, 0},
    {21, 32, 22, 0, 0, 0, 0, 0, 0},
    {28, 39, 29, 0, 0, 0, 0, 0, 0},
    {35, 26, 25, 0, 0, 0, 0, 0, 0},
    {32, 23, 22, 0, 0, 0, 0, 0, 0},
    {39, 20, 29, 0, 0, 0, 0, 0, 0},
    {36, 27, 26, 0, 0, 0, 0, 0, 0},
    {45, 43, 41, 0, 0, 0, 0, 0, 0},
    {40, 31, 30, 0, 0, 0, 0, 0, 0},
    {47, 38, 37, 0, 0, 0, 0, 0, 0},
    {34, 45, 35, 0, 0, 0, 0, 0, 0},
    {41, 32, 31, 0, 0, 0, 0, 0, 0},
    {48, 39, 38, 0, 0, 0, 0, 0, 0},
    {45, 36, 35, 0, 0, 0, 0, 0, 0},
    {42, 33, 32, 0, 0, 0, 0, 0, 0},
    {49, 30, 39, 0, 0, 0, 0, 0, 0},
    {46, 37, 36, 0, 0, 0, 0, 0, 0},
    {100, 94, 96, 0, 0, 0, 0, 0, 0},
    {90, 95, 91, 0, 0, 0, 0, 0, 0},
    {96, 90, 92, 0, 0, 0, 0, 0, 0},
    {97, 92, 93, 0, 0, 0, 0, 0, 0},
    {91, 97, 93, 0, 0, 0, 0, 0, 0},
    {103, 101, 99, 0, 0, 0, 0, 0, 0},
    {101, 96, 97, 0, 0, 0, 0, 0, 0},
    {95, 101, 97, 0, 0, 0, 0, 0, 0},
    {94, 99, 95, 0, 0, 0, 0, 0, 0},
    {103, 109, 105, 0, 0, 0, 0, 0, 0},
    {102, 99, 98, 0, 0, 0, 0, 0, 0},
    {100, 102, 98, 0, 0, 0, 0, 0, 0},
    {101, 104, 100, 0, 0, 0, 0, 0, 0},
    {110, 107, 106, 0, 0, 0, 0, 0, 0},
    {106, 103, 102, 0, 0, 0, 0, 0, 0},
    {108, 102, 104, 0, 0, 0, 0, 0, 0},
    {105, 108, 104, 0, 0, 0, 0, 0, 0},
    {114, 110, 112, 0, 0, 0, 0, 0, 0},
    {112, 106, 108, 0, 0, 0, 0, 0, 0},
    {109, 112, 108, 0, 0, 0, 0, 0, 0},
    {107, 113, 109, 0, 0, 0, 0, 0, 0},
    {116, 111, 110, 0, 0, 0, 0, 0, 0},
    {115, 111, 117, 0, 0, 0, 0, 0, 0},
    {114, 113, 115, 0, 0, 0, 0, 0, 0},
    {26, 27, 17, 0, 0, 0, 0, 0, 0},
    {9, 19, 10, 0, 0, 0, 0, 0, 0},
    {16, 17, 7, 0, 0, 0, 0, 0, 0},
    {13, 14, 4, 0, 0, 0, 0, 0, 0},
    {0, 10, 11, 0, 0, 0, 0, 0, 0},
    {7, 17, 18, 0, 0, 0, 0, 0, 0},
    {14, 15, 5, 0, 0, 0, 0, 0, 0},
    {1, 11, 12, 0, 0, 0, 0, 0, 0},
    {18, 19, 9, 0, 0, 0, 0, 0, 0},
    {15, 16, 6, 0, 0, 0, 0, 0, 0},
    {12, 13, 3, 0, 0, 0, 0, 0, 0},
    {23, 33, 34, 0, 0, 0, 0, 0, 0},
    {23, 24, 14, 0, 0, 0, 0, 0, 0},
    {20, 21, 11, 0, 0, 0, 0, 0, 0},
    {17, 27, 28, 0, 0, 0, 0, 0, 0},
    {24, 25, 15, 0, 0, 0, 0, 0, 0},
    {21, 22, 12, 0, 0, 0, 0, 0, 0},
    {28, 29, 19, 0, 0, 0, 0, 0, 0},
    {25, 26, 16, 0, 0, 0, 0, 0, 0},
    {22, 23, 13, 0, 0, 0, 0, 0, 0},
    {29, 20, 10, 0, 0, 0, 0, 0, 0},
    {33, 43, 44, 0, 0, 0, 0, 0, 0},
    {20, 30, 31, 0, 0, 0, 0, 0, 0},
    {27, 37, 38, 0, 0, 0, 0, 0, 0},
    {34, 35, 25, 0, 0, 0, 0, 0, 0},
    {21, 31, 32, 0, 0, 0, 0, 0, 0},
    {28, 38, 39, 0, 0, 0, 0, 0, 0},
    {35, 36, 26, 0, 0, 0, 0, 0, 0},
    {32, 33, 23, 0, 0, 0, 0, 0, 0},
    {39, 30, 20, 0, 0, 0, 0, 0, 0},
    {36, 37, 27, 0, 0, 0, 0, 0, 0},
    {41, 40, 49, 0, 0, 0, 0, 0, 0},
    {49, 48, 41, 0, 0, 0, 0, 0, 0},
    {48, 47, 41, 0, 0, 0, 0, 0, 0},
    {47, 46, 45, 0, 0, 0, 0, 0, 0},
    {45, 44, 43, 0, 0, 0, 0, 0, 0},
    {43, 42, 41, 0, 0, 0, 0, 0, 0},
    {47, 45, 41, 0, 0, 0, 0, 0, 0},
    {40, 41, 31, 0, 0, 0, 0, 0, 0},
    {47, 48, 38, 0, 0, 0, 0, 0, 0},
    {34, 44, 45, 0, 0, 0, 0, 0, 0},
    {41, 42, 32, 0, 0, 0, 0, 0, 0},
    {48, 49, 39, 0, 0, 0, 0, 0, 0},
    {45, 46, 36, 0, 0, 0, 0, 0, 0},
    {42, 43, 33, 0, 0, 0, 0, 0, 0},
    {49, 40, 30, 0, 0, 0, 0, 0, 0},
    {46, 47, 37, 0, 0, 0, 0, 0, 0},
    {100, 98, 94, 0, 0, 0, 0, 0, 0},
    {90, 94, 95, 0, 0, 0, 0, 0, 0},
    {96, 94, 90, 0, 0, 0, 0, 0, 0},
    {97, 96, 92, 0, 0, 0, 0, 0, 0},
    {91, 95, 97, 0, 0, 0, 0, 0, 0},
    {103, 105, 101, 0, 0, 0, 0, 0, 0},
    {101, 100, 96, 0, 0, 0, 0, 0, 0},
    {95, 99, 101, 0, 0, 0, 0, 0, 0},
    {94, 98, 99, 0, 0, 0, 0, 0, 0},
    {103, 107, 109, 0, 0, 0, 0, 0, 0},
    {102, 103, 99, 0, 0, 0, 0, 0, 0},
    {100, 104, 102, 0, 0, 0, 0, 0, 0},
    {101, 105, 104, 0, 0, 0, 0, 0, 0},
    {110, 111, 107, 0, 0, 0, 0, 0, 0},
    {106, 107, 103, 0, 0, 0, 0, 0, 0},
    {108, 106, 102, 0, 0, 0, 0, 0, 0},
    {105, 109, 108, 0, 0, 0, 0, 0, 0},
    {114, 116, 110, 0, 0, 0, 0, 0, 0},
    {112, 110, 106, 0, 0, 0, 0, 0, 0},
    {109, 113, 112, 0, 0, 0, 0, 0, 0},
    {107, 111, 113, 0, 0, 0, 0, 0, 0},
    {116, 117, 111, 0, 0, 0, 0, 0, 0},
    {115, 113, 111, 0, 0, 0, 0, 0, 0},
    {114, 112, 113, 0, 0, 0, 0, 0, 0},
    {68, 57, 58, 0, 0, 0, 0, 0, 0},
    {78, 67, 68, 0, 0, 0, 0, 0, 0},
    {65, 54, 55, 0, 0, 0, 0, 0, 0},
    {62, 51, 52, 0, 0, 0, 0, 0, 0},
    {69, 58, 59, 0, 0, 0, 0, 0, 0},
    {56, 65, 55, 0, 0, 0, 0, 0, 0},
    {63, 52, 53, 0, 0, 0, 0, 0, 0},
    {60, 59, 50, 0, 0, 0, 0, 0, 0},
    {67, 56, 57, 0, 0, 0, 0, 0, 0},
    {64, 53, 54, 0, 0, 0, 0, 0, 0},
    {51, 60, 50, 0, 0, 0, 0, 0, 0},
    {75, 84, 74, 0, 0, 0, 0, 0, 0},
    {65, 74, 64, 0, 0, 0, 0, 0, 0},
    {72, 61, 62, 0, 0, 0, 0, 0, 0},
    {79, 68, 69, 0, 0, 0, 0, 0, 0},
    {66, 75, 65, 0, 0, 0, 0, 0, 0},
    {63, 72, 62, 0, 0, 0, 0, 0, 0},
    {60, 79, 69, 0, 0, 0, 0, 0, 0},
    {67, 76, 66, 0, 0, 0, 0, 0, 0},
    {74, 63, 64, 0, 0, 0, 0, 0, 0},
    {71, 60, 61, 0, 0, 0, 0, 0, 0},
    {83, 87, 89, 0, 0, 0, 0, 0, 0},
    {82, 71, 72, 0, 0, 0, 0, 0, 0},
    {89, 78, 79, 0, 0, 0, 0, 0, 0},
    {76, 85, 75, 0, 0, 0, 0, 0, 0},
    {73, 82, 72, 0, 0, 0, 0, 0, 0},
    {80, 79, 70, 0, 0, 0, 0, 0, 0},
    {77, 86, 76, 0, 0, 0, 0, 0, 0},
    {84, 73, 74, 0, 0, 0, 0, 0, 0},
    {81, 70, 71, 0, 0, 0, 0, 0, 0},
    {88, 77, 78, 0, 0, 0, 0, 0, 0},
    {68, 67, 57, 0, 0, 0, 0, 0, 0},
    {78, 77, 67, 0, 0, 0, 0, 0, 0},
    {65, 64, 54, 0, 0, 0, 0, 0, 0},
    {62, 61, 51, 0, 0, 0, 0, 0, 0},
    {69, 68, 58, 0, 0, 0, 0, 0, 0},
    {56, 66, 65, 0, 0, 0, 0, 0, 0},
    {63, 62, 52, 0, 0, 0, 0, 0, 0},
    {60, 69, 59, 0, 0, 0, 0, 0, 0},
    {67, 66, 56, 0, 0, 0, 0, 0, 0},
    {64, 63, 53, 0, 0, 0, 0, 0, 0},
    {51, 61, 60, 0, 0, 0, 0, 0, 0},
    {75, 85, 84, 0, 0, 0, 0, 0, 0},
    {65, 75, 74, 0, 0, 0, 0, 0, 0},
    {72, 71, 61, 0, 0, 0, 0, 0, 0},
    {79, 78, 68, 0, 0, 0, 0, 0, 0},
    {66, 76, 75, 0, 0, 0, 0, 0, 0},
    {63, 73, 72, 0, 0, 0, 0, 0, 0},
    {60, 70, 79, 0, 0, 0, 0, 0, 0},
    {67, 77, 76, 0, 0, 0, 0, 0, 0},
    {74, 73, 63, 0, 0, 0, 0, 0, 0},
    {71, 70, 60, 0, 0, 0, 0, 0, 0},
    {89, 80, 81, 0, 0, 0, 0, 0, 0},
    {81, 82, 83, 0, 0, 0, 0, 0, 0},
    {83, 84, 85, 0, 0, 0, 0, 0, 0},
    {85, 86, 87, 0, 0, 0, 0, 0, 0},
    {87, 88, 89, 0, 0, 0, 0, 0, 0},
    {89, 81, 83, 0, 0, 0, 0, 0, 0},
    {83, 85, 87, 0, 0, 0, 0, 0, 0},
    {82, 81, 71, 0, 0, 0, 0, 0, 0},
    {89, 88, 78, 0, 0, 0, 0, 0, 0},
    {76, 86, 85, 0, 0, 0, 0, 0, 0},
    {73, 83, 82, 0, 0, 0, 0, 0, 0},
    {80, 89, 79, 0, 0, 0, 0, 0, 0},
    {77, 87, 86, 0, 0, 0, 0, 0, 0},
    {84, 83, 73, 0, 0, 0, 0, 0, 0},
    {81, 80, 70, 0, 0, 0, 0, 0, 0},
    {88, 87, 77, 0, 0, 0, 0, 0, 0},
    {3, 52, 2, 0, 0, 0, 0, 0, 0},
    {0, 59, 9, 0, 0, 0, 0, 0, 0},
    {57, 6, 7, 0, 0, 0, 0, 0, 0},
    {4, 53, 3, 0, 0, 0, 0, 0, 0},
    {51, 0, 1, 0, 0, 0, 0, 0, 0},
    {58, 7, 8, 0, 0, 0, 0, 0, 0},
    {55, 4, 5, 0, 0, 0, 0, 0, 0},
    {52, 1, 2, 0, 0, 0, 0, 0, 0},
    {9, 58, 8, 0, 0, 0, 0, 0, 0},
    {56, 5, 6, 0, 0, 0, 0, 0, 0},
    {3, 53, 52, 0, 0, 0, 0, 0, 0},
    {0, 50, 59, 0, 0, 0, 0, 0, 0},
    {57, 56, 6, 0, 0, 0, 0, 0, 0},
    {4, 54, 53, 0, 0, 0, 0, 0, 0},
    {51, 50, 0, 0, 0, 0, 0, 0, 0},
    {58, 57, 7, 0, 0, 0, 0, 0, 0},
    {55, 54, 4, 0, 0, 0, 0, 0, 0},
    {52, 51, 1, 0, 0, 0, 0, 0, 0},
    {9, 59, 58, 0, 0, 0, 0, 0, 0},
    {56, 55, 5, 0, 0, 0, 0, 0, 0},
};

typedef struct {
    float x, y;
} Vector2;

static Vector2 make_vector2(float x, float y)
{
    Vector2 v2;
    v2.x = x;
    v2.y = y;
    return v2;
}

typedef struct {
    float x, y, z;
} Vector3;

static Vector3 make_vector3(float x, float y, float z)
{
    Vector3 v3;
    v3.x = x;
    v3.y = y;
    v3.z = z;
    return v3;
}

#define EPSILON 1e-6

static Vector2 project_3d_2d(Vector3 v3)
{
    if (v3.z < 0) v3.z = -v3.z;
    if (v3.z < EPSILON) v3.z += EPSILON;
    return make_vector2(v3.x/v3.z, v3.y/v3.z);
}

static Vector2 project_2d_scr(Vector2 v2)
{
    return make_vector2((v2.x + 1)/2*WIDTH, (1 - (v2.y + 1)/2)*HEIGHT);
}

static Vector3 rotate_y(Vector3 p, float delta_angle)
{
    float angle = atan2f(p.z, p.x) + delta_angle;
    float mag = _sqrt(p.x*p.x + p.z*p.z);
    return make_vector3(_cos(angle)*mag, p.y, _sin(angle)*mag);
}

typedef enum {
    FACE_V1,
    FACE_V2,
    FACE_V3,
    FACE_VT1,
    FACE_VT2,
    FACE_VT3,
    FACE_VN1,
    FACE_VN2,
    FACE_VN3,
} Face_Index;

float vector3_dot(Vector3 a, Vector3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}


int hsl16[][3] = {
    {0,255,0},
    {42,127,255},
    {16,127,255},
    {0,110,255},
    {229,127,255},
    {185,75,255},
    {170,101,255},
    {144,127,255},
    {85,84,255},
    {85,50,255},
    {22,50,255},
    {20,102,120},
    {0,185,0},
    {0,134,0},
    {0,69,0},
    {0,0,0}
};

int distance_hsl16(int i, int h, int s, int l)
{
    int dh = h - hsl16[i][0];
    int ds = s - hsl16[i][1];
    int dl = l - hsl16[i][2];
    return dh*dh + ds*ds + dl*dl;
}

// TODO: bring find_ansi_index_by_rgb from image2term
int find_ansi_index_by_hsl(int h, int s, int l)
{
    int index = 0;
    for (int i = 0; i < 256; ++i) {
        if (distance_hsl16(i, h, s, l) < distance_hsl16(index, h, s, l)) {
            index = i;
        }
    }
    return index;
}



Olivec_Canvas vc_render(float dt)
{
    angle += 0.25*PI*dt;

    printf("init canvas...");
    Olivec_Canvas oc = olivec_canvas(pixels, WIDTH, HEIGHT, WIDTH);
    olivec_fill(oc, BACKGROUND_COLOR);
    olivec_text(oc, "tea cup", 0, 0, olivec_default_font, 1, 0xFFFFFFFF);

    printf("ok\nclearing zbuf...");
    for (size_t i = 0; i < WIDTH*HEIGHT; ++i) zbuffer[i] = 0;
    printf("ok\nprojecting");

    Vector3 camera = {0, 0, 1};
    for (size_t i = 0; i < faces_count; ++i) {
        int a, b, c;
        a = faces[i][FACE_V1];
        b = faces[i][FACE_V2];
        c = faces[i][FACE_V3];
        
        Vector3 v1 = rotate_y(make_vector3(vertices[a][0], vertices[a][1], vertices[a][2]), angle);
        Vector3 v2 = rotate_y(make_vector3(vertices[b][0], vertices[b][1], vertices[b][2]), angle);
        Vector3 v3 = rotate_y(make_vector3(vertices[c][0], vertices[c][1], vertices[c][2]), angle);
        v1.z += 1.5; v2.z += 1.5; v3.z += 1.5;
        a = faces[i][FACE_VN1];
        b = faces[i][FACE_VN2];
        c = faces[i][FACE_VN3];
        Vector3 vn1 = rotate_y(make_vector3(normals[a][0], normals[a][1], normals[a][2]), angle);
        Vector3 vn2 = rotate_y(make_vector3(normals[b][0], normals[b][1], normals[b][2]), angle);
        Vector3 vn3 = rotate_y(make_vector3(normals[c][0], normals[c][1], normals[c][2]), angle);
        if (vector3_dot(camera, vn1) > 0.0 &&
            vector3_dot(camera, vn2) > 0.0 &&
            vector3_dot(camera, vn3) > 0.0) {
                continue;
            }

        Vector2 p1 = project_2d_scr(project_3d_2d(v1));
        Vector2 p2 = project_2d_scr(project_3d_2d(v2));
        Vector2 p3 = project_2d_scr(project_3d_2d(v3));

        int x1 = p1.x;
        int x2 = p2.x;
        int x3 = p3.x;
        int y1 = p1.y;
        int y2 = p2.y;
        int y3 = p3.y;
        int lx, hx, ly, hy;
        if (olivec_normalize_triangle(oc.width, oc.height, x1, y1, x2, y2, x3, y3, &lx, &hx, &ly, &hy)) {
            for (int y = ly; y <= hy; ++y) {
                for (int x = lx; x <= hx; ++x) {
                    int u1, u2, det;
                    if (olivec_barycentric(x1, y1, x2, y2, x3, y3, x, y, &u1, &u2, &det)) {
                        int u3 = det - u1 - u2;
                        float z = 1/(v1.z*u1)/det + 1/(v2.z*u2)/det + 1/(v3.z*u3)/det;
                        float near = 0.1f;
                        float far = 5.0f;
                        if ((1.0f/far) < z && z < (1.0f/near) && (z > zbuffer[y*WIDTH + x])) {
                            print("infronf");
                            zbuffer[y*WIDTH + x] = z;
                            OLIVEC_PIXEL(oc, x, y) = mix_colors3(0xFF1818FF, 0xFF18FF18, 0xFFFF1818, u1, u2, det);

                            z = 1.0f/z;
                            if (z >= 1.0) {
                                z -= 1.0;
                                uint32_t v = z*255;
                                if (v > 255) v = 255;
                                olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), (v<<(3*8)));
                            }
                        }
                    }
                }
            }
        }
    }

    printf("canvas done\n");
    return oc;
} 



static uint32_t pixels1[WIDTH*HEIGHT];
static float zbuffer1[WIDTH*HEIGHT];
static uint32_t pixels2[WIDTH*HEIGHT];
static float zbuffer2[WIDTH*HEIGHT];
static float global_time = 1.0;



static void vc_term_compress_pixels(Olivec_Canvas oc, char* fb)
{
    for (size_t y = 0; y < HEIGHT; ++y) {
        for (size_t x = 0; x < WIDTH; x = x + 2) {
            uint32_t cpa = OLIVEC_PIXEL(oc, x, y);
            int ra = OLIVEC_RED(cpa);
            int ga = OLIVEC_GREEN(cpa);
            int ba = OLIVEC_BLUE(cpa);
            int aa = OLIVEC_ALPHA(cpa);
            uint8_t colix_a = aa*(ra+ga+ba)/(255*3*(255/15));

            uint32_t cpb = OLIVEC_PIXEL(oc, x+1, y);
            int rb = OLIVEC_RED(cpb);
            int gb = OLIVEC_GREEN(cpb);
            int bb = OLIVEC_BLUE(cpb);
            int ab = OLIVEC_ALPHA(cpb);
            uint8_t colix_b = ab*(rb+gb+bb)/(255*3*(255/15));
            
            writeb(colix_b + (colix_a << 4), fb + x/2 + y * (1920/4));
        }
    }
}

int main(int argc, char *argv[]) {

	printf("olive.c framework test\n");
	printf("Build: %s %s\n", __DATE__, __TIME__);


    char* fbuf = frame();
    int colval = 10;
    memset(fbuf, colval + colval * 16, 0x80000);
    printf("set up color value %d\n", colval);
    while(1){
        vc_term_compress_pixels(vc_render(1.f), fbuf);
    }
    exit(0);
}
