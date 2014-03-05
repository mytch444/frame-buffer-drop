#define p_x 0
#define p_y 1

void draw(int color, int x, int y);
void draw_score(int score);
void draw_char(int c, int x, int y);
void clear();
void clear_all();
void draw_border();
void draw_square(int color, int x, int y, int w, int h);
void draw_square_empty(int color, int x, int y, int w, int h);
int setup();
void packup();

// Window width and height.
int width = 600;
int height = 900;
int xoffset, yoffset; // Offset so that the window is in the middle.

// Stuff for framebuffer.
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
int fbfd = 0;
int *fbp;

/*
 * A buffer of changes that need to be made on the next draw_buffer.
 * Stored as c,x,y,c,x,y...
 *
 */
int *buffer;
int changes;

int xmult, ymult;

int border_color = 0xffffffff;

int *font_buffer;

/* 
 * Color is a hex.
 * 0xaarrggbb
 * Alpha doesn't see to work. So what the first two bytes do I have no idea.
 */
void draw(int color, int x, int y) {
    for (; x >= width; x -= width);
    for (; x < 0; x += width);
    if (y >= height || y < 0) return; // I don't want vertical wrapping.

    buffer[changes + p_x] = x;
    buffer[changes + p_y] = y;
    changes += 2;

    fbp[(x + xoffset) * xmult + (y + yoffset) * ymult] = color;
}

void draw_score(int score) {
    int m = 1000000;
    int p = 0;

    while (score) {
        draw_char(score / m, p++ * 16, 0);

        score %= m;
        m /= 10;
    }
}

void draw_char(int c, int x, int y) {
    int xoffset, xx, yy;

    xoffset = c * 17;
    for (xx = 0; xx < 16; xx++) {
        for (yy = 0; yy < 16; yy++) {
            if (font_buffer[xx + xoffset + 170 * yy])
                draw(0x00ffffff, xx + x, yy + y);
        }
    }
}

void clear() {
    int i, c;
    c = changes;
    changes = 0;
    for (i = 0; i < c; i += 2)
        draw(0, buffer[i + p_x], buffer[i + p_y]);

    changes = 0; 
}

void clear_all() {
    long int i;
    for (i = 0; i < screensize / sizeof(int); i++) {
        fbp[i] = 0;
    }
}

/*
 * Cannot use draw_square_empty as this is outside of the window.
 */
void draw_border() {
  int location, x, y;

  for (x = -1; x < width + 1; x++) {
      location = (x + xoffset) * ((vinfo.bits_per_pixel / 8) / sizeof(int))
          + (yoffset - 1) * (finfo.line_length / sizeof(int));

      fbp[location] = border_color;

      location = (x + xoffset) * ((vinfo.bits_per_pixel / 8) / sizeof(int))
          + (yoffset + height) * (finfo.line_length / sizeof(int));

      fbp[location] = border_color;
  }

  for (y = -1; y < height + 1; y++) {
      location = (xoffset - 1) * ((vinfo.bits_per_pixel / 8) / sizeof(int))
          + (yoffset + y) * (finfo.line_length / sizeof(int));

      fbp[location] = border_color;

      location = (xoffset + width) * ((vinfo.bits_per_pixel / 8) / sizeof(int))
          + (yoffset + y) * (finfo.line_length / sizeof(int));

      fbp[location] = border_color;
  }

}

void draw_square(int color, int x, int y, int w, int h) {
    int xx, yy;

    for (xx = x; xx < x + w; xx++)
        for (yy = y; yy < y + h; yy++)
            draw(color, xx, yy);
}

void draw_square_empty(int color, int x, int y, int w, int h) {
    int xx, yy;

    for (xx = x; xx < x + w; xx++) {
        draw(color, xx, y);
        draw(color, xx, y + h);
    }

    for (yy = y; yy < y + h; yy++) {
        draw(color, x, yy);
        draw(color, x + w, yy);
    }
}

int setup_fb() {
    FILE *font_f;
    int a, r, g, b, x, y;

    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd) {
        printf("Error opening fb\n");
        return 1;
    }

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed\n");
        return 1;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable\n");
        return 1;
    }

    if (vinfo.xres < width)
        width = vinfo.xres;
    if (vinfo.yres < height)
        height = vinfo.yres;

    // Set the offset needed to set the window in the middle of the screen.
    xoffset = vinfo.xres / 2 - width / 2 + vinfo.xoffset;
    yoffset = vinfo.yres / 2 - height / 2 + vinfo.yoffset;
 
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
  
    // Get the framebuffer date
    fbp = (int *) mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    // Need to test if it failed as I don't think what was there worked.

    xmult = ((vinfo.bits_per_pixel / 8) / sizeof(int));
    ymult = (finfo.line_length / sizeof(int));

    changes = 0;
    buffer = NULL;
    buffer = malloc(sizeof(int) * (width * height * 64));
    if (!buffer) {
        printf("Error malloc buffer\n");
        return 1;
    }

    /*
     * Load the font bitmap into memory.
     *
     */
   
    font_f = fopen("font.raw", "r");
    if (!font_f) {
        printf("Error opening 'font.raw' for font.\n");
        return 1;
    }

    font_buffer = malloc(sizeof(int) * (170 * 16));
    x = y = 0;
    do {
        a = fgetc(font_f);
        r = fgetc(font_f);
        g = fgetc(font_f);
        b = fgetc(font_f);

        font_buffer[x + 170 * y] = 0;
        if (a || r || g || b)
            font_buffer[x + 170 * y] = 1;

        x++;

        if (x >= 170) {
            x = 0;
            y++;
        }
    } while (y < 16);

    // Hide the cursor.
    printf("\x1B[?1c    \n");

    return 0;
}

void packup_fb() {
    // Bring back the cursor.
    printf("\x1B[?0c    \n");

    free(buffer);

    // Unmap and close the framebuffer
    munmap(fbp, screensize);
    close(fbfd);
}

