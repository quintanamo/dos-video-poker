#include <conio.h>
#include <dos.h>
#include <i86.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define CARD_COUNT 5
#define CARD_Y 95

#define STATE_READY 0
#define STATE_HOLD 1
#define STATE_RESULT 2
#define HAND_NO_WIN 9
#define HAND_GAME_OVER 10

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

char *hand_name(int hand_type);

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
        case ',': rows[4] = 4; break;
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

void draw_pay_table(int bet) {
    int royal, straight_flush, four_kind, full_house;
    int flush, straight, three_kind, two_pair, jacks_better;
    char payout_line[26];

    if (bet == 1) royal = 250;
    else if (bet == 2) royal = 500;
    else if (bet == 3) royal = 750;
    else if (bet == 4) royal = 1000;
    else royal = 4000;
    straight_flush = 50 * bet;
    four_kind = 25 * bet;
    full_house = 9 * bet;
    flush = 6 * bet;
    straight = 4 * bet;
    three_kind = 3 * bet;
    two_pair = 2 * bet;
    jacks_better = bet;

    draw_rect(0, 0, SCREEN_WIDTH - 1, 43, COLOR_BLUE);
    sprintf(payout_line, "ROYAL FLUSH..........%4d", royal);
    draw_text(5, 5, payout_line, COLOR_YELLOW);
    sprintf(payout_line, "STRAIGHT FLUSH.......%4d", straight_flush);
    draw_text(5, 12, payout_line, COLOR_YELLOW);
    sprintf(payout_line, "FOUR OF A KIND.......%4d", four_kind);
    draw_text(5, 19, payout_line, COLOR_YELLOW);
    sprintf(payout_line, "FULL HOUSE...........%4d", full_house);
    draw_text(5, 26, payout_line, COLOR_YELLOW);
    sprintf(payout_line, "FLUSH................%4d", flush);
    draw_text(5, 33, payout_line, COLOR_YELLOW);
    sprintf(payout_line, "STRAIGHT.............%4d", straight);
    draw_text(165, 5, payout_line, COLOR_YELLOW);
    sprintf(payout_line, "THREE OF A KIND......%4d", three_kind);
    draw_text(165, 12, payout_line, COLOR_YELLOW);
    sprintf(payout_line, "TWO PAIR.............%4d", two_pair);
    draw_text(165, 19, payout_line, COLOR_YELLOW);
    sprintf(payout_line, "JACKS OR BETTER......%4d", jacks_better);
    draw_text(165, 26, payout_line, COLOR_YELLOW);
}

void draw_status(int credits, int bet) {
    char value_text[7];

    draw_rect(0, 185, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, COLOR_LIGHT_GRAY);
    draw_text(5, 190, "CREDITS:", COLOR_YELLOW);
    sprintf(value_text, "%d", credits);
    draw_text(59, 190, value_text, COLOR_YELLOW);
    draw_text(285, 190, "BET:", COLOR_YELLOW);
    sprintf(value_text, "%d", bet);
    draw_text(310, 190, value_text, COLOR_YELLOW);
}

void clear_play_area(void) {
    draw_rect(0, 44, SCREEN_WIDTH - 1, 184, COLOR_BLUE);
}

void draw_instructions(void) {
    clear_play_area();
    draw_rect(30, 65, 289, 170, COLOR_BLACK);
    draw_rect(33, 68, 286, 167, COLOR_WHITE);
    draw_text(72, 82, "PRESS SPACE TO START DEALING", COLOR_BLACK);
    draw_text(52, 104, "USE 1, 2, 3, 4, AND 5 TO HOLD CARDS", COLOR_BLACK);
    draw_text(67, 126, "USE UP AND DOWN TO CHANGE BET", COLOR_BLACK);
}

void draw_hand_label(int hand_type) {
    char *name;
    int x;

    name = hand_name(hand_type);
    x = (SCREEN_WIDTH - (int)strlen(name) * 6) / 2;
    draw_text(x, 52, name, COLOR_YELLOW);
}

void draw_hand(Card *hand, int *held, int visible, int hand_type) {
    int i;

    clear_play_area();
    if (hand_type >= 0) {
        draw_hand_label(hand_type);
    }
    for (i = 0; i < visible; i++) {
        if (held[i]) {
            draw_text(15 + i * 60 + 13, 84, "HOLD", COLOR_YELLOW);
        }
        draw_card(&hand[i], 15 + i * 60, CARD_Y);
    }
}

int card_value(Card *card) {
    if (card->rank == 'A') return 14;
    if (card->rank == 'T') return 10;
    if (card->rank == 'J') return 11;
    if (card->rank == 'Q') return 12;
    if (card->rank == 'K') return 13;
    return card->rank - '0';
}

int is_straight(Card *hand) {
    int values[CARD_COUNT];
    int i, j, temporary;

    for (i = 0; i < CARD_COUNT; i++) {
        values[i] = card_value(&hand[i]);
    }
    for (i = 0; i < CARD_COUNT - 1; i++) {
        for (j = i + 1; j < CARD_COUNT; j++) {
            if (values[j] < values[i]) {
                temporary = values[i];
                values[i] = values[j];
                values[j] = temporary;
            }
        }
    }
    if (values[0] == 2 && values[1] == 3 && values[2] == 4 &&
        values[3] == 5 && values[4] == 14) {
        return 1;
    }
    for (i = 1; i < CARD_COUNT; i++) {
        if (values[i] != values[i - 1] + 1) return 0;
    }
    return 1;
}

int evaluate_hand(Card *hand) {
    int counts[15];
    int i, pair_count, three_count, four_count, flush;
    int straight;

    for (i = 0; i < 15; i++) counts[i] = 0;
    for (i = 0; i < CARD_COUNT; i++) counts[card_value(&hand[i])]++;

    pair_count = 0;
    three_count = 0;
    four_count = 0;
    for (i = 2; i <= 14; i++) {
        if (counts[i] == 2) pair_count++;
        if (counts[i] == 3) three_count++;
        if (counts[i] == 4) four_count++;
    }
    flush = 1;
    for (i = 1; i < CARD_COUNT; i++) {
        if (hand[i].suit != hand[0].suit) flush = 0;
    }
    straight = is_straight(hand);

    if (flush && straight && counts[10] && counts[11] && counts[12] &&
        counts[13] && counts[14]) return 0;
    if (flush && straight) return 1;
    if (four_count) return 2;
    if (three_count && pair_count) return 3;
    if (flush) return 4;
    if (straight) return 5;
    if (three_count) return 6;
    if (pair_count >= 2) return 7;
    if (pair_count == 1) {
        for (i = 11; i <= 14; i++) {
            if (counts[i] == 2) return 8;
        }
    }
    return HAND_NO_WIN;
}

char *hand_name(int hand_type) {
    static char *names[11] = {
        "ROYAL FLUSH", "STRAIGHT FLUSH", "FOUR OF A KIND",
        "FULL HOUSE", "FLUSH", "STRAIGHT", "THREE OF A KIND",
        "TWO PAIR", "JACKS OR BETTER", "", "GAME OVER"
    };
    return names[hand_type];
}

int payout_for_hand(int hand_type, int bet) {
    static int royal_payouts[5] = { 250, 500, 750, 1000, 4000 };
    static int payouts[9] = { 800, 50, 25, 9, 6, 4, 3, 2, 1 };

    if (hand_type == 0) return royal_payouts[bet - 1];
    if (hand_type >= 0 && hand_type < 9) return payouts[hand_type] * bet;
    return 0;
}

int read_key(void) {
    int key;

    key = getch();
    if (key == 0 || key == 224) return getch();
    return key;
}

int initialize_graphics(void) {

    back_buffer = (unsigned char far *)halloc(
        (unsigned long)SCREEN_WIDTH * SCREEN_HEIGHT, 1);
    if (back_buffer == 0) {
        return 0;
    }

    // initialize graphics mode and set back buffer to blue
    set_mode_13h();
    _fmemset(back_buffer, COLOR_LIGHT_BLUE, SCREEN_WIDTH * SCREEN_HEIGHT);

    return 1;
}

int main(void) {
    int i, rank, suit, key, state, visible;
    int hand_type, payout, card_index;
    int credits = 1000;
    int bet = 5;
    int held[CARD_COUNT];
    Card deck[52];
    Card hand[CARD_COUNT];
    char ranks[13] = "A23456789TJQK";
    char suits[4] = "HDCS";

    if (!initialize_graphics()) {
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

    draw_pay_table(bet);
    draw_instructions();
    draw_status(credits, bet);
    update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    state = STATE_READY;
    hand_type = HAND_NO_WIN;
    visible = 0;
    while (credits > 0) {
        key = read_key();

        if (state == STATE_READY || state == STATE_RESULT) {
            if (key == 72 || key == 80) {
                if (key == 72 && bet < 5) bet++;
                if (key == 80 && bet > 1) bet--;
                draw_pay_table(bet);
                draw_status(credits, bet);
                update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
            } else if (key == ' ' && credits > 0) {
                if (bet > credits) bet = credits;
                credits -= bet;
                draw_pay_table(bet);
                draw_status(credits, bet);
                clear_play_area();
                update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

                srand((unsigned)time(NULL));
                shuffle_deck(deck);
                for (i = 0; i < CARD_COUNT; i++) held[i] = 0;
                for (i = 0; i < CARD_COUNT; i++) {
                    hand[i] = deck[i];
                    visible = i + 1;
                    draw_hand(hand, held, visible, -1);
                    draw_status(credits, bet);
                    update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
                    delay(250);
                }
                hand_type = evaluate_hand(hand);
                draw_hand(hand, held, CARD_COUNT, hand_type);
                draw_status(credits, bet);
                update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
                state = STATE_HOLD;
            }
        } else if (state == STATE_HOLD) {
            if (key >= '1' && key <= '5') {
                i = key - '1';
                held[i] = !held[i];
                hand_type = evaluate_hand(hand);
                draw_hand(hand, held, CARD_COUNT, hand_type);
                draw_status(credits, bet);
                update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
            } else if (key == ' ') {
                card_index = CARD_COUNT;
                clear_play_area();
                for (i = 0; i < CARD_COUNT; i++) {
                    if (held[i]) {
                        draw_text(15 + i * 60 + 13, 84, "HOLD", COLOR_YELLOW);
                        draw_card(&hand[i], 15 + i * 60, CARD_Y);
                    }
                }
                update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
                for (i = 0; i < CARD_COUNT; i++) {
                    if (!held[i]) {
                        hand[i] = deck[card_index++];
                        draw_card(&hand[i], 15 + i * 60, CARD_Y);
                        update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
                        delay(250);
                    }
                }
                hand_type = evaluate_hand(hand);
                payout = payout_for_hand(hand_type, bet);
                credits += payout;
                if (hand_type == HAND_NO_WIN || credits == 0) {
                    draw_hand(hand, held, CARD_COUNT, HAND_GAME_OVER);
                } else {
                    draw_hand(hand, held, CARD_COUNT, hand_type);
                }
                draw_status(credits, bet);
                update_region(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
                state = STATE_RESULT;
            }
        }
    }

    if (credits == 0) {
        getch();
    }

    // cleanup after program closes
    set_text_mode();
    hfree(back_buffer);
    return 0;
}
