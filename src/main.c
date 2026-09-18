#include <conio.h>
#include <i86.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

#define COLOR_BLACK       0
#define COLOR_BLUE        1
#define COLOR_GREEN       2
#define COLOR_CYAN        3
#define COLOR_RED         4
#define COLOR_MAGENTA     5
#define COLOR_BROWN       6
#define COLOR_LIGHT_GRAY  7
#define COLOR_DARK_GRAY   8
#define COLOR_LIGHT_BLUE  9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN  11
#define COLOR_LIGHT_RED   12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW      14
#define COLOR_WHITE       15

// VGA Mode 13h linear frame buffer pointer
unsigned char far *vga_memory = (unsigned char far *)0xA0000000L;

// Off-screen back buffer to handle drawing and partial updates
unsigned char far *back_buffer;

typedef struct {
    char rank;
    char suit;
} Card;

void draw_char(int x, int y, char character, unsigned char color, int scale);

// Switch to VGA Mode 13h (320x200, 256 colors)
void set_mode_13h(void) {
    union REGS regs;
    regs.w.ax = 0x0013;
    int86(0x10, &regs, &regs);
}

// Restore standard text mode (Mode 3) on exit
void set_text_mode(void) {
    union REGS regs;
    regs.w.ax = 0x0003;
    int86(0x10, &regs, &regs);
}

// Draw text at a specific position in the back buffer
void draw_text(int x, int y, char *text, unsigned char color) {
    int i;
    for (i = 0; text[i] != '\0'; i++) {
        draw_char(x + i * 6, y, text[i], color, 1);
    }
}

// Draw a filled rectangle into the back buffer
void draw_rect(int x1, int y1, int x2, int y2, unsigned char color) {
    int x, y;
    for (y = y1; y <= y2; y++) {
        if (y >= 0 && y < SCREEN_HEIGHT) {
            for (x = x1; x <= x2; x++) {
                if (x >= 0 && x < SCREEN_WIDTH) {
                    back_buffer[y * SCREEN_WIDTH + x] = color;
                }
            }
        }
    }
}

// Partial Update: Copy only a specific rectangular region from back buffer to screen
void update_region(int x1, int y1, int x2, int y2) {
    int x, y;
    
    // Bounds clipping
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;
    if (y2 >= SCREEN_HEIGHT) y2 = SCREEN_HEIGHT - 1;

    for (y = y1; y <= y2; y++) {
        int row_offset = y * SCREEN_WIDTH;
        for (x = x1; x <= x2; x++) {
            vga_memory[row_offset + x] = back_buffer[row_offset + x];
        }
    }
}

// Draw one character using a small 5x5 bitmap font.
void draw_char(int x, int y, char character, unsigned char color, int scale) {
    unsigned char rows[5];
    int row, column;

    rows[0] = 0;
    rows[1] = 0;
    rows[2] = 0;
    rows[3] = 0;
    rows[4] = 0;

    switch (character) {
        case '.': rows[4] = 4; break;
        case ':': rows[1] = 4; rows[3] = 4; break;
        case '0': rows[0] = 14; rows[1] = 17; rows[2] = 17; rows[3] = 17; rows[4] = 14; break;
        case '1': rows[0] = 4; rows[1] = 12; rows[2] = 4; rows[3] = 4; rows[4] = 14; break;
        case '2': rows[0] = 14; rows[1] = 17; rows[2] = 2; rows[3] = 4; rows[4] = 31; break;
        case '3': rows[0] = 30; rows[1] = 1; rows[2] = 14; rows[3] = 1; rows[4] = 30; break;
        case '4': rows[0] = 18; rows[1] = 18; rows[2] = 31; rows[3] = 2; rows[4] = 2; break;
        case '5': rows[0] = 31; rows[1] = 16; rows[2] = 30; rows[3] = 1; rows[4] = 30; break;
        case '6': rows[0] = 14; rows[1] = 16; rows[2] = 30; rows[3] = 17; rows[4] = 14; break;
        case '7': rows[0] = 31; rows[1] = 1; rows[2] = 2; rows[3] = 4; rows[4] = 4; break;
        case '8': rows[0] = 14; rows[1] = 17; rows[2] = 14; rows[3] = 17; rows[4] = 14; break;
        case '9': rows[0] = 14; rows[1] = 17; rows[2] = 15; rows[3] = 1; rows[4] = 14; break;
        case 'A': rows[0] = 14; rows[1] = 17; rows[2] = 31; rows[3] = 17; rows[4] = 17; break;
        case 'B': rows[0] = 30; rows[1] = 17; rows[2] = 30; rows[3] = 17; rows[4] = 30; break;
        case 'C': rows[0] = 14; rows[1] = 17; rows[2] = 16; rows[3] = 17; rows[4] = 14; break;
        case 'D': rows[0] = 30; rows[1] = 17; rows[2] = 17; rows[3] = 17; rows[4] = 30; break;
        case 'E': rows[0] = 31; rows[1] = 16; rows[2] = 30; rows[3] = 16; rows[4] = 31; break;
        case 'F': rows[0] = 31; rows[1] = 16; rows[2] = 30; rows[3] = 16; rows[4] = 16; break;
        case 'G': rows[0] = 14; rows[1] = 17; rows[2] = 23; rows[3] = 17; rows[4] = 15; break;
        case 'H': rows[0] = 17; rows[1] = 17; rows[2] = 31; rows[3] = 17; rows[4] = 17; break;
        case 'I': rows[0] = 31; rows[1] = 4; rows[2] = 4; rows[3] = 4; rows[4] = 31; break;
        case 'J': rows[0] = 7; rows[1] = 2; rows[2] = 2; rows[3] = 18; rows[4] = 12; break;
        case 'K': rows[0] = 17; rows[1] = 18; rows[2] = 28; rows[3] = 18; rows[4] = 17; break;
        case 'L': rows[0] = 16; rows[1] = 16; rows[2] = 16; rows[3] = 16; rows[4] = 31; break;
        case 'M': rows[0] = 17; rows[1] = 27; rows[2] = 21; rows[3] = 17; rows[4] = 17; break;
        case 'N': rows[0] = 17; rows[1] = 25; rows[2] = 21; rows[3] = 19; rows[4] = 17; break;
        case 'O': rows[0] = 14; rows[1] = 17; rows[2] = 17; rows[3] = 17; rows[4] = 14; break;
        case 'P': rows[0] = 30; rows[1] = 17; rows[2] = 30; rows[3] = 16; rows[4] = 16; break;
        case 'Q': rows[0] = 14; rows[1] = 17; rows[2] = 17; rows[3] = 19; rows[4] = 15; break;
        case 'R': rows[0] = 30; rows[1] = 17; rows[2] = 30; rows[3] = 20; rows[4] = 18; break;
        case 'S': rows[0] = 15; rows[1] = 16; rows[2] = 14; rows[3] = 1; rows[4] = 30; break;
        case 'T': rows[0] = 31; rows[1] = 4; rows[2] = 4; rows[3] = 4; rows[4] = 4; break;
        case 'U': rows[0] = 17; rows[1] = 17; rows[2] = 17; rows[3] = 17; rows[4] = 14; break;
        case 'V': rows[0] = 17; rows[1] = 17; rows[2] = 17; rows[3] = 10; rows[4] = 4; break;
        case 'W': rows[0] = 17; rows[1] = 17; rows[2] = 21; rows[3] = 21; rows[4] = 10; break;
        case 'X': rows[0] = 17; rows[1] = 10; rows[2] = 4; rows[3] = 10; rows[4] = 17; break;
        case 'Y': rows[0] = 17; rows[1] = 10; rows[2] = 4; rows[3] = 4; rows[4] = 4; break;
        case 'Z': rows[0] = 31; rows[1] = 2; rows[2] = 4; rows[3] = 8; rows[4] = 31; break;
    }

    for (row = 0; row < 5; row++) {
        for (column = 0; column < 5; column++) {
            if (rows[row] & (1 << (4 - column))) {
                draw_rect(x + column * scale, y + row * scale,
                    x + column * scale + scale - 1,
                    y + row * scale + scale - 1, color);
            }
        }
    }
}

void draw_suit(int x, int y, char suit, unsigned char color) {
    static char *heart[15] = {
        "...............",
        "...............",
        "...###...###...", 
        "..#####.#####..",
        ".#############.",
        ".#############.", 
        "..###########..", 
        "..###########..",
        "...#########...", 
        "....#######....", 
        ".....#####.....",
        "......###......", 
        ".......#.......", 
        "...............",
        "..............."
    };
    static char *diamond[15] = {
        "...............", 
        ".......#.......", 
        "......###......",
        ".....#####.....", 
        "....#######....", 
        "...#########...",
        "..###########..", 
        ".#############.", 
        "..###########..",
        "...#########...", 
        "....#######....", 
        ".....#####.....",
        "......###......", 
        ".......#.......", 
        "..............."
    };
    static char *club[15] = {
        "...............",
        "......###......",
        ".....#####.....",
        ".....#####.....",
        ".....#####.....",
        "..###.###.###..",
        ".#############.",
        ".#############.",
        ".#############.",
        "..###..#..###..",
        "......###......",
        ".....#####.....",
        "..............."
    };
    static char *spade[15] = {
        "...............",
        ".......#.......",
        "......###......",
        ".....#####.....",
        "....#######....",
        "...#########...",
        "..###########..",
        ".#############.",
        ".#############.",
        "..###..#..###..",
        "......###......",
        ".....#####.....",
        "..............."
    };
    char **pattern;
    int row, column;

    pattern = heart;
    if (suit == 'D') pattern = diamond;
    if (suit == 'C') pattern = club;
    if (suit == 'S') pattern = spade;

    for (row = 0; row < 15; row++) {
        for (column = 0; column < 15; column++) {
            if (pattern[row][column] == '#') {
                draw_rect(x + column * 2, y + row * 2,
                    x + column * 2 + 1, y + row * 2 + 1, color);
            }
        }
    }
}

void draw_card(Card *card, int x, int y) {
    unsigned char color;

    color = (card->suit == 'H' || card->suit == 'D') ? COLOR_RED : COLOR_BLACK;
    draw_rect(x, y, x + 49, y + 79, COLOR_BLACK);
    draw_rect(x + 2, y + 2, x + 47, y + 77, COLOR_WHITE);
    if (card->rank == 'T') {
        draw_char(x + 5, y + 7, '1', color, 2);
        draw_char(x + 15, y + 7, '0', color, 2);
    } else {
        draw_char(x + 7, y + 7, card->rank, color, 2);
    }
    draw_suit(x + 10, y + 27, card->suit, color);
}

void shuffle_deck(Card *deck) {
    int i, swap_index;
    Card temporary;

    for (i = 51; i > 0; i--) {
        swap_index = rand() % (i + 1);
        temporary = deck[i];
        deck[i] = deck[swap_index];
        deck[swap_index] = temporary;
    }
}

int initialize_graphics(int credits, int bet) {
    char credits_text[7];

    back_buffer = (unsigned char far *)halloc(
        (unsigned long)SCREEN_WIDTH * SCREEN_HEIGHT, 1);
    if (back_buffer == 0) {
        return 0;
    }

    // initialize graphics mode and set back buffer to blue
    set_mode_13h();
    _fmemset(back_buffer, COLOR_BLUE, SCREEN_WIDTH * SCREEN_HEIGHT);

    draw_text(5, 5,    "ROYAL FLUSH..........4000", COLOR_YELLOW);
    draw_text(5, 12,   "STRAIGHT FLUSH........250", COLOR_YELLOW);
    draw_text(5, 19,   "FOUR OF A KIND........125", COLOR_YELLOW);
    draw_text(5, 26,   "FULL HOUSE.............45", COLOR_YELLOW);
    draw_text(5, 33,   "FLUSH..................30", COLOR_YELLOW);
    draw_text(165, 5,  "STRAIGHT...............20", COLOR_YELLOW);
    draw_text(165, 12, "THREE OF A KIND........15", COLOR_YELLOW);
    draw_text(165, 19, "TWO PAIR...............10", COLOR_YELLOW);
    draw_text(165, 26, "JACKS OR BETTER.........5", COLOR_YELLOW);

    draw_rect(0, 185, 320, 200, COLOR_LIGHT_GRAY);

    draw_text(5, 190, "CREDITS: ", COLOR_YELLOW);
    sprintf(credits_text, "%d", credits);
    draw_text(53, 190, credits_text, COLOR_YELLOW);
    draw_text(285, 190, "BET: ", COLOR_YELLOW);
    sprintf(credits_text, "%d", bet);
    draw_text(310, 190, credits_text, COLOR_YELLOW);
    return 1;
}

int main(void) {
    int i, rank, suit;
    int credits = 1000;
    int bet = 5;
    Card deck[52];
    char ranks[13] = "A23456789TJQK";
    char suits[4] = "HDCS";
    
    if (!initialize_graphics(credits, bet)) {
        return 1;
    }

    i = 0;
    for (suit = 0; suit < 4; suit++) {
        for (rank = 0; rank < 13; rank++) {
            deck[i].rank = ranks[rank];
            deck[i].suit = suits[suit];
            i++;
        }
    }

    srand((unsigned)time(NULL));
    shuffle_deck(deck);

    for (i = 0; i < 5; i++) {
        draw_card(&deck[i], 15 + i * 60, 90);
    }

    update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    // Wait for a keypress to inspect the result
    getch();

    // cleanup after program closes
    set_text_mode();
    hfree(back_buffer);
    return 0;
}
