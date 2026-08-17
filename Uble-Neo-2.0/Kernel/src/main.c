#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "font.h"
#include "fs.h"
#include "io.h"
#include "PCI.h"
#include "E1000.h"
#include "memory.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

typedef signed char     int8_t_local;
typedef unsigned char   uint8_t_local;
typedef unsigned short  uint16_t_local;
typedef unsigned int    uint32_t_local;
typedef unsigned long long uint64_t_local;

typedef int bool_local;

#define true  1
#define false 0

static struct limine_framebuffer *kernel_framebuffer = NULL;

static uint32_t framebuffer_width  = 0;
static uint32_t framebuffer_height = 0;
static uint32_t framebuffer_pitch  = 0;
static uint32_t framebuffer_bpp    = 0;

static uint8_t framebuffer_red_shift   = 16;
static uint8_t framebuffer_green_shift = 8;
static uint8_t framebuffer_blue_shift  = 0;

/* ================================================================
 * TERMINAL
 * ================================================================ */
#define COLOR 0x0F

#define MAX_TERMINAL_COLUMNS 512
#define MAX_TERMINAL_ROWS    256

#define EDITOR_MAX_LINES 256
#define EDITOR_MAX_LINE_SIZE MAX_TERMINAL_COLUMNS

static int EDITOR_LINE_SIZE = 80;
static char editor_lines[EDITOR_MAX_LINES][EDITOR_MAX_LINE_SIZE];

static int TERMINAL_COLUMNS = 80;
static int TERMINAL_ROWS = 25;
static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t active_color = COLOR;
static char terminal_chars[MAX_TERMINAL_ROWS][MAX_TERMINAL_COLUMNS];
static uint8_t terminal_colors[MAX_TERMINAL_ROWS][MAX_TERMINAL_COLUMNS];

static void terminal_update_size(void) {
    TERMINAL_COLUMNS = framebuffer_width / FONT_WIDTH;
    TERMINAL_ROWS = framebuffer_height / FONT_HEIGHT;
    if (TERMINAL_COLUMNS > MAX_TERMINAL_COLUMNS)
        TERMINAL_COLUMNS = MAX_TERMINAL_COLUMNS;
    if (TERMINAL_ROWS > MAX_TERMINAL_ROWS)
        TERMINAL_ROWS = MAX_TERMINAL_ROWS;
    if (TERMINAL_COLUMNS == 0)
        TERMINAL_COLUMNS = 1;
    if (TERMINAL_ROWS == 0)
        TERMINAL_ROWS = 1;
    EDITOR_LINE_SIZE = TERMINAL_COLUMNS;
}

/* ================================================================
 * COLOR HELPERS
 * ================================================================ */

static uint32_t rgb_to_pixel(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << framebuffer_red_shift) | ((uint32_t)g << framebuffer_green_shift) | ((uint32_t)b << framebuffer_blue_shift);
}

static uint32_t ansi_to_rgb(uint8_t color) {
    uint8_t base = color & 0x07;
    bool bright = (color & 0x08) != 0;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    switch (base) {
        case 0:
            r = g = b = 0;
            break;
        case 1:
            r = 170;
            break;
        case 2:
            g = 170;
            break;
        case 3:
            r = g = 170;
            break;
        case 4:
            b = 170;
            break;
        case 5:
            r = b = 170;
            break;
        case 6:
            g = b = 170;
            break;
        case 7:
            r = g = b = 210;
            break;
    }
    if (bright) {
        if (r) r = 255;
        if (g) g = 255;
        if (b) b = 255;
    }
    return rgb_to_pixel(r, g, b);
}
uint32_t color_foreground(uint8_t color) {
    return ansi_to_rgb(color & 0x0F);
}
uint32_t color_background(uint8_t color) {
    uint8_t bg = (color >> 4) & 0x0F;
    return ansi_to_rgb(bg);
}

/* ================================================================
 * FRAMEBUFFER PIXEL ROUTINES
 * ================================================================ */

void framebuffer_put_pixel(uint32_t x,uint32_t y,uint32_t color) {
    if (!kernel_framebuffer)
        return;
    if (x >= framebuffer_width || y >= framebuffer_height)
        return;
    volatile uint8_t *base = (volatile uint8_t *)kernel_framebuffer->address;
    uint32_t bytes_per_pixel = framebuffer_bpp / 8;
    volatile uint32_t *pixel = (volatile uint32_t *)(base + (uint64_t)y * framebuffer_pitch + (uint64_t)x * bytes_per_pixel);
    *pixel = color;
}

static void framebuffer_fill_rect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t color) {
    if (!kernel_framebuffer)
        return;
    if (x >= framebuffer_width || y >= framebuffer_height)
        return;
    if (x + w > framebuffer_width)
        w = framebuffer_width - x;
    if (y + h > framebuffer_height)
        h = framebuffer_height - y;
    for (uint32_t yy = 0; yy < h; yy++) {
        for (uint32_t xx = 0; xx < w; xx++) {
            framebuffer_put_pixel(x + xx, y + yy, color);
        }
    }
}

/* ================================================================
 * TERMINAL CHARACTER RENDERING
 * ================================================================ */
static void put_char_at(char c, int row, int col) {
    if (row < 0 || row >= TERMINAL_ROWS)
        return;
    if (col < 0 || col >= TERMINAL_COLUMNS)
        return;
    terminal_chars[row][col] = c;
    terminal_colors[row][col] = active_color;
    draw_glyph(c,(uint32_t)col,(uint32_t)row,active_color);
}

static void redraw_terminal_cell(int row, int col) {
    if (row < 0 || row >= TERMINAL_ROWS)
        return;
    if (col < 0 || col >= TERMINAL_COLUMNS)
        return;
    char c = terminal_chars[row][col];
    if (c == 0)
        c = ' ';
    draw_glyph(c,(uint32_t)col,(uint32_t)row,terminal_colors[row][col]);
}

static void erase_cursor(void) {
    if (cursor_row < 0 || cursor_row >= TERMINAL_ROWS || cursor_col < 0 || cursor_col >= TERMINAL_COLUMNS)
        return;
    redraw_terminal_cell(cursor_row,cursor_col);
}

static void draw_cursor(void) {
    if (cursor_row < 0 || cursor_row >= TERMINAL_ROWS || cursor_col < 0 || cursor_col >= TERMINAL_COLUMNS)
        return;
    uint32_t x = (uint32_t)cursor_col * FONT_WIDTH;
    uint32_t y = (uint32_t)cursor_row * FONT_HEIGHT;
    uint32_t fg = color_foreground(active_color);
    framebuffer_fill_rect(x,y + FONT_HEIGHT - 2,FONT_WIDTH,2,fg);
}

static void update_hardware_cursor(void) {
    static int old_row = -1;
    static int old_col = -1;
    if (old_row >= 0 && old_col >= 0) {
        redraw_terminal_cell(old_row,old_col);
    }
    draw_cursor();
    old_row = cursor_row;
    old_col = cursor_col;
}

static void scroll_if_needed(void) {
    if (cursor_row < TERMINAL_ROWS)
        return;
    uint32_t bg = color_background(active_color);
    /*
     * Move framebuffer rows upward by one character row.
     */
    for (uint32_t y = FONT_HEIGHT; y < framebuffer_height; y++) {
        volatile uint8_t *base = (volatile uint8_t *)kernel_framebuffer->address;
        for (uint32_t x = 0; x < framebuffer_width * (framebuffer_bpp / 8); x++) {
            base[(uint64_t)(y - FONT_HEIGHT) * framebuffer_pitch + x] = base[(uint64_t)y * framebuffer_pitch + x];
        }
    }
    framebuffer_fill_rect(0,framebuffer_height - FONT_HEIGHT,framebuffer_width,FONT_HEIGHT,bg);
    cursor_row = TERMINAL_ROWS - 1;
}

static void put_char(char c) {
    if (c == '\n') {
        put_char_at(' ', cursor_row, cursor_col);
        cursor_col = 0;
        cursor_row++;
        scroll_if_needed();
        draw_cursor();
        return;
    }
    if (c == '\r') {
        put_char_at(' ', cursor_row, cursor_col);
        cursor_col = 0;
        draw_cursor();
        return;
    }
    if (c == '\b') {
        redraw_terminal_cell(cursor_row, cursor_col);
        if (cursor_col > 0) {
            cursor_col--;
            put_char_at(' ',cursor_row,cursor_col);
        }
        else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = TERMINAL_COLUMNS - 1;
            put_char_at(' ',cursor_row,cursor_col);
        }
        draw_cursor();
        return;
    }
    redraw_terminal_cell(cursor_row, cursor_col);
    put_char_at(c,cursor_row,cursor_col);
    cursor_col++;
    if (cursor_col >= TERMINAL_COLUMNS) {
        cursor_col = 0;
        cursor_row++;
        scroll_if_needed();
    }
    draw_cursor();
}

void print_string(const char *s) {
    if (!s)
        return;
    while (*s)
        put_char(*s++);
}

void print_int(int num) {
    char buf[12];
    int i = 0;
    if (num == 0) {
        put_char('0');
        return;
    }
    bool negative = false;
    if (num < 0) {
        negative = true;
        num = -num;
    }
    while (num > 0) {
        buf[i++] = (char)((num % 10) + '0');
        num /= 10;
    }
    if (negative)
        put_char('-');
    while (i > 0) {
        i--;
        put_char(buf[i]);
    }
}

void clear_screen(void) {
    if (!kernel_framebuffer)
        return;
    uint8_t bg_color = active_color;
    for (int row = 0; row < TERMINAL_ROWS; row++) {
        for (int col = 0; col < TERMINAL_COLUMNS; col++) {
            terminal_chars[row][col] = ' ';
            terminal_colors[row][col] = bg_color;
        }
    }
    framebuffer_fill_rect(0,0,framebuffer_width,framebuffer_height,color_background(active_color));
    cursor_row = 0;
    cursor_col = 0;
    draw_cursor();
}

/* ================================================================
 * BASIC STRING FUNCTIONS
 * ================================================================ */

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a,const char *b,int n) {
    while (n > 0 && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0)
        return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

int strlen(const char *s) {
    int len = 0;
    while (*s++)
        len++;
    return len;
}

char *strcpy(char *dst,const char *src) {
    char *ret = dst;
    while (*src)
        *dst++ = *src++;
    *dst = '\0';
    return ret;
}

char *strncpy(char *dst,const char *src,int n) {
    int i;
    for (i = 0; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++)
        dst[i] = '\0';
    return dst;
}

char *strchr(const char *s,int c) {
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    if (c == 0)
        return (char *)s;
    return 0;
}

/* ================================================================
 * UBLE CONFIGURATION
 * ================================================================ */

#define MAX_PASSWORD 32
#define MAX_CMD_LENGTH 18432
#define MAX_FILE_DATA 8192

#define KEY_ESC 27
#define KEY_ENTER 13
#define KEY_BACKSPACE 8
#define KEY_UP 128
#define KEY_DOWN 129
#define KEY_LEFT 130
#define KEY_RIGHT 131
#define KEY_SHIFT_SPACE 132
#define KEY_SHIFT_ENTER 133

#define ATXT_EXTENSION ".atxt"
#define USF_EXTENSION ".usf"
#define MENU_NEW 0
#define MENU_OPEN 1
#define MENU_SAVE 2
#define MENU_BUILD 3
#define MENU_RUN 4
#define MENU_EXIT 5

/* ================================================================
 * UBLE FILE FORMATS
 * ================================================================ */

#pragma pack(push, 1)
typedef struct {
    uint8_t magic[4];
    uint16_t version;
    uint32_t program_len;
} UbleHeader;

typedef struct {
    uint8_t short_jmp_opcode;
    uint8_t jmp_offset;
    uint8_t magic[4];
    uint8_t type;
    char app_name[16];
    uint16_t required_ram;
} UbleStructuredNativeHeader;
#pragma pack(pop)

/* ================================================================
 * VM OPCODES
 * ================================================================ */

#define OP_HALT          0x00
#define OP_PRINT_CHAR    0x01
#define OP_PRINT_NL      0x02
#define OP_PRINT_STR     0x03
#define OP_LOAD_REG      0x04
#define OP_ADD           0x05
#define OP_SUB           0x06
#define OP_PRINT_REG     0x07
#define OP_JMP           0x08
#define OP_JZ            0x09
#define OP_JNZ           0x0A
#define OP_CMP           0x0B
#define OP_INC           0x0C
#define OP_DEC           0x0D
#define OP_SET_COLOR     0x0E
#define OP_READ_REG      0x0F
#define OP_MUL           0x10
#define OP_DIV           0x11
#define OP_MOD           0x12
#define OP_CLEAR_SCREEN  0x13
#define OP_SET_CURSOR    0x14
#define OP_DRAW_CHAR     0x15
#define OP_READ_KEY      0x16
#define OP_DELAY         0x17
#define OP_DRAW_AT       0x18
#define OP_GET_CHAR      0x19
#define OP_CMP_IMM       0x1A
#define OP_ADD_IMM       0x1B
#define OP_SUB_IMM       0x1C
#define OP_DRAW_REG      0x1D
#define OP_KEY_AVAILABLE 0x1E
#define OP_RAND          0x1F
#define OP_AND           0x20
#define OP_OR            0x21
#define OP_XOR           0x22
#define OP_STORE_MEM     0x23
#define OP_LOAD_MEM      0x24
#define OP_RECT          0x25
#define OP_DRAW_TEXT     0x26
#define OP_TIMER         0x27
#define OP_RAND_RANGE    0x28
#define OP_JE            0x2A
#define OP_JNE           0x2B
#define OP_DRAW_MEMORY_CHAR 0x2C

/* ================================================================
 * GLOBAL STATE
 * ================================================================ */

static int editor_cursor_x = 0;
static int editor_cursor_y = 0;
static int editor_scroll = 0;

static char editor_filename[32];

uint32_t users_magic = USERS_MAGIC;

User users[MAX_USERS];

int num_users = 0;
int current_user_index = -1;

/* ================================================================
 * FORWARD DECLARATIONS
 * ================================================================ */

void kernel_main(void);

void users_save_to_disk(void);
void users_load_from_disk(void);

void editor_open(const char *filename);
void editor_reset(void);
void editor_draw(void);
void editor_save_txt(void);
void editor_build_usf(void);

uint32_t editor_export(char *buffer,uint32_t max);

bool save_usf(const char *name,char *source);

uint32_t assemble_text(char *text,uint8_t *program);

void cmd_run(const char *name);

/* ================================================================
 * KEYBOARD
 * ================================================================ */

static const char scancode_table[128] = {
    0,27,
    '1','2','3','4','5','6','7','8','9','0','-','=',
    '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,
    '\\','z','x','c','v','b','n','m',',','.','/',
    0,
    '*',
    0,
    ' '
};

uint8_t get_key(void) {
    static bool shift = false;
    while (1) {
        if (!(inb(0x64) & 1))
            continue;
        uint8_t sc = inb(0x60);
        if (sc & 0x80) {
            if (sc == 0xAA || sc == 0xB6)
                shift = false;
            continue;
        }
        if (sc == 0x2A || sc == 0x36) {
            shift = true;
            continue;
        }
        switch (sc) {
            case 0x48:
                return KEY_UP;
            case 0x50:
                return KEY_DOWN;
            case 0x4B:
                return KEY_LEFT;
            case 0x4D:
                return KEY_RIGHT;
            case 0x01:
                return KEY_ESC;
            case 0x1C:
                if (shift)
                    return KEY_SHIFT_ENTER;
                return '\n';
            case 0x39:
                if (shift)
                    return KEY_SHIFT_SPACE;
                return ' ';
            case 0x0E:
                return '\b';
        }
        if (sc >= 128)
            continue;
        char c = scancode_table[sc];
        if (c == 0)
            continue;
        if (shift) {
            if (c >= 'a' && c <= 'z') {
                c -= 32;
            }
            else {
                switch (c) {
                    case '1': c='!'; break;
                    case '2': c='@'; break;
                    case '3': c='#'; break;
                    case '4': c='$'; break;
                    case '5': c='%'; break;
                    case '6': c='^'; break;
                    case '7': c='&'; break;
                    case '8': c='*'; break;
                    case '9': c='('; break;
                    case '0': c=')'; break;
                    case '-': c='_'; break;
                    case '=': c='+'; break;
                    case '[': c='{'; break;
                    case ']': c='}'; break;
                    case ';': c=':'; break;
                    case '\'': c='"'; break;
                    case ',': c='<'; break;
                    case '.': c='>'; break;
                    case '/': c='?'; break;
                    case '\\': c='|'; break;
                    case '`': c='~'; break;
                }
            }
        }
        return (uint8_t)c;
    }
}

void read_line(char *buf,int max) {
    int idx = 0;
    while (1) {
        char c = get_key();
        if (c == '\n') {
            put_char('\n');
            buf[idx] = '\0';
            return;
        }
        if (c == '\b') {
            if (idx > 0) {
                idx--;
                put_char('\b');
            }
            continue;
        }
        if (idx < max - 1 && c >= 32 && c <= 126) {
            buf[idx++] = c;
            put_char(c);
        }
    }
}

/* ================================================================
 * USERS
 * ================================================================ */

void users_init(void) {
    users_load_from_disk();
    if (num_users <= 0 || num_users > MAX_USERS) {
        for (int i = 0; i < MAX_USERS; i++) {
            users[i].used = false;
            users[i].username[0] = '\0';
            users[i].password[0] = '\0';
        }
        users[0].used = true;
        strcpy(users[0].username,"admin");
        strcpy(users[0].password,"admin");
        num_users = 1;
        users_save_to_disk();
    }
    current_user_index = -1;
}

bool login(const char *username,const char *password) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].used && strcmp(users[i].username,username) == 0 && strcmp(users[i].password,password) == 0) {
            current_user_index = i;
            return true;
        }
    }
    return false;
}

bool add_user(const char *username,const char *password) {
    if (num_users >= MAX_USERS)
        return false;
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].used && strcmp(users[i].username,username) == 0) {
            return false;
        }
    }
    for (int i = 0; i < MAX_USERS; i++) {
        if (!users[i].used) {
            users[i].used = true;
            strncpy(users[i].username,username,MAX_USERNAME);
            users[i].username[MAX_USERNAME - 1] = '\0';
            strncpy(users[i].password,password,MAX_PASSWORD);
            users[i].password[MAX_PASSWORD - 1] = '\0';
            num_users++;
            users_save_to_disk();
            return true;
        }
    }
    return false;
}

bool del_user(const char *username) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].used && strcmp(users[i].username,username) == 0) {
            users[i].used = false;
            num_users--;
            if (i == current_user_index)
                current_user_index = -1;
            for (int f = 0; f < MAX_FILES; f++) {
                if (files[f].used && strcmp(files[f].owner,username) == 0) {
                    for (uint32_t j = 0; j < sizeof(File); j++) {
                        ((uint8_t *)&files[f])[j] = 0;
                    }
                }
            }
            users_save_to_disk();
            fs_save_to_disk();
            return true;
        }
    }
    return false;
}

/* ================================================================
 * USER DISK STORAGE
 * ================================================================ */

void users_save_to_disk(void) {
    uint32_t total_bytes = sizeof(users_magic) + sizeof(users) + sizeof(num_users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors > USERS_MAX_SECTORS) {
        print_string("User dataset index overflow.\n");
        return;
    }
    static uint16_t buffer[256 * USERS_MAX_SECTORS];
    uint8_t *dst = (uint8_t *)buffer;
    for (uint32_t i = 0; i < sectors * FS_SECTOR_SIZE; i++) {
        dst[i] = 0;
    }
    uint32_t off = 0;
    for (uint32_t i = 0; i < sizeof(users_magic); i++) {
        dst[off++] = ((uint8_t *)&users_magic)[i];
    }
    for (uint32_t i = 0; i < sizeof(users); i++) {
        dst[off++] = ((uint8_t *)users)[i];
    }
    for (uint32_t i = 0; i < sizeof(num_users); i++) {
        dst[off++] = ((uint8_t *)&num_users)[i];
    }
    if (!ata_write_sectors(USERS_LBA_START,(uint8_t)sectors,buffer)) {
        print_string("Users disk save failed\n");
    }
}

void users_load_from_disk(void) {
    uint32_t total_bytes = sizeof(users_magic) + sizeof(users) + sizeof(num_users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors > USERS_MAX_SECTORS) {
        num_users = 0;
        return;
    }
    static uint16_t buffer[256 * USERS_MAX_SECTORS];
    uint8_t *src = (uint8_t *)buffer;
    if (!ata_read_sectors(USERS_LBA_START,(uint8_t)sectors,buffer)) {
        num_users = 0;
        return;
    }
    uint32_t off = 0;
    uint32_t magic;
    for (uint32_t i = 0; i < sizeof(magic); i++) {
        ((uint8_t *)&magic)[i] = src[off++];
    }
    if (magic != USERS_MAGIC) {
        num_users = 0;
        return;
    }
    for (uint32_t i = 0; i < sizeof(users); i++) {
        ((uint8_t *)users)[i] = src[off++];
    }
    for (uint32_t i = 0; i < sizeof(num_users); i++) {
        ((uint8_t *)&num_users)[i] = src[off++];
    }
    if (users_magic != USERS_MAGIC || num_users < 0 || num_users > MAX_USERS) {
        num_users = 0;
    }
}

/* ================================================================
 * HEX ESCAPE PARSER
 * ================================================================ */

static uint32_t parse_hex_escapes(char *dest,const char *src,uint32_t max_len) {
    uint32_t d_idx = 0;
    uint32_t s_idx = 0;
    while (src[s_idx] != '\0' && d_idx < max_len - 1) {
        if (src[s_idx] == '\\' && src[s_idx + 1] == 'x') {
            char h1 = src[s_idx + 2];
            char h2 = src[s_idx + 3];
            if (h1 != '\0' && h2 != '\0') {
                uint8_t value = 0;
                if (h1 >= '0' && h1 <= '9')
                    value += (uint8_t)((h1 - '0') << 4);
                else if (h1 >= 'A' && h1 <= 'F')
                    value += (uint8_t)((h1 - 'A' + 10) << 4);
                else if (h1 >= 'a' && h1 <= 'f')
                    value += (uint8_t)((h1 - 'a' + 10) << 4);
                if (h2 >= '0' && h2 <= '9')
                    value += (uint8_t)(h2 - '0');
                else if (h2 >= 'A' && h2 <= 'F')
                    value += (uint8_t)(h2 - 'A' + 10);
                else if (h2 >= 'a' && h2 <= 'f')
                    value += (uint8_t)(h2 - 'a' + 10);
                dest[d_idx++] = (char)value;
                s_idx += 4;
                continue;
            }
        }
        dest[d_idx++] = src[s_idx++];
    }
    dest[d_idx] = '\0';
    return d_idx;
}

/* ================================================================
 * EDITOR
 * ================================================================ */

void editor_reset(void) {
    for (int y = 0; y < EDITOR_MAX_LINES; y++) {
        for (int x = 0; x < EDITOR_LINE_SIZE; x++) {
            editor_lines[y][x] = 0;
        }
    }
    editor_cursor_x = 0;
    editor_cursor_y = 0;
    editor_scroll = 0;
}

void editor_draw_menu_bar(void) {
    uint8_t old = active_color;
    active_color = 0x70;
    for (int x = 0; x < TERMINAL_COLUMNS; x++) {
        put_char_at(' ', 0, x);
    }
    const char *menu = " File  Edit  Search  Run  Help ";
    for (int i = 0; menu[i]; i++) {
        put_char_at(menu[i],0,i);
    }
    active_color = old;
}

void editor_draw_status_bar(void) {
    uint8_t old = active_color;
    active_color = 0x70;
    for (int x = 0; x < TERMINAL_COLUMNS; x++) {
        put_char_at(' ',TERMINAL_ROWS - 1,x);
    }
    cursor_row = TERMINAL_ROWS - 1;
    cursor_col = 0;
    print_string(editor_filename);
    put_char_at(' ',TERMINAL_ROWS - 1,cursor_col);
    cursor_col = 50;
    print_string("Ln ");
    print_int(editor_cursor_y + 1);
    print_string("  Col ");
    print_int(editor_cursor_x + 1);
    put_char_at(' ',TERMINAL_ROWS - 1,cursor_col);
    active_color = old;
}

int editor_save_dialog(void) {
    const char *items[] = {
        "Yes",
        "No",
        "Cancel"
    };
    int choice = 0;
    while (1) {
        editor_draw();
        uint8_t old = active_color;
        active_color = 0x70;
        for (int y = 8; y < 14; y++) {
            for (int x = 20; x < 60; x++) {
                put_char_at(' ',y,x);
            }
        }
        const char *title = "Save current project?";
        for (int i = 0; title[i]; i++) {
            put_char_at(title[i],9,22 + i);
        }
        for (int i = 0; i < 3; i++) {
            active_color = (i == choice) ? 0x1F : 0x70;
            for (int x = 0; items[i][x]; x++) {
                put_char_at(items[i][x],11 + i,24 + x);
            }
        }
        active_color = old;
        uint8_t k = get_key();
        switch (k) {
            case KEY_UP:
                if (choice > 0)
                    choice--;
                break;
            case KEY_DOWN:
                if (choice < 2)
                    choice++;
                break;
            case '\n':
                return choice;
            case KEY_ESC:
                return 2;
        }
    }
}

int editor_open_dialog(char *filename) {
    int list[MAX_FILES];
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used)
            continue;
        if (strcmp(files[i].owner,users[current_user_index].username) != 0)
            continue;
        int len = strlen(files[i].name);
        if (len >= 4 && strcmp(&files[i].name[len - 4],".txt") == 0) {
            list[count++] = i;
        }
        else if (len >= 5 && strcmp(&files[i].name[len - 5],".atxt") == 0) {
            list[count++] = i;
        }
    }
    if (count == 0) {
        print_string("No text files found.\n");
        return 0;
    }
    int choice = 0;
    int scroll = 0;
    while (1) {
        editor_draw();
        uint8_t old = active_color;
        active_color = 0x70;
        for (int y = 3; y < 20; y++) {
            for (int x = 18; x < 62; x++) {
                put_char_at(' ',y,x);
            }
        }
        const char *title = "Open File";
        for (int x = 0; title[x]; x++) {
            put_char_at(title[x],4,20 + x);
        }
        for (int i = 0; i < 12 && (scroll + i) < count; i++) {
            int file_index = scroll + i;
            active_color = (file_index == choice) ? 0x1F : 0x70;
            for (int x = 0; files[list[file_index]].name[x]; x++) {
                put_char_at(files[list[file_index]].name[x],6 + i,20 + x);
            }
        }
        active_color = old;
        uint8_t k = get_key();
        switch (k) {
            case KEY_UP:
                if (choice > 0) {
                    choice--;
                    if (choice < scroll) {
                        scroll--;
                    }
                }
                break;
            case KEY_DOWN:
                if (choice < count - 1) {
                    choice++;
                    if (choice >= scroll + 12) {
                        scroll++;
                    }
                }
                break;
            case '\n':
                strcpy(filename,files[list[choice]].name);
                return 1;
            case KEY_ESC:
                return 0;
        }
    }
}

int editor_file_menu(void) {
    const char *items[] = {
        "New",
        "Open",
        "Save",
        "Build",
        "Run",
        "Exit"
    };
    int choice = 0;
    while (1) {
        uint8_t old = active_color;
        active_color = 0x70;
        for (int y = 1; y <= 8; y++) {
            for (int x = 0; x < 18; x++) {
                put_char_at(' ',y,x);
            }
        }
        put_char_at('+', 1, 0);
        for (int i = 0; i < 6; i++) {
            active_color = (i == choice) ? 0x1F : 0x70;
            for (int x = 0; items[i][x]; x++) {
                put_char_at(items[i][x],i + 2,1 + x);
            }
        }
        active_color = old;
        uint8_t k = get_key();
        switch (k) {
            case KEY_UP:
                if (choice > 0)
                    choice--;
                break;
            case KEY_DOWN:
                if (choice < 5)
                    choice++;
                break;
            case '\n':
                return choice;
            case KEY_ESC:
                return -1;
        }
    }
}

void editor_draw(void) {
    clear_screen();
    editor_draw_menu_bar();
    for (int y = 0; y < TERMINAL_ROWS - 2; y++) {
        int line = y + editor_scroll;
        if (line >= EDITOR_MAX_LINES)
            break;
        for (int x = 0; x < EDITOR_LINE_SIZE; x++) {
            char c = editor_lines[line][x];
            if (c == 0)
                break;
            put_char_at(c,y + 1,x);
        }
    }
    editor_draw_status_bar();
    cursor_row = (editor_cursor_y - editor_scroll) + 1;
    cursor_col = editor_cursor_x;
    if (cursor_row < 1)
        cursor_row = 1;
    if (cursor_row > TERMINAL_ROWS - 2)
        cursor_row = TERMINAL_ROWS - 2;
    update_hardware_cursor();
}

void editor_insert_char(char c) {
    if (editor_cursor_x >=
        EDITOR_LINE_SIZE - 1)
        return;
    int len = 0;
    while (len < EDITOR_LINE_SIZE - 1 && editor_lines[editor_cursor_y][len]) {
        len++;
    }
    for (int i = len; i >= editor_cursor_x; i--) {
        editor_lines[editor_cursor_y][i + 1] = editor_lines[editor_cursor_y][i];
    }
    editor_lines[editor_cursor_y][editor_cursor_x] = c;
    editor_cursor_x++;
}

void editor_backspace(void) {
    if (editor_cursor_x == 0)
        return;
    editor_cursor_x--;
    int i = editor_cursor_x;
    while (i < EDITOR_LINE_SIZE - 1) {
        editor_lines[editor_cursor_y][i] = editor_lines[editor_cursor_y][i + 1];
        i++;
    }
}

void editor_open(const char *filename) {
    strcpy(editor_filename,filename);
    editor_reset();
    int idx = fs_find(filename);
    if (idx != -1) {
        static char buffer[MAX_FILE_DATA];
        uint32_t size = files[idx].size;
        if (size >= MAX_FILE_DATA)
            size = MAX_FILE_DATA - 1;
        uint32_t sectors = (size + 511) / 512;
        if (!ata_read_sectors(files[idx].start_sector,(uint8_t)sectors,(uint16_t *)buffer)) {
            print_string("File read failure\n");
        } else {
            buffer[size] = '\0';
            int y = 0;
            int x = 0;
            for (uint32_t i = 0; buffer[i] && y < EDITOR_MAX_LINES; i++) {
                if (buffer[i] == '\n') {
                    editor_lines[y][x] = 0;
                    y++;
                    x = 0;
                }
                else if (x < EDITOR_LINE_SIZE - 1) {
                    editor_lines[y][x++] = buffer[i];
                }
            }
        }
    }
    editor_draw();
    while (1) {
        uint8_t key = get_key();
        switch (key) {
            case KEY_ESC:
                clear_screen();
                return;
            case KEY_LEFT:
                if (editor_cursor_x > 0)
                    editor_cursor_x--;
                break;
            case KEY_RIGHT:
                if (editor_cursor_x < EDITOR_LINE_SIZE - 1) {
                    if (editor_lines[editor_cursor_y][editor_cursor_x]) {
                        editor_cursor_x++;
                    }
                }
                break;
            case KEY_UP:
                if (editor_cursor_y > 0)
                    editor_cursor_y--;
                {
                    int len = strlen(editor_lines[editor_cursor_y]);
                    if (editor_cursor_x > len)
                        editor_cursor_x = len;
                }
                if (editor_cursor_y < editor_scroll)
                    editor_scroll--;
                break;
            case KEY_DOWN:
                if (editor_cursor_y < EDITOR_MAX_LINES - 1) {
                    editor_cursor_y++;
                }
                {
                    int len = strlen(editor_lines[editor_cursor_y]);
                    if (editor_cursor_x > len)
                        editor_cursor_x = len;
                }
                if (editor_cursor_y >= editor_scroll + TERMINAL_ROWS - 2) {
                    editor_scroll++;
                }
                break;
            case '\n': {
                if (editor_cursor_y < EDITOR_MAX_LINES - 1) {
                    int len = strlen(editor_lines[editor_cursor_y]);
                    if (editor_cursor_x > len)
                        editor_cursor_x = len;
                    for (int y = EDITOR_MAX_LINES - 2; y > editor_cursor_y; y--) {
                        strcpy(editor_lines[y + 1],editor_lines[y]);
                    }
                    strcpy(editor_lines[editor_cursor_y + 1],&editor_lines[editor_cursor_y][editor_cursor_x]);
                    editor_lines[editor_cursor_y][editor_cursor_x] = 0;
                    editor_cursor_y++;
                    editor_cursor_x = 0;
                    if (editor_cursor_y >= editor_scroll + TERMINAL_ROWS - 2) {
                        editor_scroll++;
                    }
                }
                break;
            }
            case '\b':
                editor_backspace();
                break;
            case KEY_SHIFT_ENTER:
                editor_save_txt();
                editor_build_usf();
                break;
            case KEY_SHIFT_SPACE: {
                int action = editor_file_menu();
                editor_draw();
                switch (action) {
                    case MENU_NEW:
                        editor_reset();
                        break;
                    case MENU_OPEN: {
                        int answer = editor_save_dialog();
                        if (answer == 0)
                            editor_save_txt();
                        else if (answer == 2)
                            break;
                        char newfile[32];
                        if (editor_open_dialog(newfile)) {
                            editor_open(newfile);
                            return;
                        }
                        break;
                    }
                    case MENU_SAVE:
                        editor_save_txt();
                        break;
                    case MENU_BUILD:
                        editor_build_usf();
                        break;
                    case MENU_RUN: {
                        char name[40];
                        strcpy(name,editor_filename);
                        int len = strlen(name);
                        if (len > 4) {
                            name[len - 3] = 'u';
                            name[len - 2] = 's';
                            name[len - 1] = 'f';
                        }
                        cmd_run(name);
                        break;
                    }
                    case MENU_EXIT:
                        clear_screen();
                        return;
                }
                break;
            }
            default:
                if (key >= 32 && key <= 126) {
                    editor_insert_char(key);
                }
                break;
        }
        editor_draw();
    }
}

/* ================================================================
 * COMMANDS
 * ================================================================ */

void cmd_help(void) {
    print_string("Uble Shell Infrastructure Utilities:\n");
    print_string("  help                    - Show this menu\n");
    print_string("  clear                   - Clear screen\n");
    print_string("  echo <text>             - Print text\n");
    print_string("  login <u> <p>           - Login as user\n");
    print_string("  adduser <u> <p>         - Add user\n");
    print_string("  deluser <u>             - Wipe user data and files\n");
    print_string("  whoami                  - Inspect current user\n");
    print_string("  users                   - Print list of users\n");
    print_string("  ls                      - List Directory contents\n");
    print_string("  cat <name>              - Show file contents\n");
    print_string("  touch <name>            - Create empty file\n");
    print_string("  write <name> <text>     - Write raw file data values (Interprets \\xNN)\n");
    print_string("  rm <name>               - Delete file\n");
    print_string("  run <name>              - Execute Script Engine (.usf) or Native Apps (.unx)\n");
    print_string("  mem                     - System Memory Info\n");
    print_string("  sysinfo                 - Print system diagnostics\n");
    print_string("  matrix                  - Launch active digital waterfall terminal app\n");
    print_string("  edit <file>             - Open Uble File editor\n");
}

void cmd_clear(void) {
    clear_screen();
}

void cmd_echo(const char *args) {
    if (args && *args)
        print_string(args);
    print_string("\n");
}

void cmd_whoami(void) {
    if (current_user_index == -1) {
        print_string("Guest Context Session\n");
    } else {
        print_string(users[current_user_index].username);
        print_string("\n");
    }
}

void cmd_users(void) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].used) {
            print_string(" -> ");
            print_string(users[i].username);
            print_string("\n");
        }
    }
}

void cmd_ls(void) {
    if (current_user_index == -1) {
        cmd_users();
        return;
    }
    bool any = false;
    print_string("Owner        File Identifier        Allocated Metric\n");
    print_string("----------------------------------------------------\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].owner,users[current_user_index].username) == 0) {
            print_string(files[i].owner);
            int s_len = strlen(files[i].owner);
            for (int s = 0; s < 13 - s_len; s++)
                print_string(" ");
            print_string(files[i].name);
            s_len = strlen(files[i].name);
            for (int s = 0; s < 23 - s_len; s++)
                print_string(" ");
            print_int(files[i].size);
            print_string(" Bytes\n");
            any = true;
        }
    }
    if (!any) {
        print_string("(Zero storage allocation tables mapped)\n");
    }
}

void cmd_cat(const char *name) {
    if (current_user_index == -1) {
        print_string("Security Validation Failure\n");
        return;
    }
    if (!name || !*name) {
        print_string("Syntax: cat <name>\n");
        return;
    }
    int idx = fs_find(name);
    if (idx == -1) {
        print_string("File resource trace fault\n");
        return;
    }
    static uint8_t buffer[MAX_FILE_DATA];
    uint32_t sectors = (files[idx].size + 511) / 512;
    for (uint32_t i = 0; i < MAX_FILE_DATA; i++)
        buffer[i] = 0;
    if (!ata_read_sectors(files[idx].start_sector,(uint8_t)sectors,(uint16_t *)buffer)) {
        print_string("File read failure\n");
        return;
    }
    for (uint32_t i = 0; i < files[idx].size; i++) {
        put_char(buffer[i]);
    }
    put_char('\n');
}

void cmd_touch(const char *name) {
    if (current_user_index == -1) {
        print_string("Security Validation Failure\n");
        return;
    }
    if (!name || !*name) {
        print_string("Syntax: touch <name>\n");
        return;
    }
    if (strlen(name) >= MAX_FILENAME) {
        print_string("Identifier string overrun\n");
        return;
    }
    int idx = fs_create(name);
    if (idx == -1) {
        print_string("Allocation collision or table overrun index\n");
        return;
    }
    if (!fs_save_to_disk())
        print_string("Storage driver serialization failed\n");
}

void cmd_write(const char *name,const char *text) {
    if (current_user_index == -1) {
        print_string("Security Validation Failure\n");
        return;
    }
    if (!name || !*name || !text) {
        print_string("Syntax: write <name> <text>\n");
        return;
    }
    if (strlen(name) >= MAX_FILENAME) {
        print_string("Identifier string overrun\n");
        return;
    }
    int idx = fs_find(name);
    if (idx == -1) {
        idx = fs_create(name);
        if (idx == -1) {
            print_string("Allocation mapping creation error\n");
            return;
        }
        files[idx].start_sector = fs_next_free_sector();
    }
    static uint8_t buffer[MAX_FILE_DATA];
    for (uint32_t i = 0; i < MAX_FILE_DATA; i++)
        buffer[i] = 0;
    uint32_t size = parse_hex_escapes((char *)buffer,text,MAX_FILE_DATA);
    uint32_t sectors = (size + 511) / 512;
    uint32_t old_sectors = (files[idx].size + 511) / 512;
    if (sectors > old_sectors)
        files[idx].start_sector = fs_next_free_sector();
    if (!ata_write_sectors(files[idx].start_sector,(uint8_t)sectors,(uint16_t *)buffer)) {
        print_string("File data write failed\n");
        return;
    }
    files[idx].size = size;
    if (!fs_save_to_disk())
        print_string("Directory save failed\n");
    print_string("File written successfully\n");
}

void cmd_rm(const char *name) {
    if (current_user_index == -1) {
        print_string("Security Validation Failure\n");
        return;
    }
    if (!name || !*name) {
        print_string("Syntax: rm <name>\n");
        return;
    }
    if (!fs_delete(name)) {
        print_string("Target block trace error\n");
    }
    if (!fs_save_to_disk())
        print_string("Storage driver serialization failed\n");
}

/* ================================================================
 * BUILT-IN THINGS
 * ================================================================ */

void app_mem_diagnostics(void) {
    print_string("[=== Uble Core Memory Map Diagnostics ===]\n");
    int used_files = 0;
    uint32_t consumed_bytes = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            used_files++;
            consumed_bytes += files[i].size;
        }
    }
    print_string(" Mapped Files Metrics:   ");
    print_int(used_files);
    print_string(" / ");
    print_int(MAX_FILES);
    print_string("\n");
    print_string(" Cluster Block Density:  ");
    print_int(consumed_bytes);
    print_string(" Bytes used out of ");
    print_int(MAX_FILES * MAX_FILE_DATA);
    print_string("\n");
    print_string(" System Status Check:     [OPERATIONAL]\n");
}

void app_sysinfo(void) {
    print_string("System Hardware Profile Architecture Information:\n");
    print_string(" Operating Platform:  Uble Core System OS\n");
    print_string(" Processor Execution: 64-Bit x86-64 Limine UEFI pipeline\n");
    print_string(" Hardware Video Mode: Limine UEFI Framebuffer\n");
    print_string(" Native Driver Call:  ATA PIO Direct Access Mode\n");
}
void app_matrix(void) {
    clear_screen();
    uint32_t seed = 4120;
    active_color = 0x02;
    for (int i = 0; i < 400; i++) {
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        int target_col = seed % TERMINAL_COLUMNS;
        int target_row = (seed >> 8) % TERMINAL_ROWS;
        char glyph = (char)(33 + seed % 93);
        put_char_at(glyph,target_row,target_col);
        for (volatile int d = 0; d < 30000; d++) {
        }
    }
    active_color = COLOR;
    clear_screen();
}

/* ================================================================
 * EDITOR BUILDING
 * ================================================================ */

void replace_extension(char *filename,const char *ext) {
    int len = strlen(filename);
    while (len > 0) {
        if (filename[len] == '.') {
            filename[len] = 0;
            break;
        }
        len--;
    }
    int i = strlen(filename);
    while (*ext)
        filename[i++] = *ext++;
    filename[i] = 0;
}

void editor_build_usf(void) {
    static char source[MAX_FILE_DATA];
    editor_export(source,MAX_FILE_DATA);
    char usf_name[40];
    int i = 0;
    while (editor_filename[i] && i < 35) {
        usf_name[i] = editor_filename[i];
        i++;
    }
    usf_name[i] = 0;
    replace_extension(usf_name,".usf");
    save_usf(usf_name,source);
    print_string("\nBUILD COMPLETE: ");
    print_string(usf_name);
    print_string("\n");
}

void editor_save_txt(void) {
    static char buffer[MAX_FILE_DATA];
    editor_export(buffer,MAX_FILE_DATA);
    cmd_write(editor_filename,buffer);
    print_string("\nTXT SAVED\n");
}

uint32_t editor_export(char *buffer,uint32_t max) {
    uint32_t pos = 0;
    int last = -1;
    for (int y = EDITOR_MAX_LINES - 1; y >= 0; y--) {
        if (editor_lines[y][0] != 0) {
            last = y;
            break;
        }
    }
    if (last == -1) {
        buffer[0] = 0;
        return 0;
    }
    for (int y = 0; y <= last; y++) {
        for (int x = 0; editor_lines[y][x]; x++) {
            if (pos >= max - 1)
                break;
            buffer[pos++] = editor_lines[y][x];
        }
        if (y != last && pos < max - 1) {
            buffer[pos++] = '\n';
        }
    }
    buffer[pos] = 0;
    return pos;
}

/* ================================================================
 * UBLE SAVE / ASSEMBLER
 * ================================================================ */

bool save_usf(const char *name,char *source) {
    static uint8_t program[MAX_FILE_DATA];
    uint32_t len = assemble_text(source,program);
    static uint8_t file[MAX_FILE_DATA];
    UbleHeader *hdr = (UbleHeader *)file;
    hdr->magic[0] = 'U';
    hdr->magic[1] = 'B';
    hdr->magic[2] = 'L';
    hdr->magic[3] = 'E';
    hdr->version = 1;
    hdr->program_len = len;
    for (uint32_t i = 0; i < len; i++) {
        file[sizeof(UbleHeader) + i] = program[i];
    }
    int idx = fs_find(name);
    if (idx == -1) {
        idx = fs_create(name);
        if (idx == -1)
            return false;
        files[idx].start_sector = fs_next_free_sector();
    }
    uint32_t total = sizeof(UbleHeader) + len;
    uint32_t sectors = (total + 511) / 512;
    if (sectors == 0)
        sectors = 1;
    if (!ata_write_sectors(files[idx].start_sector,(uint8_t)sectors,(uint16_t *)file)) {
        return false;
    }
    files[idx].size = total;
    fs_save_to_disk();
    return true;
}

/* ================================================================
 * ASSEMBLER
 * ================================================================ */

#define ASM_MAX_LINES 256
#define ASM_MAX_SIZE MAX_FILE_DATA
#define ASM_MAX_LABELS 128
#define ASM_LABEL_SIZE 32

typedef struct {
    char name[ASM_LABEL_SIZE];
    uint32_t address;
} AsmLabel;
AsmLabel asm_labels[ASM_MAX_LABELS];
int asm_label_count = 0;
int asm_find_label(char *name) {
    for (int i = 0; i < asm_label_count; i++) {
        if (strcmp(asm_labels[i].name,name) == 0) {
            return (int)asm_labels[i].address;
        }
    }
    return -1;
}

bool asm_is_label(char *token) {
    int len = strlen(token);
    if (len == 0)
        return false;
    return token[len - 1] == ':';
}

int asm_tokenize(char *line,char tokens[][32],int max) {
    int count = 0;
    while (*line) {
        while (*line == ' ')
            line++;
        if (*line == 0)
            break;
        if (count >= max)
            break;
        int i = 0;
        while (*line && *line != ' ' && i < 31) {
            tokens[count][i++] = *line++;
        }
        tokens[count][i] = 0;
        count++;
    }
    return count;
}
int8_t asm_number(char *s) {
    int n = 0;
    int neg = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        n *= 10;
        n += *s - '0';
        s++;
    }
    if (neg)
        n = -n;
    return (int8_t)n;
}

uint32_t assemble_line(char tokens[][32],int count,uint8_t *out) {
    if (count == 0)
        return 0;
    if (strcmp(tokens[0], "HALT") == 0) {
        out[0] = OP_HALT;
        return 1;
    }
    if (strcmp(tokens[0], "CLEAR") == 0) {
        out[0] = OP_CLEAR_SCREEN;
        return 1;
    }
    if (strcmp(tokens[0], "COLOR") == 0) {
        out[0] = OP_SET_COLOR;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "WAIT") == 0) {
        out[0] = OP_DELAY;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "DRAW") == 0) {
        out[0] = OP_DRAW_AT;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        out[3] = tokens[3][0];
        return 4;
    }
    if (strcmp(tokens[0], "PRINT") == 0) {
        out[0] = OP_PRINT_STR;
        int pos = 1;
        for (int i = 1; i < count; i++) {
            int j = 0;
            while (tokens[i][j])
                out[pos++] = tokens[i][j++];
            if (i + 1 < count)
                out[pos++] = ' ';
        }
        out[pos++] = 0;
        return pos;
    }
    if (strcmp(tokens[0], "PRINTCHAR") == 0) {
        out[0] = OP_PRINT_CHAR;
        out[1] = tokens[1][0];
        return 2;
    }
    if (strcmp(tokens[0], "NEWLINE") == 0) {
        out[0] = OP_PRINT_NL;
        return 1;
    }
    if (strcmp(tokens[0], "LOAD") == 0) {
        out[0] = OP_LOAD_REG;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "ADD") == 0) {
        out[0] = OP_ADD;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "SUB") == 0) {
        out[0] = OP_SUB;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "PRINTREG") == 0) {
        out[0] = OP_PRINT_REG;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "RANDOM") == 0) {
        out[0] = OP_RAND;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "KEY") == 0) {
        out[0] = OP_KEY_AVAILABLE;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "READKEY") == 0) {
        out[0] = OP_READ_KEY;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "CMP") == 0) {
        out[0] = OP_CMP_IMM;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "JMP") == 0) {
        out[0] = OP_JMP;
        uint16_t addr = (uint8_t)asm_number(tokens[1]);
        out[1] = addr >> 8;
        out[2] = addr & 0xFF;
        return 3;
    }
    if (strcmp(tokens[0], "DRAWREG") == 0) {
        out[0] = OP_DRAW_REG;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        out[3] = tokens[3][0];
        return 4;
    }
    if (strcmp(tokens[0], "JNZ") == 0) {
        out[0] = OP_JNZ;
        uint16_t addr = (uint8_t)asm_number(tokens[1]);
        out[1] = addr >> 8;
        out[2] = addr & 0xFF;
        return 3;
    }
    if (strcmp(tokens[0], "AND") == 0) {
        out[0] = OP_AND;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "OR") == 0) {
        out[0] = OP_OR;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "XOR") == 0) {
        out[0] = OP_XOR;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "STORE") == 0) {
        out[0] = OP_STORE_MEM;
        out[1] = asm_number(tokens[1]);
        uint16_t addr = (uint8_t)asm_number(tokens[2]);
        out[2] = addr >> 8;
        out[3] = addr & 0xFF;
        return 4;
    }
    if (strcmp(tokens[0], "LOADMEM") == 0) {
        out[0] = OP_LOAD_MEM;
        out[1] = asm_number(tokens[1]);
        uint16_t addr = (uint8_t)asm_number(tokens[2]);
        out[2] = addr >> 8;
        out[3] = addr & 0xFF;
        return 4;
    }
    if (strcmp(tokens[0], "RECT") == 0) {
        out[0] = OP_RECT;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        out[3] = asm_number(tokens[3]);
        out[4] = asm_number(tokens[4]);
        out[5] = tokens[5][0];
        return 6;
    }
    if (strcmp(tokens[0], "DRAWTEXT") == 0) {
        out[0] = OP_DRAW_TEXT;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        int pos = 3;
        for (int i = 3; i < count; i++) {
            int j = 0;
            while (tokens[i][j])
                out[pos++] = tokens[i][j++];
            if (i + 1 < count)
                out[pos++] = ' ';
        }
        out[pos++] = 0;
        return pos;
    }
    if (strcmp(tokens[0], "TIMER") == 0) {
        out[0] = OP_TIMER;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "RANDRANGE") == 0) {
        out[0] = OP_RAND_RANGE;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "JE") == 0) {
        out[0] = OP_JE;
        uint16_t addr = (uint8_t)asm_number(tokens[1]);
        out[1] = addr >> 8;
        out[2] = addr & 0xFF;
        return 3;
    }
    if (strcmp(tokens[0], "JNE") == 0) {
        out[0] = OP_JNE;
        uint16_t addr = (uint8_t)asm_number(tokens[1]);
        out[1] = addr >> 8;
        out[2] = addr & 0xFF;
        return 3;
    }
    if (strcmp(tokens[0], "DRAWMEMCHAR") == 0) {
        out[0] = OP_DRAW_MEMORY_CHAR;
        uint16_t addr = (uint8_t)asm_number(tokens[1]);
        out[1] = addr >> 8;
        out[2] = addr & 0xFF;
        out[3] = asm_number(tokens[2]);
        out[4] = asm_number(tokens[3]);
        return 5;
    }
    if (strcmp(tokens[0], "INC") == 0) {
        out[0] = OP_INC;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "DEC") == 0) {
        out[0] = OP_DEC;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "MUL") == 0) {
        out[0] = OP_MUL;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "DIV") == 0) {
        out[0] = OP_DIV;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "MOD") == 0) {
        out[0] = OP_MOD;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "CURSOR") == 0) {
        out[0] = OP_SET_CURSOR;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "DRAWCHAR") == 0) {
        out[0] = OP_DRAW_CHAR;
        out[1] = tokens[1][0];
        return 2;
    }
    if (strcmp(tokens[0], "GETCHAR") == 0) {
        out[0] = OP_GET_CHAR;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        out[3] = asm_number(tokens[3]);
        return 4;
    }
    if (strcmp(tokens[0], "ADDI") == 0) {
        out[0] = OP_ADD_IMM;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "SUBI") == 0) {
        out[0] = OP_SUB_IMM;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    if (strcmp(tokens[0], "JZ") == 0) {
        out[0] = OP_JZ;
        uint16_t addr = (uint8_t)asm_number(tokens[1]);
        out[1] = addr >> 8;
        out[2] = addr & 0xFF;
        return 3;
    }
    if (strcmp(tokens[0], "READREG") == 0) {
        out[0] = OP_READ_REG;
        out[1] = asm_number(tokens[1]);
        return 2;
    }
    if (strcmp(tokens[0], "CMPREG") == 0) {
        out[0] = OP_CMP;
        out[1] = asm_number(tokens[1]);
        out[2] = asm_number(tokens[2]);
        return 3;
    }
    return 0;
}

uint32_t assemble_text(char *text,uint8_t *program) {
    uint32_t size = 0;
    asm_label_count = 0;
    char *line = text;
    /*
     * PASS 1
     */
    while (*line) {
        char buffer[128];
        int i = 0;
        while (*line && *line != '\n' && i < 127) {
            buffer[i++] = *line++;
        }
        buffer[i] = 0;
        if (*line == '\n')
            line++;
        char tokens[16][32];
        for (int x = 0; x < 16; x++)
            tokens[x][0] = 0;
        int count = asm_tokenize(buffer,tokens,16);
        if (count > 0) {
            if (asm_is_label(tokens[0])) {
                if (asm_label_count < ASM_MAX_LABELS) {
                    int len = strlen(tokens[0]);
                    tokens[0][len - 1] = 0;
                    strcpy(asm_labels[asm_label_count].name,tokens[0]);
                    asm_labels[asm_label_count].address = size;
                    asm_label_count++;
                }
            } else {
                size += assemble_line(tokens,count,&program[size]);
            }
        }
    }
    /*
     * PASS 2
     */
    size = 0;
    line = text;
    while (*line) {
        char buffer[128];
        int i = 0;
        while (*line && *line != '\n' && i < 127) {
            buffer[i++] = *line++;
        }
        buffer[i] = 0;
        if (*line == '\n')
            line++;
        char tokens[16][32];
        for (int x = 0; x < 16; x++)
            tokens[x][0] = 0;
        int count = asm_tokenize(buffer,tokens,16);
        if (count == 0)
            continue;
        if (asm_is_label(tokens[0]))
            continue;
        if (strcmp(tokens[0], "JMP") == 0 || strcmp(tokens[0], "JZ") == 0 || strcmp(tokens[0], "JNZ") == 0 || strcmp(tokens[0], "JE") == 0 || strcmp(tokens[0], "JNE") == 0) {
            int addr = asm_find_label(tokens[1]);
            if (addr >= 0) {
                char number[16];
                int n = 0;
                if (addr == 0) {
                    number[n++] = '0';
                } else {
                    int temp = addr;
                    char rev[16];
                    int r = 0;
                    while (temp) {
                        rev[r++] = (char)((temp % 10) + '0');
                        temp /= 10;
                    }
                    while (r)
                        number[n++] = rev[--r];
                }
                number[n] = 0;
                strcpy(tokens[1],number);
            }
        }
        size += assemble_line(tokens,count,&program[size]);
    }
    return size;
}

/* ================================================================
 * VM RUNTIME
 * ================================================================ */

void cmd_run(const char *name) {
    if (current_user_index == -1) {
        print_string("Security Validation Failure\n");
        return;
    }
    if (!name || !*name) {
        print_string("Syntax: run <name>\n");
        return;
    }
    int idx = fs_find(name);
    if (idx == -1) {
        print_string("Target file asset trace failure\n");
        return;
    }
    File *f = &files[idx];
    static uint8_t file_buffer[MAX_FILE_DATA];
    for (uint32_t i = 0; i < MAX_FILE_DATA; i++)
        file_buffer[i] = 0;
    uint32_t sectors = (f->size + 511) / 512;
    if (!ata_read_sectors(f->start_sector,(uint8_t)sectors,(uint16_t *)file_buffer)) {
        print_string("File load failed\n");
        return;
    }
    uint8_t *raw_bytes = file_buffer;
    /*
     * Native .unx
     */
    if (raw_bytes[0] == 0xEB) {
        UbleStructuredNativeHeader
            *native_hdr = (UbleStructuredNativeHeader *)file_buffer;
        if (native_hdr->magic[0] != 'U' || native_hdr->magic[1] != 'B' || native_hdr->magic[2] != 'L' || native_hdr->magic[3] != 'E' || native_hdr->type != 0x02) {
            print_string("Execution Trap: Corrupted proprietary structure validation signature.\n");
            return;
        }
        print_string("[Executing Native Context Application Block: ");
        print_string(native_hdr->app_name);
        print_string("]\n");
        typedef void (*native_execution_block_t)(void);
        native_execution_block_t
            run_native_app = (native_execution_block_t)file_buffer;
        run_native_app();
        print_string("\n[Native Application Code Signal Return Success]\n");
        return;
    }
    /*
     * Uble VM
     */
    if (raw_bytes[0] == 'U' && raw_bytes[1] == 'B' && raw_bytes[2] == 'L' && raw_bytes[3] == 'E') {
        UbleHeader *header = (UbleHeader *)file_buffer;
        uint8_t *bytecode = file_buffer + sizeof(UbleHeader);
        uint32_t pc = 0;
        bool running = true;
        int32_t virtual_registers[8] = {0,0,0,0,0,0,0,0};
        uint8_t virtual_memory[4096] = {0};
        bool zero_flag = false;
        active_color = COLOR;
        print_string("[Uble Interpreter Runtime Virtual Environment Engaged]\n");
        while (running && pc < header->program_len && sizeof(UbleHeader) + pc < MAX_FILE_DATA) {
            uint8_t opcode = bytecode[pc++];
            switch (opcode) {
                case OP_HALT:
                    running = false;
                    break;
                case OP_PRINT_CHAR: {
                    char c = (char)bytecode[pc++];
                    put_char(c);
                    break;
                }
                case OP_PRINT_NL:
                    put_char('\n');
                    break;
                case OP_PRINT_STR:
                    while (bytecode[pc] != 0x00 && sizeof(UbleHeader) + pc < MAX_FILE_DATA - 1) {
                        put_char((char)bytecode[pc++]);
                    }
                    pc++;
                    break;
                case OP_LOAD_REG: {
                    uint8_t reg = bytecode[pc++];
                    int8_t val = (int8_t)bytecode[pc++];
                    if (reg < 8)
                        virtual_registers[reg] = val;
                    break;
                }
                case OP_ADD: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        virtual_registers[a] += virtual_registers[b];
                    }
                    break;
                }
                case OP_SUB: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        virtual_registers[a] -= virtual_registers[b];
                    }
                    break;
                }
                case OP_PRINT_REG: {
                    uint8_t reg = bytecode[pc++];
                    if (reg < 8)
                        print_int(virtual_registers[reg]);
                    break;
                }
                case OP_JMP: {
                    uint16_t target = ((uint16_t)bytecode[pc] << 8) | bytecode[pc + 1];
                    pc = target;
                    break;
                }
                case OP_JZ: {
                    uint16_t target = ((uint16_t)bytecode[pc] << 8) | bytecode[pc + 1];
                    pc += 2;
                    if (zero_flag)
                        pc = target;
                    break;
                }
                case OP_JNZ: {
                    uint16_t target = ((uint16_t)bytecode[pc] << 8) | bytecode[pc + 1];
                    pc += 2;
                    if (!zero_flag)
                        pc = target;
                    break;
                }
                case OP_CMP: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        zero_flag = virtual_registers[a] == virtual_registers[b];
                    }
                    break;
                }
                case OP_INC: {
                    uint8_t reg = bytecode[pc++];
                    if (reg < 8)
                        virtual_registers[reg]++;
                    break;
                }
                case OP_DEC: {
                    uint8_t reg = bytecode[pc++];
                    if (reg < 8)
                        virtual_registers[reg]--;
                    break;
                }
                case OP_SET_COLOR:
                    active_color = bytecode[pc++];
                    break;
                case OP_READ_REG: {
                    uint8_t reg = bytecode[pc++];
                    if (reg < 8) {
                        char c = get_key();
                        put_char(c);
                        if (c >= '0' && c <= '9') {
                            virtual_registers[reg] = c - '0';
                        }
                        else {
                            virtual_registers[reg] = 0;
                        }
                    }
                    break;
                }
                case OP_MUL: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        virtual_registers[a] *= virtual_registers[b];
                    }
                    break;
                }
                case OP_DIV: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        int32_t den = virtual_registers[b];
                        int32_t num = virtual_registers[a];
                        if (den == 0) {
                            print_string("\nVM Crash: Division by Zero Exception Safety Intercept.");
                            running = false;
                        }
                        else {
                            virtual_registers[a] /= den;
                        }
                    }
                    break;
                }
                case OP_MOD: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        int32_t den = virtual_registers[b];
                        if (den == 0) {
                            print_string("\nVM Crash: Modulo by Zero Exception Safety Intercept.");
                            running = false;
                        }
                        else {virtual_registers[a] %= den;
                        }
                    }
                    break;
                }
                case OP_CLEAR_SCREEN:
                    clear_screen();
                    break;
                case OP_SET_CURSOR: {
                    uint8_t row = bytecode[pc++];
                    uint8_t col = bytecode[pc++];
                    if (row < TERMINAL_ROWS)
                        cursor_row = row;
                    if (col < TERMINAL_COLUMNS)
                        cursor_col = col;
                    update_hardware_cursor();
                    break;
                }
                case OP_DRAW_CHAR: {
                    char c = bytecode[pc++];
                    put_char(c);
                    break;
                }
                case OP_READ_KEY: {
                    uint8_t reg = bytecode[pc++];
                    if (reg < 8)
                        virtual_registers[reg] = get_key();
                    break;
                }
                case OP_DELAY: {
                    uint8_t amount = bytecode[pc++];
                    for (volatile uint32_t i = 0; i < (uint32_t)amount * 10000; i++)
                    {
                    }
                    break;
                }
                case OP_DRAW_AT: {
                    uint8_t row = bytecode[pc++];
                    uint8_t col = bytecode[pc++];
                    char c = bytecode[pc++];
                    if (row < TERMINAL_ROWS && col < TERMINAL_COLUMNS) {
                        put_char_at(c,row,col);
                    }
                    break;
                }
                case OP_GET_CHAR: {
                    uint8_t reg = bytecode[pc++];
                    uint8_t row = bytecode[pc++];
                    uint8_t col = bytecode[pc++];
                    if (reg < 8 && row < TERMINAL_ROWS && col < TERMINAL_COLUMNS) {
                        virtual_registers[reg] = 0;
                    }
                    break;
                }
                case OP_CMP_IMM: {
                    uint8_t reg = bytecode[pc++];
                    uint8_t value = bytecode[pc++];
                    if (reg < 8)
                        zero_flag = virtual_registers[reg] == value;
                    break;
                }
                case OP_ADD_IMM: {
                    uint8_t reg = bytecode[pc++];
                    uint8_t value = bytecode[pc++];
                    if (reg < 8)
                        virtual_registers[reg] += value;
                    break;
                }
                case OP_SUB_IMM: {
                    uint8_t reg = bytecode[pc++];
                    uint8_t value = bytecode[pc++];
                    if (reg < 8)
                        virtual_registers[reg] -= value;
                    break;
                }
                case OP_DRAW_REG: {
                    uint8_t row_reg = bytecode[pc++];
                    uint8_t col_reg = bytecode[pc++];
                    uint8_t char_val = bytecode[pc++];
                    if (row_reg < 8 && col_reg < 8) {
                        uint8_t row = virtual_registers[row_reg];
                        uint8_t col = virtual_registers[col_reg];
                        if (row < TERMINAL_ROWS && col < TERMINAL_COLUMNS) {
                            put_char_at(char_val,row,col);
                        }
                    }
                    break;
                }
                case OP_KEY_AVAILABLE: {
                    uint8_t reg = bytecode[pc++];
                    if (reg < 8) {
                        uint8_t status = inb(0x64);
                        virtual_registers[reg] = (status & 1) ? 1 : 0;
                    }
                    break;
                }
                case OP_RAND: {
                    uint8_t reg = bytecode[pc++];
                    static uint32_t seed = 12345;
                    seed = seed * 1103515245 + 12345;
                    if (reg < 8) {
                        virtual_registers[reg] = (seed >> 16) & 0x7FFF;
                    }
                    break;
                }
                case OP_AND: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        virtual_registers[a] &= virtual_registers[b];
                    }
                    break;
                }
                case OP_OR: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        virtual_registers[a] |= virtual_registers[b];
                    }
                    break;
                }
                case OP_XOR: {
                    uint8_t a = bytecode[pc++];
                    uint8_t b = bytecode[pc++];
                    if (a < 8 && b < 8) {
                        virtual_registers[a] ^= virtual_registers[b];
                    }
                    break;
                }
                case OP_STORE_MEM: {
                    uint8_t reg = bytecode[pc++];
                    uint16_t addr = ((uint16_t)bytecode[pc++] << 8);
                    addr |= bytecode[pc++];
                    if (reg < 8 && addr < 4096) {
                        virtual_memory[addr] = (uint8_t)
                            virtual_registers[reg];
                    }
                    break;
                }
                case OP_LOAD_MEM: {
                    uint8_t reg = bytecode[pc++];
                    uint16_t addr = ((uint16_t)bytecode[pc++] << 8);
                    addr |= bytecode[pc++];
                    if (reg < 8 && addr < 4096) {
                        virtual_registers[reg] = virtual_memory[addr];
                    }
                    break;
                }
                case OP_RECT: {
                    uint8_t row = bytecode[pc++];
                    uint8_t col = bytecode[pc++];
                    uint8_t width = bytecode[pc++];
                    uint8_t height = bytecode[pc++];
                    char c = bytecode[pc++];
                    for (uint8_t y = 0; y < height; y++) {
                        for (uint8_t x = 0; x < width; x++) {
                            if (row + y < TERMINAL_ROWS && col + x < TERMINAL_COLUMNS) {
                                put_char_at(c,row + y,col + x);
                            }
                        }
                    }
                    break;
                }
                case OP_DRAW_TEXT: {
                    uint8_t row = bytecode[pc++];
                    uint8_t col = bytecode[pc++];
                    uint8_t x = col;
                    while (bytecode[pc] != 0 && x < TERMINAL_COLUMNS) {
                        put_char_at(bytecode[pc++],row,x++);
                    }
                    pc++;
                    break;
                }
                case OP_TIMER: {
                    uint8_t reg = bytecode[pc++];
                    static uint32_t timer = 0;
                    timer++;
                    if (reg < 8)
                        virtual_registers[reg] = timer;
                    break;
                }
                case OP_RAND_RANGE: {
                    uint8_t reg = bytecode[pc++];
                    uint8_t max = bytecode[pc++];
                    static uint32_t seed = 54321;
                    seed = seed * 1103515245 + 12345;
                    if (reg < 8 && max != 0) {
                        virtual_registers[reg] = (seed >> 16) % max;
                    }
                    break;
                }
                case OP_JE: {
                    uint16_t target = ((uint16_t)bytecode[pc] << 8) | bytecode[pc + 1];
                    pc += 2;
                    if (zero_flag)
                        pc = target;
                    break;
                }
                case OP_JNE: {
                    uint16_t target = ((uint16_t)bytecode[pc] << 8) | bytecode[pc + 1];
                    pc += 2;
                    if (!zero_flag)
                        pc = target;
                    break;
                }
                case OP_DRAW_MEMORY_CHAR: {
                    uint16_t addr = ((uint16_t)bytecode[pc++] << 8);
                    addr |= bytecode[pc++];
                    uint8_t row = bytecode[pc++];
                    uint8_t col = bytecode[pc++];
                    if (addr < 4096 && row < TERMINAL_ROWS && col < TERMINAL_COLUMNS) {
                        put_char_at(virtual_memory[addr],row,col);
                    }
                    break;
                }
                default:
                    print_string("\nRuntime fault: Illegal instruction opcode validation trap.");
                    running = false;
                    break;
            }
        }
        active_color = COLOR;
        print_string("\n[VM Logic Execution Session Terminated Clean]\n");
        return;
    }
    print_string("Execution error: Unsupported binary configuration standard file definition map.\n");
}

/* ================================================================
 * COMMAND PARSER
 * ================================================================ */
void split_first(const char *cmd, char *first, int first_max, const char **rest_out) {
    int i = 0;
    while (*cmd == ' ')
        cmd++;
    while (*cmd && *cmd != ' ') {
        if (i < first_max - 1)
            first[i++] = *cmd;
        cmd++;
    }
    first[i] = '\0';
    while (*cmd == ' ')
        cmd++;
    *rest_out = cmd;
}

void process_command(const char *cmdline) {
    char cmd[32];
    const char *rest;
    if (!cmdline || !*cmdline)
        return;
    split_first(cmdline,cmd,sizeof(cmd),&rest);
    if (strcmp(cmd, "help") == 0)
        cmd_help();
    else if (strcmp(cmd, "clear") == 0)
        cmd_clear();
    else if (strcmp(cmd, "echo") == 0)
        cmd_echo(rest);
    else if (strcmp(cmd, "whoami") == 0)
        cmd_whoami();
    else if (strcmp(cmd, "users") == 0)
        cmd_users();
    else if (strcmp(cmd, "ls") == 0)
        cmd_ls();
    else if (strcmp(cmd, "mem") == 0)
        app_mem_diagnostics();
    else if (strcmp(cmd, "sysinfo") == 0)
        app_sysinfo();
    else if (strcmp(cmd, "matrix") == 0)
        app_matrix();
    else if (strcmp(cmd, "cat") == 0) {
        char name[32];
        const char *dummy;
        split_first(rest,name,sizeof(name),&dummy);
        cmd_cat(name);
    }
    else if (strcmp(cmd, "touch") == 0) {
        char name[32];
        const char *dummy;
        split_first(rest,name,sizeof(name),&dummy);
        cmd_touch(name);
    }
    else if (strcmp(cmd, "write") == 0) {
        char name[32];
        const char *text;
        split_first(rest,name,sizeof(name),&text);
        cmd_write(name,text);
    }
    else if (strcmp(cmd, "rm") == 0) {
        char name[32];
        const char *dummy;
        split_first(rest,name,sizeof(name),&dummy);
        cmd_rm(name);
    }
    else if (strcmp(cmd, "run") == 0) {
        char name[32];
        const char *dummy;
        split_first(rest,name,sizeof(name),&dummy);
        cmd_run(name);
    }
    else if (strcmp(cmd, "login") == 0) {
        char user[32];
        const char *pass;
        split_first(rest,user,sizeof(user),&pass);
        if (!*user || !*pass) {
            print_string("Syntax: login <user> <pass>\n");
        }
        else if (login(user, pass)) {
            print_string("Session identity tokens loaded successfully.\n");
        } else {
            print_string("Invalid credentials validation profiles matched.\n");
        }
    }
    else if (strcmp(cmd, "adduser") == 0) {
        char user[32];
        const char *pass;
        if (current_user_index == -1) {
            print_string("Security Validation Failure\n");
            return;
        }
        split_first(rest,user,sizeof(user),&pass);
        if (!*user || !*pass) {
            print_string("Syntax: adduser <user> <pass>\n");
        }
        else if (add_user(user, pass)) {
            print_string("Profile map appended successfully.\n");
        } else {
            print_string("Execution denied: Profile definition issues.\n");
        }
    }
    else if (strcmp(cmd, "deluser") == 0) {
        char user[32];
        const char *dummy;
        if (current_user_index == -1) {
            print_string("Security Validation Failure\n");
            return;
        }
        if (strcmp(users[current_user_index].username,"admin") != 0)
        {
            print_string("Admin level token authentication required.\n");
            return;
        }
        split_first(rest,user,sizeof(user),&dummy);
        if (!*user) {
            print_string("Syntax: deluser <user>\n");
        }
        else if (del_user(user)) {
            print_string("Profile sequence stripped from file records.\n");
        } else {
            print_string("No matching profile allocation traced.\n");
        }
    }
    else if (strcmp(cmd, "edit") == 0) {
        char filename[32];
        const char *dummy;
        split_first(rest,filename,sizeof(filename),&dummy);
        if (filename[0] == 0) {
            print_string("Usage: edit <file>\n");
        } else {
            editor_open(filename);
        }
    }
    else {
        print_string("Unknown environmental routine statement command.\n");
    }
}

/* ================================================================
 * SHELL
 * ================================================================ */

void shell_loop(void) {
    static char cmdline[MAX_CMD_LENGTH];
    while (1) {
        print_string("\n");
        if (current_user_index == -1) {
            print_string("Uble$ ");
        } else {
            print_string("Uble/");
            print_string(users[current_user_index].username);
            print_string("$ ");
        }
        read_line(
            cmdline,
            MAX_CMD_LENGTH);

        process_command(
            cmdline);
    }
}

/* ================================================================
 * LIMINE FRAMEBUFFER INITIALIZATION
 * ================================================================ */

static bool limine_video_init(void) {
    if (framebuffer_request.response == NULL)
        return false;

    if (framebuffer_request.response->
            framebuffer_count < 1)
        return false;

    kernel_framebuffer =
        framebuffer_request.response->
            framebuffers[0];

    if (!kernel_framebuffer)
        return false;

    framebuffer_width = (uint32_t)kernel_framebuffer->width;
    framebuffer_height = (uint32_t)kernel_framebuffer->height;
    framebuffer_pitch = (uint32_t)kernel_framebuffer->pitch;
    framebuffer_bpp = (uint32_t)kernel_framebuffer->bpp;
    terminal_update_size();

    /*
     * Limine's normal x86-64 framebuffer is 32-bit RGB.
     * The framebuffer response also provides the masks/shifts.
     */
    framebuffer_red_shift = kernel_framebuffer->red_mask_shift;
    framebuffer_green_shift = kernel_framebuffer->green_mask_shift;
    framebuffer_blue_shift = kernel_framebuffer->blue_mask_shift;
    if (framebuffer_bpp != 32)
        return false;
    return true;
}


/* ================================================================
 * KERNEL MAIN
 * ================================================================ */

void kernel_main(void) {
    active_color = COLOR;
    clear_screen();
    print_string("========================================================\n");
    print_string("                         UBLE\n");
    print_string("========================================================\n");
    print_string(" Systems Identity Initialized. Default: admin / admin\n");
    print_string(" Type 'help' to dump shell system instruction map\n\n");
    if (!memory_init())
    {
        print_string(
            "MEM: FATAL: memory initialization failed.\n"
        );

        for (;;)
        {
            __asm__ volatile ("cli; hlt");
        }
    }
    pci_init(); // new
    e1000_init(); // new
    if (!e1000_init()) {
        print_string("E1000: initialization failed.\n");
    }
    else
    {
        /*
        * ============================================================
        * E1000 TRANSMIT TEST
        * ============================================================
        */

        uint8_t test_frame[60] = { 0 };

        /*
        * Destination MAC: broadcast.
        */
        test_frame[0] = 0xFF;
        test_frame[1] = 0xFF;
        test_frame[2] = 0xFF;
        test_frame[3] = 0xFF;
        test_frame[4] = 0xFF;
        test_frame[5] = 0xFF;

        /*
        * Source MAC.
        *
        * Filled below.
        */

        /*
        * EtherType = 0x88B5
        *
        * Experimental/local-use Ethernet type.
        */
        test_frame[12] = 0x88;
        test_frame[13] = 0xB5;

        /*
        * Test payload begins at byte 14.
        */
        test_frame[14] = 'U';
        test_frame[15] = 'B';
        test_frame[16] = 'L';
        test_frame[17] = 'E';

        test_frame[18] = '-';
        test_frame[19] = 'N';
        test_frame[20] = 'E';
        test_frame[21] = 'O';

        test_frame[22] = '-';
        test_frame[23] = 'T';
        test_frame[24] = 'X';
        test_frame[25] = '!';

        /*
        * Get our E1000 MAC address.
        */
        const uint8_t *mac = e1000_get_mac();

        /*
        * Put our MAC into the Ethernet source field.
        */
        for (uint32_t i = 0; i < 6; i++) {
            test_frame[6 + i] = mac[i];
        }
        print_string("NET: sending Ethernet test frame...\n");

        if (e1000_send(test_frame,60)) {
            print_string("NET: Ethernet frame submitted!\n");
        } else {
            print_string("NET: Ethernet frame submission FAILED!\n");
        }
        print_string("NET: waiting for TX completion...\n");
        for (uint32_t timeout = 0; timeout < 1000000; timeout++) {
            if (e1000_tx_complete()) {
                print_string( "NET: E1000 TX COMPLETE!\n");
                break;
            }
            if (timeout == 999999) {
                print_string("NET: ERROR: E1000 TX timed out.\n");
            }
        }
    }
    users_init();
    fs_init();
    shell_loop();
}


/* ================================================================
 * LIMINE KERNEL ENTRY
 * ================================================================ */

static void hcf(void) {
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

void kmain(void) {
    /*
     * This is the Limine kernel entry point.
     * There is deliberately no Multiboot entry/header.
     */
    if (!LIMINE_BASE_REVISION_SUPPORTED(
            limine_base_revision))
    {
        hcf();
    }
    if (!limine_video_init()) {
        hcf();
    }
    kernel_main();
    hcf();
}

void poweroff(void) {
    outw(0x604,0x2000);
    outw(0xB004,0x2000);
    outw(0x4004,0x3400);
    hcf();
}