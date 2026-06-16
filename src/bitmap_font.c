#include "bitmap_font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* capture_output is defined in process_utils.c, which is included before bitmap_font.c */
extern char* capture_output(const char *cmd);

/* 5×7 bitmap font using Unicode full blocks (█) */

typedef struct {
    uint32_t codepoint;
    Glyph glyph;
} ExtendedGlyph;

static Glyph glyphs[128];
static ExtendedGlyph extended_glyphs[200];
static int extended_glyph_count = 0;
static int font_initialized = 0;

/* UTF-8 decoder: converts UTF-8 sequence to Unicode code point */
static uint32_t utf8_to_codepoint(const unsigned char *str, int *bytes_consumed) {
    unsigned char c = str[0];

    if ((c & 0x80) == 0) {
        /* ASCII: 0xxxxxxx */
        *bytes_consumed = 1;
        return c;
    } else if ((c & 0xE0) == 0xC0) {
        /* 2-byte: 110xxxxx 10xxxxxx */
        if ((str[1] & 0xC0) != 0x80) return '?';
        *bytes_consumed = 2;
        return ((c & 0x1F) << 6) | (str[1] & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        /* 3-byte: 1110xxxx 10xxxxxx 10xxxxxx */
        if ((str[1] & 0xC0) != 0x80 || (str[2] & 0xC0) != 0x80) return '?';
        *bytes_consumed = 3;
        return ((c & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
    } else if ((c & 0xF8) == 0xF0) {
        /* 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        if ((str[1] & 0xC0) != 0x80 || (str[2] & 0xC0) != 0x80 || (str[3] & 0xC0) != 0x80) return '?';
        *bytes_consumed = 4;
        return ((c & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
    }

    *bytes_consumed = 1;
    return '?';
}

/* Get glyph for a Unicode code point */
static Glyph* get_glyph(uint32_t codepoint) {
    if (codepoint < 128) {
        return &glyphs[codepoint];
    }

    /* Search extended glyphs */
    for (int i = 0; i < extended_glyph_count; i++) {
        if (extended_glyphs[i].codepoint == codepoint) {
            return &extended_glyphs[i].glyph;
        }
    }

    /* Not found, use placeholder */
    return &glyphs['?'];
}

/* Add an extended glyph for a specific Unicode code point */
static void add_glyph(uint32_t codepoint, const Glyph *g) {
    if (extended_glyph_count < 200) {
        extended_glyphs[extended_glyph_count].codepoint = codepoint;
        extended_glyphs[extended_glyph_count].glyph = *g;
        extended_glyph_count++;
    }
}

static void init_font(void) {
    if (font_initialized) return;

    /* Space */
    glyphs[' '] = (Glyph){.width = 2, .height = 7, .rows = {0, 0, 0, 0, 0, 0, 0}};

    /* A */
    glyphs['A'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001
    }};

    /* B */
    glyphs['B'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b11110, 0b10001, 0b10001, 0b10001, 0b11110
    }};

    /* C */
    glyphs['C'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110
    }};

    /* D */
    glyphs['D'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110
    }};

    /* E */
    glyphs['E'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000, 0b11111
    }};

    /* F */
    glyphs['F'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000, 0b10000
    }};

    /* G */
    glyphs['G'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10000, 0b10011, 0b10001, 0b10001, 0b01110
    }};

    /* H */
    glyphs['H'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001, 0b10001
    }};

    /* I */
    glyphs['I'] = (Glyph){.width = 3, .height = 7, .rows = {
        0b111, 0b010, 0b010, 0b010, 0b010, 0b010, 0b111
    }};

    /* J */
    glyphs['J'] = (Glyph){.width = 4, .height = 7, .rows = {
        0b0111, 0b0010, 0b0010, 0b0010, 0b1010, 0b1010, 0b0100
    }};

    /* K */
    glyphs['K'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001
    }};

    /* L */
    glyphs['L'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111
    }};

    /* M */
    glyphs['M'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b10001, 0b10001
    }};

    /* N */
    glyphs['N'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001
    }};

    /* O */
    glyphs['O'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }};

    /* P */
    glyphs['P'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000
    }};

    /* Q */
    glyphs['Q'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101
    }};

    /* R */
    glyphs['R'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001
    }};

    /* S */
    glyphs['S'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110
    }};

    /* T */
    glyphs['T'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100
    }};

    /* U */
    glyphs['U'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }};

    /* V */
    glyphs['V'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b01010, 0b00100
    }};

    /* W */
    glyphs['W'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010, 0b01010
    }};

    /* X */
    glyphs['X'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001
    }};

    /* Y */
    glyphs['Y'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100
    }};

    /* Z */
    glyphs['Z'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111
    }};

    /* 0-9 */
    glyphs['0'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110
    }};

    glyphs['1'] = (Glyph){.width = 3, .height = 7, .rows = {
        0b010, 0b110, 0b010, 0b010, 0b010, 0b010, 0b111
    }};

    glyphs['2'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111
    }};

    glyphs['3'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b00001, 0b00010, 0b00110, 0b00001, 0b10001, 0b01110
    }};

    glyphs['4'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010
    }};

    glyphs['5'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b10001, 0b01110
    }};

    glyphs['6'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110
    }};

    glyphs['7'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000
    }};

    glyphs['8'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110
    }};

    glyphs['9'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100
    }};

    /* Common punctuation - all uppercase for consistency */
    for (int i = 0; i < 128; i++) {
        if (glyphs[i].width == 0) {
            /* Unknown character - use filled block as placeholder */
            glyphs[i] = (Glyph){.width = 3, .height = 7, .rows = {
                0b111, 0b111, 0b111, 0b111, 0b111, 0b111, 0b111
            }};
        }
    }

    /* Extended glyphs for accented characters */
    /* á (0xC3 0xA1) - A with acute */
    add_glyph(0xE1, &(Glyph){.width = 5, .height = 9, .rows = {
        0b00010, 0b00100, 0b01110, 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001
    }});

    /* é (0xC3 0xA9) - E with acute */
    add_glyph(0xE9, &(Glyph){.width = 5, .height = 9, .rows = {
        0b00010, 0b00100, 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111
    }});

    /* í (0xC3 0xAD) - I with acute */
    add_glyph(0xED, &(Glyph){.width = 3, .height = 9, .rows = {
        0b010, 0b100, 0b111, 0b010, 0b010, 0b010, 0b010, 0b010, 0b111
    }});

    /* ó (0xC3 0xB3) - O with acute */
    add_glyph(0xF3, &(Glyph){.width = 5, .height = 9, .rows = {
        0b00010, 0b00100, 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }});

    /* ú (0xC3 0xBA) - U with acute */
    add_glyph(0xFA, &(Glyph){.width = 5, .height = 9, .rows = {
        0b00010, 0b00100, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }});

    /* ñ (0xC3 0xB1) - N with tilde */
    add_glyph(0xF1, &(Glyph){.width = 5, .height = 9, .rows = {
        0b01010, 0b10101, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001
    }});

    /* à (0xC3 0xA0) - A with grave */
    add_glyph(0xE0, &(Glyph){.width = 5, .height = 9, .rows = {
        0b01000, 0b00010, 0b01110, 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001
    }});

    /* è (0xC3 0xA8) - E with grave */
    add_glyph(0xE8, &(Glyph){.width = 5, .height = 9, .rows = {
        0b01000, 0b00010, 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111
    }});

    /* ì (0xC3 0xAC) - I with grave */
    add_glyph(0xEC, &(Glyph){.width = 3, .height = 9, .rows = {
        0b100, 0b010, 0b111, 0b010, 0b010, 0b010, 0b010, 0b010, 0b111
    }});

    /* ò (0xC3 0xB2) - O with grave */
    add_glyph(0xF2, &(Glyph){.width = 5, .height = 9, .rows = {
        0b01000, 0b00010, 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }});

    /* ù (0xC3 0xB9) - U with grave */
    add_glyph(0xF9, &(Glyph){.width = 5, .height = 9, .rows = {
        0b01000, 0b00010, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }});

    /* Punctuation */
    glyphs['.'] = (Glyph){.width = 2, .height = 7, .rows = {
        0b00, 0b00, 0b00, 0b00, 0b00, 0b11, 0b11
    }};

    glyphs[','] = (Glyph){.width = 2, .height = 7, .rows = {
        0b00, 0b00, 0b00, 0b00, 0b00, 0b11, 0b10
    }};

    glyphs['!'] = (Glyph){.width = 2, .height = 7, .rows = {
        0b11, 0b11, 0b11, 0b11, 0b00, 0b11, 0b11
    }};

    glyphs['?'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100
    }};

    glyphs['\''] = (Glyph){.width = 2, .height = 7, .rows = {
        0b11, 0b11, 0b00, 0b00, 0b00, 0b00, 0b00
    }};

    glyphs['"'] = (Glyph){.width = 4, .height = 7, .rows = {
        0b1010, 0b1010, 0b0000, 0b0000, 0b0000, 0b0000, 0b0000
    }};

    glyphs['-'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000
    }};

    glyphs[':'] = (Glyph){.width = 2, .height = 7, .rows = {
        0b00, 0b11, 0b11, 0b00, 0b00, 0b11, 0b11
    }};

    glyphs[';'] = (Glyph){.width = 2, .height = 7, .rows = {
        0b00, 0b11, 0b11, 0b00, 0b00, 0b11, 0b10
    }};

    glyphs['('] = (Glyph){.width = 3, .height = 7, .rows = {
        0b001, 0b010, 0b100, 0b100, 0b100, 0b010, 0b001
    }};

    glyphs[')'] = (Glyph){.width = 3, .height = 7, .rows = {
        0b100, 0b010, 0b001, 0b001, 0b001, 0b010, 0b100
    }};

    glyphs['/'] = (Glyph){.width = 5, .height = 7, .rows = {
        0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b10000, 0b10000
    }};

    /* Inverted punctuation (Spanish/Catalan) */
    /* ¡ - inverted exclamation mark */
    add_glyph(0x00A1, &(Glyph){.width = 2, .height = 7, .rows = {
        0b11, 0b11, 0b00, 0b11, 0b11, 0b11, 0b11
    }});

    /* ¿ - inverted question mark */
    add_glyph(0x00BF, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00100, 0b00000, 0b00010, 0b00100, 0b01000, 0b10001, 0b01110
    }});

    /* Cyrillic uppercase letters */
    /* А */
    add_glyph(0x0410, &(Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001
    }});
    /* Б */
    add_glyph(0x0411, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b11110, 0b10001, 0b10001, 0b10001, 0b11110
    }});
    /* В */
    add_glyph(0x0412, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b11110, 0b10001, 0b10001, 0b10001, 0b11110
    }});
    /* Г */
    add_glyph(0x0413, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000
    }});
    /* Д */
    add_glyph(0x0414, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }});
    /* Е */
    add_glyph(0x0415, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000, 0b11111
    }});
    /* Ё */
    add_glyph(0x0401, &(Glyph){.width = 5, .height = 9, .rows = {
        0b10101, 0b10101, 0b11111, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000, 0b11111
    }});
    /* Ж */
    add_glyph(0x0416, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10101, 0b10101, 0b01110, 0b00100, 0b01110, 0b10101, 0b10101
    }});
    /* З */
    add_glyph(0x0417, &(Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b00001, 0b00110, 0b10000, 0b10001, 0b01110
    }});
    /* И */
    add_glyph(0x0418, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001
    }});
    /* Й */
    add_glyph(0x0419, &(Glyph){.width = 5, .height = 9, .rows = {
        0b01010, 0b10100, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001
    }});
    /* К */
    add_glyph(0x041A, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001
    }});
    /* Л */
    add_glyph(0x041B, &(Glyph){.width = 5, .height = 7, .rows = {
        0b01100, 0b10010, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }});
    /* М */
    add_glyph(0x041C, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b10001, 0b10001
    }});
    /* Н */
    add_glyph(0x041D, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001
    }});
    /* О */
    add_glyph(0x041E, &(Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }});
    /* П */
    add_glyph(0x041F, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b10001, 0b10001, 0b11111, 0b10000, 0b10000, 0b10000
    }});
    /* Р */
    add_glyph(0x0420, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000
    }});
    /* С */
    add_glyph(0x0421, &(Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110
    }});
    /* Т */
    add_glyph(0x0422, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100
    }});
    /* У */
    add_glyph(0x0423, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00110
    }});
    /* Ф */
    add_glyph(0x0424, &(Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b11011, 0b10101, 0b11011, 0b10001, 0b01110
    }});
    /* Х */
    add_glyph(0x0425, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001
    }});
    /* Ц */
    add_glyph(0x0426, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11111
    }});
    /* Ч */
    add_glyph(0x0427, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00001
    }});
    /* Ш */
    add_glyph(0x0428, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10101, 0b10101, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111
    }});
    /* Щ */
    add_glyph(0x0429, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10101, 0b10101, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111
    }});
    /* Ъ */
    add_glyph(0x042A, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110
    }});
    /* Ы */
    add_glyph(0x042B, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10001, 0b10001, 0b10001, 0b11101, 0b10001, 0b10001, 0b11101
    }});
    /* Ь */
    add_glyph(0x042C, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110
    }});
    /* Э */
    add_glyph(0x042D, &(Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b00001, 0b00110, 0b10000, 0b10001, 0b01110
    }});
    /* Ю */
    add_glyph(0x042E, &(Glyph){.width = 5, .height = 7, .rows = {
        0b10101, 0b10101, 0b10101, 0b11101, 0b10101, 0b10101, 0b10101
    }});
    /* Я */
    add_glyph(0x042F, &(Glyph){.width = 5, .height = 7, .rows = {
        0b01110, 0b10001, 0b10001, 0b01110, 0b00100, 0b00100, 0b10100
    }});

    /* Cyrillic lowercase letters */
    /* а */
    add_glyph(0x0430, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001
    }});
    /* б */
    add_glyph(0x0431, &(Glyph){.width = 5, .height = 7, .rows = {
        0b11000, 0b10000, 0b11110, 0b10001, 0b10001, 0b10001, 0b11110
    }});
    /* в */
    add_glyph(0x0432, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b11110, 0b10001, 0b11110, 0b10001, 0b11110
    }});
    /* г */
    add_glyph(0x0433, &(Glyph){.width = 4, .height = 7, .rows = {
        0b0000, 0b0000, 0b1111, 0b1000, 0b1000, 0b1000, 0b1000
    }});
    /* д */
    add_glyph(0x0434, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00100, 0b01010, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
    }});
    /* е */
    add_glyph(0x0435, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b01110, 0b10001, 0b11110, 0b10000, 0b01110
    }});
    /* ё */
    add_glyph(0x0451, &(Glyph){.width = 5, .height = 9, .rows = {
        0b10101, 0b10101, 0b01110, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000, 0b01110
    }});
    /* ж */
    add_glyph(0x0436, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10101, 0b01110, 0b00100, 0b01110, 0b10101
    }});
    /* з */
    add_glyph(0x0437, &(Glyph){.width = 4, .height = 7, .rows = {
        0b0000, 0b0000, 0b0110, 0b1001, 0b0110, 0b1001, 0b0110
    }});
    /* и */
    add_glyph(0x0438, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001
    }});
    /* й */
    add_glyph(0x0439, &(Glyph){.width = 5, .height = 9, .rows = {
        0b01010, 0b10100, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001
    }});
    /* к */
    add_glyph(0x043A, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10001, 0b10010, 0b11000, 0b10010, 0b10001
    }});
    /* л */
    add_glyph(0x043B, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00100, 0b00100, 0b01010, 0b01010, 0b01010, 0b01010, 0b10001
    }});
    /* м */
    add_glyph(0x043C, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10001, 0b11011, 0b10101, 0b10001, 0b10001
    }});
    /* н */
    add_glyph(0x043D, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001
    }});
    /* о */
    add_glyph(0x043E, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110
    }});
    /* п */
    add_glyph(0x043F, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b11111, 0b10001, 0b10001, 0b10001, 0b10001
    }});
    /* р */
    add_glyph(0x0440, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b11110, 0b10001, 0b11110, 0b10000, 0b10000
    }});
    /* с */
    add_glyph(0x0441, &(Glyph){.width = 4, .height = 7, .rows = {
        0b0000, 0b0000, 0b0110, 0b1000, 0b1000, 0b1000, 0b0110
    }});
    /* т */
    add_glyph(0x0442, &(Glyph){.width = 4, .height = 7, .rows = {
        0b0000, 0b0000, 0b1111, 0b0100, 0b0100, 0b0100, 0b0100
    }});
    /* у */
    add_glyph(0x0443, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10001, 0b10001, 0b01111, 0b00001, 0b00110
    }});
    /* ф */
    add_glyph(0x0444, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00100, 0b00100, 0b01110, 0b10101, 0b01110, 0b00100, 0b00100
    }});
    /* х */
    add_glyph(0x0445, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001
    }});
    /* ц */
    add_glyph(0x0446, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00100, 0b00100, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111
    }});
    /* ч */
    add_glyph(0x0447, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001
    }});
    /* ш */
    add_glyph(0x0448, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111
    }});
    /* щ */
    add_glyph(0x0449, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00100, 0b00100, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111
    }});
    /* ъ */
    add_glyph(0x044A, &(Glyph){.width = 4, .height = 7, .rows = {
        0b0000, 0b0000, 0b1110, 0b1001, 0b1110, 0b1001, 0b1110
    }});
    /* ы */
    add_glyph(0x044B, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10001, 0b10001, 0b11101, 0b10001, 0b10001
    }});
    /* ь */
    add_glyph(0x044C, &(Glyph){.width = 4, .height = 7, .rows = {
        0b0000, 0b0000, 0b1110, 0b1001, 0b1110, 0b1001, 0b1110
    }});
    /* э */
    add_glyph(0x044D, &(Glyph){.width = 4, .height = 7, .rows = {
        0b0000, 0b0000, 0b0110, 0b1001, 0b0110, 0b1001, 0b0110
    }});
    /* ю */
    add_glyph(0x044E, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b10101, 0b10101, 0b11101, 0b10101, 0b10101
    }});
    /* я */
    add_glyph(0x044F, &(Glyph){.width = 5, .height = 7, .rows = {
        0b00000, 0b00000, 0b01110, 0b10001, 0b01110, 0b00100, 0b10100
    }});

    font_initialized = 1;
}

void render_bitmap_text(const char *text) {
    if (!text || !*text) return;

    init_font();

    /* Convert to uppercase and decode UTF-8 */
    int text_len = strlen(text);
    uint32_t codepoints[256];
    int codepoint_count = 0;

    for (int i = 0; i < text_len && codepoint_count < 255; ) {
        int bytes_consumed = 0;
        uint32_t cp = utf8_to_codepoint((const unsigned char *)&text[i], &bytes_consumed);

        /* Convert ASCII to uppercase */
        if (cp >= 'a' && cp <= 'z') {
            cp = cp - 'a' + 'A';
        }

        codepoints[codepoint_count++] = cp;
        i += bytes_consumed;
    }
    codepoints[codepoint_count] = 0;

    /* Calculate total width needed */
    int total_width = 0;
    int max_height = 9;  /* Always use full height for consistent vertical alignment */
    for (int i = 0; i < codepoint_count; i++) {
        Glyph *g = get_glyph(codepoints[i]);
        total_width += g->width + 1; /* +1 for space between chars */
    }
    if (total_width > 0) total_width--; /* Remove last space */

    /* Get terminal width */
    int term_width = 80;
    char *width_str = capture_output("tput cols 2>/dev/null");
    if (width_str) {
        term_width = atoi(width_str);
        free(width_str);
        if (term_width < 40) term_width = 80;
    }

    /* Calculate horizontal padding */
    int horiz_padding = (term_width - total_width) / 2;
    if (horiz_padding < 0) horiz_padding = 0;

    /* Render each row */
    for (int row = 0; row < max_height; row++) {
        /* Print leading spaces */
        for (int i = 0; i < horiz_padding; i++) printf(" ");

        for (int i = 0; i < codepoint_count; i++) {
            Glyph *g = get_glyph(codepoints[i]);
            int v_offset = max_height - g->height;  /* Vertical offset to align baseline */

            if (row < v_offset) {
                /* Empty rows above short glyphs */
                for (int bit = 0; bit < g->width; bit++) {
                    printf(" ");
                }
            } else if (row < v_offset + g->height) {
                /* Glyph content */
                uint8_t byte = g->rows[row - v_offset];
                for (int bit = 0; bit < g->width; bit++) {
                    if (byte & (1 << (g->width - 1 - bit))) {
                        printf("█");
                    } else {
                        printf(" ");
                    }
                }
            } else {
                /* Should not happen */
                for (int bit = 0; bit < g->width; bit++) {
                    printf(" ");
                }
            }
            printf(" ");
        }
        printf("\n");
    }
}
