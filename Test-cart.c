#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"

#define WIDTH 64
#define HEIGHT 64
#define FRAME_SYNC 0x26  // '&'
#define PI 3.14159265

// Fireworks parameters
#define MAX_PARTICLES 20
#define MAX_EXPLOSIONS 3
#define MAX_AGE 8

// Multiple bouncing balls
#define BALLS 4

// Tiny 4x5 font for title cards (A-Z + space)
const uint8_t font4x5[][5] = {
    {0x0E,0x11,0x1F,0x11,0x11}, // A
    {0x1E,0x11,0x1E,0x11,0x1E}, // B
    {0x0E,0x11,0x10,0x11,0x0E}, // C
    {0x1C,0x12,0x11,0x12,0x1C}, // D
    {0x1F,0x10,0x1E,0x10,0x1F}, // E
    {0x1F,0x10,0x1E,0x10,0x10}, // F
    {0x0F,0x10,0x17,0x11,0x0F}, // G
    {0x11,0x11,0x1F,0x11,0x11}, // H
    {0x0E,0x04,0x04,0x04,0x0E}, // I
    {0x07,0x02,0x02,0x12,0x0C}, // J
    {0x11,0x12,0x1C,0x12,0x11}, // K
    {0x10,0x10,0x10,0x10,0x1F}, // L
    {0x11,0x1B,0x15,0x11,0x11}, // M
    {0x11,0x13,0x15,0x19,0x11}, // N
    {0x0E,0x11,0x11,0x11,0x0E}, // O
    {0x1E,0x11,0x1E,0x10,0x10}, // P
    {0x0E,0x11,0x11,0x15,0x0E}, // Q
    {0x1E,0x11,0x1E,0x12,0x11}, // R
    {0x0F,0x10,0x0E,0x01,0x1E}, // S
    {0x1F,0x04,0x04,0x04,0x04}, // T
    {0x11,0x11,0x11,0x11,0x0E}, // U
    {0x11,0x11,0x11,0x0A,0x04}, // V
    {0x11,0x11,0x15,0x1B,0x11}, // W
    {0x11,0x0A,0x04,0x0A,0x11}, // X
    {0x11,0x0A,0x04,0x04,0x04}, // Y
    {0x1F,0x02,0x04,0x08,0x1F}, // Z
    {0x00,0x00,0x00,0x00,0x00}  // space
};

// Framebuffer structure
typedef struct {
    uint8_t *buf;
    int width;
    int height;
} FrameBuffer;

// Fireworks particle
typedef struct {
    int x, y;
    int dx, dy;
    int age;
    uint8_t brightness;
    int active;
} Particle;

typedef struct {
    int cx, cy;
    Particle particles[MAX_PARTICLES];
    int active;
} Explosion;

// Utility: fill framebuffer
void fb_fill(FrameBuffer *fb, uint8_t color) {
    for (int i = 0; i < fb->width * fb->height; i++)
        fb->buf[i] = color & 0x03;
}

// Utility: set a pixel
void fb_pixel(FrameBuffer *fb, int x, int y, uint8_t color) {
    if (x >= 0 && x < fb->width && y >= 0 && y < fb->height)
        fb->buf[y * fb->width + x] = color & 0x03;
}

// Compress 4 pixels per byte and send frame
void compress_and_send(FrameBuffer *fb) {
    for (int i = 0; i < fb->width * fb->height; i += 4) {
        uint8_t b = ((fb->buf[i] & 0x03) << 6) |
                    ((fb->buf[i+1] & 0x03) << 4) |
                    ((fb->buf[i+2] & 0x03) << 2) |
                    (fb->buf[i+3] & 0x03);
        fwrite(&b, 1, 1, stdout);
    }
}

void send_frame(FrameBuffer *fb) {
    putchar(FRAME_SYNC);
    compress_and_send(fb);
}

// Draw a character
void fb_draw_char(FrameBuffer *fb, char c, int x, int y, uint8_t color) {
    int index = (c >= 'A' && c <= 'Z') ? c - 'A' : 26; // space fallback
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 4; col++) {
            if (font4x5[index][row] & (1 << (3-col))) {
                fb_pixel(fb, x+col, y+row, color);
            }
        }
    }
}

// Draw a string
void fb_draw_text(FrameBuffer *fb, const char *text, int x, int y, uint8_t color) {
    while (*text) {
        fb_draw_char(fb, *text, x, y, color);
        x += 5;
        text++;
    }
}

// Title card animation
void title_card(FrameBuffer *fb, const char *title, int frames) {
    for (int f = 0; f < frames; f++) {
        fb_fill(fb, 0);
        fb_draw_text(fb, title, 5, HEIGHT/2 - 3, 3);
        send_frame(fb);
        sleep_ms(41);
    }
}

// Fade to black
void fade_to_black(FrameBuffer *fb, int steps) {
    for (int s = steps; s >= 0; s--) {
        for (int i = 0; i < WIDTH*HEIGHT; i++)
            fb->buf[i] = fb->buf[i] * s / steps;
        send_frame(fb);
        sleep_ms(41);
    }
}

// ---------- Animations ----------

// Bouncing ball
void bouncing_ball(FrameBuffer *fb) {
    int x = 0, y = 0, dx = 1, dy = 1;
    for (int f = 0; f < 150; f++) {
        fb_fill(fb, 0);
        fb_pixel(fb, x, y, 3);
        send_frame(fb);

        x += dx; y += dy;
        if (x <= 0 || x >= WIDTH-1) dx = -dx;
        if (y <= 0 || y >= HEIGHT-1) dy = -dy;
        sleep_ms(41);
    }
}

// Spinning ring
void spinning_ring(FrameBuffer *fb) {
    float angle = 0.0f;
    int cx = WIDTH/2, cy = HEIGHT/2, radius = 20;
    for (int f = 0; f < 150; f++) {
        fb_fill(fb, 0);
        for (int i = 0; i < 360; i += 10) {
            float rad = (i + angle) * PI / 180.0f;
            int px = cx + (int)(radius * cosf(rad));
            int py = cy + (int)(radius * sinf(rad));
            fb_pixel(fb, px, py, 3);
        }
        send_frame(fb);
        angle += 5.0f;
        if (angle >= 360.0f) angle -= 360.0f;
        sleep_ms(41);
    }
}

// Fireworks
void init_explosion(Explosion *ex) {
    ex->cx = rand() % WIDTH;
    ex->cy = rand() % HEIGHT;
    ex->active = 1;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        ex->particles[i].x = ex->cx;
        ex->particles[i].y = ex->cy;
        ex->particles[i].dx = (rand() % 7) - 3;
        ex->particles[i].dy = (rand() % 7) - 3;
        ex->particles[i].age = 0;
        ex->particles[i].brightness = 3;
        ex->particles[i].active = 1;
    }
}

void update_explosion(Explosion *ex) {
    int active_particles = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ex->particles[i];
        if (!p->active) continue;
        p->x += p->dx;
        p->y += p->dy;
        p->age++;
        if (p->age > MAX_AGE || p->x < 0 || p->x >= WIDTH || p->y < 0 || p->y >= HEIGHT)
            p->active = 0;
        else {
            p->brightness = 3 - (p->age * 3 / MAX_AGE);
            active_particles++;
        }
    }
    if (active_particles == 0) ex->active = 0;
}

void fireworks(FrameBuffer *fb) {
    Explosion explosions[MAX_EXPLOSIONS] = {0};
    for (int f = 0; f < 150; f++) {
        fb_fill(fb, 0);
        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (!explosions[i].active && (rand() % 20 == 0))
                init_explosion(&explosions[i]);
            if (explosions[i].active) {
                update_explosion(&explosions[i]);
                for (int j = 0; j < MAX_PARTICLES; j++) {
                    Particle *p = &explosions[i].particles[j];
                    if (p->active) fb_pixel(fb, p->x, p->y, p->brightness);
                }
            }
        }
        send_frame(fb);
        sleep_ms(41);
    }
}

// Diagonal wave
void diagonal_wave(FrameBuffer *fb) {
    for (int f = 0; f < 150; f++) {
        fb_fill(fb, 0);
        for (int y = 0; y < HEIGHT; y++) {
            int x = (y + f) % WIDTH;
            fb_pixel(fb, x, y, 3);
        }
        send_frame(fb);
        sleep_ms(41);
    }
}

// Multiple bouncing balls
void multiple_balls(FrameBuffer *fb) {
    int x[BALLS] = {0, 63, 0, 63};
    int y[BALLS] = {0, 0, 63, 63};
    int dx[BALLS] = {1, -1, 1, -1};
    int dy[BALLS] = {1, 1, -1, -1};

    for (int f = 0; f < 150; f++) {
        fb_fill(fb, 0);
        for (int i = 0; i < BALLS; i++) {
            fb_pixel(fb, x[i], y[i], 3);
            x[i] += dx[i]; y[i] += dy[i];
            if (x[i] <= 0 || x[i] >= WIDTH-1) dx[i] = -dx[i];
            if (y[i] <= 0 || y[i] >= HEIGHT-1) dy[i] = -dy[i];
        }
        send_frame(fb);
        sleep_ms(41);
    }
}

// Pulsating blob
void pulsating_blob(FrameBuffer *fb) {
    int cx = WIDTH/2, cy = HEIGHT/2;
    for (int f = 0; f < 150; f++) {
        fb_fill(fb, 0);
        int radius = 5 + (f % 10);
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius)
                    fb_pixel(fb, cx+x, cy+y, 3);
            }
        }
        send_frame(fb);
        sleep_ms(41);
    }
}

// Rotating stripes
void rotating_stripes(FrameBuffer *fb) {
    int offset = 0;
    for (int f = 0; f < 150; f++) {
        fb_fill(fb, 0);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if ((x + y + offset) % 8 < 4)
                    fb_pixel(fb, x, y, 3);
            }
        }
        offset++;
        send_frame(fb);
        sleep_ms(41);
    }
}

// ---------- Main ----------
int main() {
    stdio_init_all();
    sleep_ms(2000); // Give time for USB

    uint8_t *buffer = malloc(WIDTH*HEIGHT);
    if (!buffer) return 1;
    FrameBuffer fb = {buffer, WIDTH, HEIGHT};

    while (1) {
        title_card(&fb, "BOUNCING BALL", 24); bouncing_ball(&fb); fade_to_black(&fb, 8);
        title_card(&fb, "SPINNING RING", 24); spinning_ring(&fb); fade_to_black(&fb, 8);
        title_card(&fb, "FIREWORKS", 24); fireworks(&fb); fade_to_black(&fb, 8);
        title_card(&fb, "DIAGONAL WAVE", 24); diagonal_wave(&fb); fade_to_black(&fb, 8);
        title_card(&fb, "MULTIPLE BALLS", 24); multiple_balls(&fb); fade_to_black(&fb, 8);
        title_card(&fb, "PULSATING BLOB", 24); pulsating_blob(&fb); fade_to_black(&fb, 8);
        title_card(&fb, "ROTATING STRIPES", 24); rotating_stripes(&fb); fade_to_black(&fb, 8);
    }

    free(buffer);
    return 0;
}
