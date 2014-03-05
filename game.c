#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

typedef struct part part;
typedef struct key key;

struct part {
    int x, y, w, h; // x, y, width, height, speed.
    int color;
    float speed;
};

struct key {
    int key;
    void (*function)(void);
};

static void setup_game();
static void start();
static void render();
static void update();
static void left();
static void right();
static void quit();
static void replace_block(struct part *block);
static int randr(int min, int max);
static int collides(struct part a, struct part b);

int running;

struct part player;

float max_speed; // Termianal velocity of the player.
float speed;
struct part *blocks;

int score;
int next_level;

struct key keys[] = {
    {68, left},
    {67, right},

    {'h', left},
    {'l', right},
   
    {'q', quit},
    {0, NULL},
};

#include "draw.c"
#include "input.c"

void render() {
    int i;

    draw_square_empty(player.color, player.x, player.y, player.w, player.h);
    draw_square(player.color, player.x + 8, player.y + 8, 16, 16);

    for (i = 0; blocks[i].color != -1; i++) {
        if (blocks[i].color == -2) continue; // If I don't want the block but It's not the last one.

        draw_square(blocks[i].color, blocks[i].x, blocks[i].y, blocks[i].w, blocks[i].h);
    }

    draw_score(score);
}

void update() {
    int i;

    player.y += player.speed;
    if (player.speed < max_speed)
        player.speed += 0.5f;

    if (player.y + player.h > height)
        running = 0;

    if (player.x < 0)
        player.x = width;
    if (player.x > width)
        player.x = 0;

    for (i = 0; blocks[i].color != -1; i++) {
        if (blocks[i].color == -2) continue; // If I don't want the block but It's not the last one.

        blocks[i].y -= speed; 

        if (blocks[i].y + blocks[i].h < 0) {
            if (score > next_level) {
                next_level *= 2;
                blocks[i].color = -2;
            } else {
                replace_block(&blocks[i]);
            }
        }

        if (collides(player, blocks[i])) {
            player.speed = -(player.speed * 0.3f + speed * 0.8f + 4);
            player.y = blocks[i].y - player.h;
        }
    }

    speed += 0.01f;

    score++;
}

void start() {
    setup_game();

    clear_all();

    draw_border();

    running = 1;

    pthread_t pth;
    pthread_create(&pth, NULL, input, "get input");

    while (running) {
        update();
        render();

        usleep(30000);
        clear();
    }

    pthread_cancel(pth);

    clear_all();

    printf("Score: %i\n", score);
}

void left() {
    player.x -= 6;
}

void right() {
    player.x += 6;
}

void quit() {
    running = 0;
}

void setup_game() {
    int i;

    player.w = 32;
    player.h = 32;
    player.x = width / 2 - player.w / 2;
    player.y = player.h;
    player.speed = 0;
    player.color = 0xff00ffff;

    speed = 4;

    blocks = malloc(sizeof(part) * 11);
    for (i = 0; i < 10; i++) {
        replace_block(&blocks[i]);
    }

    blocks[i].color = -1;

    max_speed = blocks[0].h - 0.4f;

    score = 0;
    next_level = 100;
}

void replace_block(struct part *block) {
    (*block).x = randr(0, width);
    (*block).y = randr(0, height) + height;
    (*block).w = 32;
    (*block).h = 32;
    (*block).color = 0x00ffffff;
}

int randr(int min, int max) {
    return (rand() % (max - min) + min);
}

int collides(struct part a, struct part b) {
    if ((a.x >= b.x && a.x <= b.x + b.w) || (b.x >= a.x && b.x <= a.x + a.w))
        if ((a.y >= b.y && a.y <= b.y + b.h) || (b.y >= a.y && b.y <= a.y + a.h))
            return 1;
    return 0;
}

int main() {
    if (setup_fb())
        return 1;

    start();

    packup_fb();

    return 0;
}

