/* ========================================================================
 * UBLE GRAPHICAL OPERATING SYSTEM - CUSTOM CORE EDITION
 * PIXEL-PERFECT FRAMEBUFFER DESKTOP ENVIRONMENT
 * ======================================================================== */

/* ==== 1. MULTIBOOT GRAPHICS HEADER ==== */
#define MULTIBOOT_FLAGS    0x00000007 
#define MULTIBOOT_MAGIC    0x1BADB002
#define MULTIBOOT_CHECKSUM -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

__attribute__((section(".multiboot")))
const unsigned long multiboot_header[] = {
    MULTIBOOT_MAGIC,
    MULTIBOOT_FLAGS,
    MULTIBOOT_CHECKSUM,
    0, 0, 0, 0, 0, 
    0,             // Mode type: 0 = Linear Framebuffer Graphics
    800,           // Screen Width (Pixels)
    600,           // Screen Height (Pixels)
    32             // 32-Bit True Color Depth (ARGB Layout)
};

/* ==== 2. SYSTEM TYPE DEFINITIONS ==== */
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef int             int32_t;
typedef int             bool;

#define true  1
#define false 0

#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define MAX_CMD_LENGTH  18432
#define MAX_USERS       32

/* ==== 3. HARDWARE SPECIFIC TYPE LAYOUTS ==== */
#pragma pack(push, 1)
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint32_t framebuffer_addr_lower;
    uint32_t framebuffer_addr_upper;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} MultibootInfo;
#pragma pack(pop)

/* ==== 4. STRINGS STANDARDS COMPATIBILITY UTILITIES ==== */
char* strcpy(char* dst, const char* src) {
    char* ret = dst;
    while (*src) { *dst++ = *src++; }
    *dst = '\0';
    return ret;
}
int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
int strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

/* ==== 5. STORAGE STRUCTURE CONFIGURATIONS ==== */
#define MAX_FILES        512
#define MAX_FILENAME     32
#define MAX_FILE_DATA    16384  

typedef struct {
    char owner[MAX_USERNAME];
    char name[MAX_FILENAME];
    char data[MAX_FILE_DATA];
    uint8_t used;
} File;

#define FS_LBA_START       2048u
#define USERS_MAGIC        0x55534552  
#define FS_SECTOR_SIZE     512u
#define FS_MAGIC           0x46534F32  

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t sectors;
} FSHeader;

#define FS_MAX_SECTORS     16384  
#define USERS_MAX_SECTORS  128
#define USERS_LBA_START    (FS_LBA_START + FS_MAX_SECTORS)

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    uint8_t used;
} User;

/* ==== 6. EGA/VGA COMPATIBLE PALETTE DEFINITIONS ==== */
#define PAL_BLACK         0x00000000
#define PAL_BLUE          0x000000AA
#define PAL_GREEN         0x0000AA00
#define PAL_CYAN          0x0000AAAA
#define PAL_RED           0x00AA0000
#define PAL_MAGENTA       0x00AA00AA
#define PAL_BROWN         0x00AA5500
#define PAL_LIGHT_GRAY    0x00AAAAAA
#define PAL_DARK_GRAY     0x00555555
#define PAL_LIGHT_BLUE    0x005555FF
#define PAL_LIGHT_GREEN   0x0055FF55 
#define PAL_LIGHT_CYAN    0x0055FFFF
#define PAL_LIGHT_RED     0x00FF5555
#define PAL_LIGHT_MAGENTA 0x00FF55FF
#define PAL_YELLOW        0x00FFFF55
#define PAL_WHITE         0x00FFFFFF

#define WIN1_BACKGROUND   0x00008080 
#define WIN1_HEADER       0x000000AA 
#define WIN1_BORDER       0x00FFFFFF 

/* ==== 7. GLOBAL PERSISTENT INSTANCE MEMORY VOLUMES ==== */
static uint32_t* fb_mem    = 0;
static uint32_t  fb_width  = 0;
static uint32_t  fb_height = 0;
static uint32_t  fb_pitch  = 0;

uint32_t users_magic       = USERS_MAGIC;
User users[MAX_USERS];
File files[MAX_FILES];
int  num_users             = 0;
int  current_user_index    = -1;

/* Hardware Cursor Coordinates and Trail Tracking buffers */
static int32_t mouse_x         = 400;
static int32_t mouse_y         = 300;
static int32_t last_mouse_x    = 400;
static int32_t last_mouse_y    = 300;
static bool    mouse_click     = false;
static bool    prev_click      = false;
static uint32_t mouse_back_buf[16 * 16]; 

/* Graphical Terminal Window Bounds & Text Cursors */
#define TERM_X          15
#define TERM_Y          330
#define TERM_WIDTH      770
#define TERM_HEIGHT     255
static int term_cursor_x   = TERM_X + 5;
static int term_cursor_y   = TERM_Y + 25;

/* File Manager UI Bounds */
#define EXPLORER_X      20
#define EXPLORER_Y      45
#define EXPLORER_WIDTH  360
#define EXPLORER_HEIGHT 270
static bool explorer_open = false;

/* Notepad UI Bounds & Text Buffer State */
#define NOTEPAD_X       400
#define NOTEPAD_Y       45
#define NOTEPAD_WIDTH   380
#define NOTEPAD_HEIGHT  270
static bool notepad_open = false;
static char notepad_buffer[512] = "";
static int  notepad_buf_idx = 0;
static int  notepad_saved_count = 0;

/* Start Menu Interface State */
static bool start_menu_open = false;

/* Dynamic UI Click Area Engine */
typedef struct {
    int x, y, w, h;
    char label[32];
    uint32_t action_id; 
} DesktopWidget;

static DesktopWidget widgets[45];
static int num_widgets = 0;

/* Internal Forward Declarations */
void users_save_to_disk(void);
void users_load_from_disk(void);
bool fs_save_to_disk(void);
bool fs_load_from_disk(void);
void desktop_redraw_pipeline(void);

/* ==== 8. BITMAP FONT MATRIX ENGINE ==== */
static const uint8_t font_bitmap[128][8] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['!'] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00},
    ['"'] = {0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['#'] = {0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A, 0x00},
    ['$'] = {0x04, 0x1F, 0x10, 0x0E, 0x01, 0x1F, 0x04, 0x00},
    ['%'] = {0x11, 0x12, 0x04, 0x08, 0x10, 0x09, 0x11, 0x00},
    ['&'] = {0x0C, 0x12, 0x12, 0x0C, 0x15, 0x12, 0x0D, 0x00},
    ['\'']= {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['('] = {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02, 0x00},
    [')'] = {0x40, 0x20, 0x10, 0x10, 0x10, 0x20, 0x40, 0x00},
    ['*'] = {0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00, 0x00},
    ['+'] = {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00, 0x00},
    [','] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08},
    ['-'] = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},
    ['/'] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00},
    ['0'] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E, 0x00},
    ['1'] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00},
    ['2'] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F, 0x00},
    ['3'] = {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E, 0x00},
    ['4'] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02, 0x00},
    ['5'] = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E, 0x00},
    ['6'] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E, 0x00},
    ['7'] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08, 0x00},
    ['8'] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0x00},
    ['9'] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C, 0x00},
    [':'] = {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00, 0x00},
    [';'] = {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x04, 0x08, 0x00},
    ['<'] = {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0x00},
    ['='] = {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00},
    ['>'] = {0x40, 0x20, 0x10, 0x08, 0x10, 0x20, 0x40, 0x00},
    ['?'] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04, 0x00},
    ['@'] = {0x0E, 0x11, 0x01, 0x0D, 0x15, 0x15, 0x0D, 0x00},
    ['A'] = {0x04, 0x0A, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x00},
    ['B'] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0x00},
    ['C'] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E, 0x00},
    ['D'] = {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C, 0x00},
    ['E'] = {0x1F, 0x10, 0x10, 0x1C, 0x10, 0x10, 0x1F, 0x00},
    ['F'] = {0x1F, 0x10, 0x10, 0x1C, 0x10, 0x10, 0x10, 0x00},
    ['G'] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F, 0x00},
    ['H'] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00},
    ['I'] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00},
    ['J'] = {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C, 0x00},
    ['K'] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11, 0x00},
    ['L'] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00},
    ['M'] = {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11, 0x00},
    ['N'] = {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x00},
    ['O'] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00},
    ['P'] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10, 0x00},
    ['Q'] = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D, 0x00},
    ['R'] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11, 0x00},
    ['S'] = {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E, 0x00},
    ['T'] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00},
    ['U'] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00},
    ['V'] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00},
    ['W'] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11, 0x00},
    ['X'] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11, 0x00},
    ['Y'] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x00},
    ['Z'] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F, 0x00},
    ['a'] = {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0x00},
    ['b'] = {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E, 0x00},
    ['c'] = {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E, 0x00},
    ['d'] = {0x01, 0x01, 0x0F, 0x11, 0x11, 0x11, 0x0F, 0x00},
    ['e'] = {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0x00},
    ['f'] = {0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08, 0x00},
    ['g'] = {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E},
    ['h'] = {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11, 0x00},
    ['i'] = {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E, 0x00},
    ['j'] = {0x02, 0x00, 0x06, 0x02, 0x02, 0x02, 0x12, 0x0C},
    ['k'] = {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12, 0x00},
    ['l'] = {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00},
    ['m'] = {0x00, 0x00, 0x1A, 0x15, 0x11, 0x11, 0x11, 0x00},
    ['n'] = {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11, 0x00},
    ['o'] = {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00},
    ['p'] = {0x00, 0x00, 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10},
    ['q'] = {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x01},
    ['r'] = {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10, 0x00},
    ['s'] = {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E, 0x00},
    ['t'] = {0x08, 0x08, 0x1C, 0x08, 0x08, 0x08, 0x05, 0x02},
    ['u'] = {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D, 0x00},
    ['v'] = {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00},
    ['w'] = {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A, 0x00},
    ['x'] = {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00},
    ['y'] = {0x00, 0x00, 0x11, 0x11, 0x11, 0x0F, 0x01, 0x0E},
    ['z'] = {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F, 0x00}
};

/* ==== 9. LOW-LEVEL PIXEL PRIMITIVES GRAPHICS ENGINES ==== */
uint32_t get_pixel(uint32_t x, uint32_t y) {
    if (x >= fb_width || y >= fb_height) return 0;
    return fb_mem[y * (fb_pitch / 4) + x];
}
void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height) return;
    fb_mem[y * (fb_pitch / 4) + x] = color;
}
void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t i = 0; i < h; i++) {
        for (uint32_t j = 0; j < w; j++) { put_pixel(x + j, y + i, color); }
    }
}
void draw_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    uint8_t glyph = (uint8_t)c;
    if (glyph >= 128) return;
    for (int row = 0; row < 8; row++) {
        uint8_t bits = font_bitmap[glyph][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) { put_pixel(x + col, y + row, color); }
        }
    }
}
void draw_string(const char* s, uint32_t x, uint32_t y, uint32_t color) {
    while (*s) { draw_char(*s, x, y, color); x += 8; s++; }
}

void draw_bubble_logo(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    for (int y = -r; y <= (int)r; y++) {
        for (int x = -r; x <= (int)r; x++) {
            if (x*x + y*y <= (int)(r*r)) {
                if (x*x + y*y >= (int)((r-2)*(r-2))) { put_pixel(cx + x, cy + y, PAL_WHITE); }
                else if (x == -(int)(r/3) && y == -(int)(r/3)) {
                    put_pixel(cx + x, cy + y, PAL_WHITE);
                    put_pixel(cx + x + 1, cy + y, PAL_WHITE);
                } else { put_pixel(cx + x, cy + y, color); }
            }
        }
    }
}

/* Base Window Painter with an Explicit [X] Close Button Register */
void draw_custom_window(int x, int y, int w, int h, const char* title, uint32_t close_act_id) {
    draw_rect(x, y, w, h, PAL_LIGHT_GRAY); 
    draw_rect(x, y, w, 22, WIN1_HEADER);   
    draw_rect(x, y, w, 2, WIN1_BORDER);
    draw_rect(x, y + 20, w, 2, WIN1_BORDER);
    draw_rect(x, y, 2, h, WIN1_BORDER);
    draw_rect(x + w - 2, y, 2, h, WIN1_BORDER);
    draw_rect(x, y + h - 2, w, 2, WIN1_BORDER);
    draw_string(title, x + 6, y + 6, PAL_WHITE);

    // Draw standard high-contrast OS close box [X]
    int cx = x + w - 18;
    int cy = y + 3;
    draw_rect(cx, cy, 14, 14, PAL_RED);
    draw_string("X", cx + 3, cy + 3, PAL_WHITE);

    if (num_widgets < 45) {
        widgets[num_widgets].x = cx; widgets[num_widgets].y = cy;
        widgets[num_widgets].w = 14; widgets[num_widgets].h = 14;
        strcpy(widgets[num_widgets].label, "CLOSE");
        widgets[num_widgets].action_id = close_act_id;
        num_widgets++;
    }
}

/* ==== 10. GRAPHICS ORIENTED TERMINAL EMULATOR CORE ==== */
void put_char(char c) {
    if (c == '\n') {
        term_cursor_x = TERM_X + 5;
        term_cursor_y += 12;
        if (term_cursor_y >= (TERM_Y + TERM_HEIGHT - 15)) {
            draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
            term_cursor_y = TERM_Y + 25;
        }
        return;
    } else if (c == '\b') {
        if (term_cursor_x > (TERM_X + 5)) {
            term_cursor_x -= 8;
            draw_rect(term_cursor_x, term_cursor_y, 8, 8, PAL_BLACK);
        }
        return;
    }
    draw_char(c, term_cursor_x, term_cursor_y, PAL_WHITE);
    term_cursor_x += 8;
    if (term_cursor_x >= (TERM_X + TERM_WIDTH - 10)) {
        term_cursor_x = TERM_X + 5;
        term_cursor_y += 12;
    }
}

void print_string(const char* s) { while (*s) { put_char(*s++); } }

/* ==== 11. PORT LEVEL I/O INTERFACE BUS ==== */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "dN"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" :: "a"(val), "dN"(port));
}

/* ==== 12. PS/2 MOUSE DRIVER CORE ==== */
void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (type == 0) { if ((inb(0x64) & 1) == 1) return; } 
        else { if ((inb(0x64) & 2) == 0) return; }
    }
}
void mouse_write(uint8_t write) {
    mouse_wait(1); outb(0x64, 0xD4);
    mouse_wait(1); outb(0x60, write);
}
uint8_t mouse_read(void) { mouse_wait(0); return inb(0x60); }

void mouse_init(void) {
    mouse_wait(1); outb(0x64, 0xA8); 
    mouse_wait(1); outb(0x64, 0x20);  
    mouse_wait(0); uint8_t status = (inb(0x60) | 2); 
    mouse_wait(1); outb(0x64, 0x60); 
    mouse_wait(1); outb(0x60, status); 
    mouse_write(0xF6); mouse_read();     
    mouse_write(0xF4); mouse_read();     
}

void mouse_poll_cycle(void) {
    if ((inb(0x64) & 1) == 0) return; 
    if (!(inb(0x64) & 0x20))  return; 

    uint8_t status = inb(0x60);
    int32_t rel_x  = mouse_read();
    int32_t rel_y  = mouse_read();

    if (status & 0x40 || status & 0x80) return; 
    if (status & 0x10) rel_x |= 0xFFFFFF00; 
    if (status & 0x20) rel_y |= 0xFFFFFF00;

    last_mouse_x = mouse_x; last_mouse_y = mouse_y;
    mouse_x += rel_x;       mouse_y -= rel_y; 

    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= (int32_t)fb_width - 12)  mouse_x = fb_width - 13;
    if (mouse_y >= (int32_t)fb_height - 16) mouse_y = fb_height - 17;

    prev_click = mouse_click;
    mouse_click = (status & 1) ? true : false;
}

void clear_old_mouse(void) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 12; x++) { put_pixel(last_mouse_x + x, last_mouse_y + y, mouse_back_buf[y * 12 + x]); }
    }
}
void save_mouse_backbuffer(void) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 12; x++) { mouse_back_buf[y * 12 + x] = get_pixel(mouse_x + x, mouse_y + y); }
    }
}
void draw_mouse_cursor(int32_t mx, int32_t my) {
    static const char cursor_matrix[16][12] = {
        "W...........", "WW..........", "WBW.........", "WBBW........",
        "WBBBW.......", "WBBBBW......", "WBBBBBW.....", "WBBBBBBW....",
        "WBBBBBBBW...", "WBBBBBBBBW..", "WBBBBWWWWWW.", "WBBWWB......",
        "WBW..WB.....", "WW....WB....", "......WB....", ".......W...."
    };
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 12; x++) {
            char p = cursor_matrix[y][x];
            if (p == 'W')      put_pixel(mx + x, my + y, PAL_WHITE);
            else if (p == 'B') put_pixel(mx + x, my + y, PAL_BLACK);
        }
    }
}

/* ==== 13. PS/2 HARDWARE KEYBOARD DRIVER ==== */
static const char scancode_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
};

char get_key(void) {
    uint8_t status = inb(0x64);
    if ((status & 1) && !(status & 0x20)) {
        uint8_t sc = inb(0x60);
        if (sc & 0x80) return 0;
        if (sc == 0x1C) return '\n';
        if (sc == 0x0E) return '\b';
        if (sc < 128) return scancode_table[sc];
    }
    return 0;
}

/* ==== 14. ATA DRIVER STORAGE MANAGEMENT ENGINE ==== */
#define ATA_TIMEOUT 100000
static bool ata_wait(uint8_t mask, uint8_t value) {
    for (int i = 0; i < ATA_TIMEOUT; i++) { if ((inb(0x1F7) & mask) == value) return true; }
    return false;
}
bool ata_read_sectors(uint32_t lba, uint8_t count, uint16_t* buf) {
    if (count == 0) return false;
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba); outb(0x1F4, (uint8_t)(lba >> 8)); outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);
    for (int i = 0; i < count; i++) {
        if (!ata_wait(0x88, 0x08)) return false;
        for (int j = 0; j < 256; j++) buf[i * 256 + j] = inw(0x1F0);
    }
    return true;
}
bool ata_write_sectors(uint32_t lba, uint8_t count, uint16_t* buf) {
    if (count == 0) return false;
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba); outb(0x1F4, (uint8_t)(lba >> 8)); outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);
    for (int i = 0; i < count; i++) {
        if (!ata_wait(0x88, 0x08)) return false;
        for (int j = 0; j < 256; j++) outw(0x1F0, buf[i * 256 + j]);
    }
    outb(0x1F7, 0xE7); ata_wait(0x80, 0x00); return true;
}

bool fs_save_to_disk(void) {
    uint32_t total_bytes = sizeof(FSHeader) + sizeof(files);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    FSHeader header = { FS_MAGIC, 2, sectors };
    static uint8_t sector_buf[FS_SECTOR_SIZE]; uint32_t bytes_written = 0;
    for (uint32_t s = 0; s < sectors; s++) {
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) sector_buf[i] = 0;
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) {
            if (bytes_written < sizeof(FSHeader)) sector_buf[i] = ((uint8_t*)&header)[bytes_written];
            else if (bytes_written < total_bytes) sector_buf[i] = ((uint8_t*)files)[bytes_written - sizeof(FSHeader)];
            else break;
            bytes_written++;
        }
        if (!ata_write_sectors(FS_LBA_START + s, 1, (uint16_t*)sector_buf)) return false;
    }
    return true;
}
bool fs_load_from_disk(void) {
    static uint8_t sector_buf[FS_SECTOR_SIZE];
    if (!ata_read_sectors(FS_LBA_START, 1, (uint16_t*)sector_buf)) return false;
    FSHeader* header = (FSHeader*)sector_buf;
    if (header->magic != FS_MAGIC || header->version != 2) return false;
    uint32_t total_sectors = header->sectors; uint32_t bytes_read = 0; uint32_t total_bytes = sizeof(FSHeader) + sizeof(files);
    for (uint32_t s = 0; s < total_sectors; s++) {
        if (!ata_read_sectors(FS_LBA_START + s, 1, (uint16_t*)sector_buf)) return false;
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) {
            if (bytes_read >= sizeof(FSHeader) && bytes_read < total_bytes) ((uint8_t*)files)[bytes_read - sizeof(FSHeader)] = sector_buf[i];
            bytes_read++;
        }
    }
    return true;
}
void users_save_to_disk(void) {
    uint32_t total_bytes = sizeof(users_magic) + sizeof(users) + sizeof(num_users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    static uint16_t buffer[256 * USERS_MAX_SECTORS]; uint8_t* dst = (uint8_t*)buffer;
    for (uint32_t i = 0; i < sectors * FS_SECTOR_SIZE; i++) dst[i] = 0;
    uint32_t off = 0;
    for (uint32_t i = 0; i < sizeof(users_magic); i++) dst[off++] = ((uint8_t*)&users_magic)[i];
    for (uint32_t i = 0; i < sizeof(users); i++) dst[off++] = ((uint8_t*)users)[i];
    for (uint32_t i = 0; i < sizeof(num_users); i++) dst[off++] = ((uint8_t*)&num_users)[i];
    ata_write_sectors(USERS_LBA_START, (uint8_t)sectors, buffer);
}
void users_load_from_disk(void) {
    uint32_t total_bytes = sizeof(users_magic) + sizeof(users) + sizeof(num_users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    static uint16_t buffer[256 * USERS_MAX_SECTORS]; uint8_t* src = (uint8_t*)buffer;
    if (!ata_read_sectors(USERS_LBA_START, sectors, buffer)) { num_users = 0; return; }
    uint32_t off = 0; uint32_t magic;
    for (uint32_t i = 0; i < sizeof(magic); i++) ((uint8_t*)&magic)[i] = src[off++];
    if (magic != USERS_MAGIC) { num_users = 0; return; }
    for (uint32_t i = 0; i < sizeof(users); i++) ((uint8_t*)users)[i] = src[off++];
    for (uint32_t i = 0; i < sizeof(num_users); i++) ((uint8_t*)&num_users)[i] = src[off++];
}

int fs_find(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) return i;
    }
    return -1;
}
int fs_create(const char* name) {
    if (fs_find(name) != -1) return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            strcpy(files[i].owner, "admin");
            strcpy(files[i].name, name);
            files[i].data[0] = '\0';
            return i;
        }
    }
    return -1;
}

/* ==== 15. INTERACTIVE APPS & SHELL CONTROLLER SUITE ==== */
void split_first(const char* cmd, char* first, int first_max, const char** rest_out) {
    int i = 0; while (*cmd == ' ') cmd++;  
    while (*cmd && *cmd != ' ') { if (i < first_max - 1) first[i++] = *cmd; cmd++; }
    first[i] = '\0'; while (*cmd == ' ') cmd++; *rest_out = cmd;
}

void process_command(const char* cmdline) {
    char cmd[32]; const char* rest;
    if (!cmdline || !*cmdline) return;
    split_first(cmdline, cmd, sizeof(cmd), &rest);
    
    if (strcmp(cmd, "help") == 0) {
        print_string("Commands: help, clear, sysinfo, files\n");
    } else if (strcmp(cmd, "clear") == 0) {
        draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
        term_cursor_x = TERM_X + 5; term_cursor_y = TERM_Y + 25;
    } else if (strcmp(cmd, "sysinfo") == 0) {
        print_string("Uble OS Core. Linear Framebuffer active (800x600).\n");
    } else if (strcmp(cmd, "files") == 0) {
        print_string("Stored System Files:\n");
        int count = 0;
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].used) {
                print_string(" - "); print_string(files[i].name); print_string("\n");
                count++;
            }
        }
        if(count == 0) print_string(" No files found.\n");
    } else {
        print_string("Unknown operational command sentence.\n");
    }
}

/* ==== 16. DESKTOP GRAPHICS COMPOSITOR PIPELINE ==== */
void draw_file_manager(void) {
    draw_custom_window(EXPLORER_X, EXPLORER_Y, EXPLORER_WIDTH, EXPLORER_HEIGHT, "File Manager", 55);
    draw_rect(EXPLORER_X + 4, EXPLORER_Y + 22, EXPLORER_WIDTH - 8, EXPLORER_HEIGHT - 26, PAL_WHITE);
    
    draw_string("File Name", EXPLORER_X + 15, EXPLORER_Y + 30, PAL_BLACK);
    draw_string("Owner", EXPLORER_X + 220, EXPLORER_Y + 30, PAL_BLACK);
    draw_rect(EXPLORER_X + 10, EXPLORER_Y + 42, EXPLORER_WIDTH - 20, 1, PAL_DARK_GRAY);

    int print_y = EXPLORER_Y + 50;
    int items = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            draw_string(files[i].name, EXPLORER_X + 15, print_y, PAL_BLUE);
            draw_string(files[i].owner, EXPLORER_X + 220, print_y, PAL_BLACK);
            print_y += 14; items++;
            if (items >= 14) break;
        }
    }
    if (items == 0) { draw_string("[No Text Files Created]", EXPLORER_X + 60, EXPLORER_Y + 110, PAL_DARK_GRAY); }
}

void draw_notepad(void) {
    draw_custom_window(NOTEPAD_X, NOTEPAD_Y, NOTEPAD_WIDTH, NOTEPAD_HEIGHT, "Notepad", 66);
    
    // Typing field background canvas rectangle
    draw_rect(NOTEPAD_X + 6, NOTEPAD_Y + 26, NOTEPAD_WIDTH - 12, NOTEPAD_HEIGHT - 62, PAL_WHITE);
    
    // Draw string from text editing buffer array
    if (strlen(notepad_buffer) > 0) {
        draw_string(notepad_buffer, NOTEPAD_X + 12, NOTEPAD_Y + 32, PAL_BLACK);
    } else {
        draw_string("Start typing your document here...", NOTEPAD_X + 12, NOTEPAD_Y + 32, PAL_LIGHT_GRAY);
    }

    // Interactive Action Save Button Widget
    int bx = NOTEPAD_X + 10;
    int by = NOTEPAD_Y + NOTEPAD_HEIGHT - 30;
    draw_rect(bx, by, 90, 22, PAL_LIGHT_GRAY);
    draw_rect(bx, by, 90, 2, PAL_WHITE);
    draw_rect(bx + 88, by, 2, 22, PAL_DARK_GRAY);
    draw_rect(bx, by + 20, 90, 2, PAL_DARK_GRAY);
    draw_string("Save File", bx + 10, by + 5, PAL_BLACK);

    if (num_widgets < 45) {
        widgets[num_widgets].x = bx; widgets[num_widgets].y = by;
        widgets[num_widgets].w = 90; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "SAVE_NOTE");
        widgets[num_widgets].action_id = 77;
        num_widgets++;
    }
}

void draw_start_menu(void) {
    // Elegant, geometric classic dropdown window pane matching our desktop styles
    int mx = 5;
    int my = 28;
    int mw = 150;
    int mh = 80;
    
    draw_rect(mx, my, mw, mh, PAL_LIGHT_GRAY);
    draw_rect(mx, my, mw, 2, PAL_WHITE);
    draw_rect(mx, my, 2, mh, PAL_WHITE);
    draw_rect(mx + mw - 2, my, 2, mh, PAL_DARK_GRAY);
    draw_rect(mx, my + mh - 2, mw, 2, PAL_DARK_GRAY);

    // Option rows
    draw_string("1. File Manager", mx + 10, my + 10, PAL_BLACK);
    draw_string("2. Notepad", mx + 10, my + 32, PAL_BLACK);
    draw_string("3. Clear Screen", mx + 10, my + 54, PAL_BLACK);

    // Register active hit test regions inside our start items array mappings
    if (num_widgets < 45) {
        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 5;
        widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_FILE");
        widgets[num_widgets].action_id = 101; num_widgets++;
        
        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 27;
        widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_NOTE");
        widgets[num_widgets].action_id = 102; num_widgets++;

        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 49;
        widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_CLEAR");
        widgets[num_widgets].action_id = 103; num_widgets++;
    }
}

void desktop_redraw_pipeline(void) {
    num_widgets = 0;
    
    // Clear the core primary wallpaper area cleanly to avoid flashes
    draw_rect(0, 0, fb_width, fb_height, WIN1_BACKGROUND);

    // Upper Shell Status Taskbar Element Strips
    draw_rect(0, 0, fb_width, 28, WIN1_HEADER);
    draw_rect(0, 26, fb_width, 2, WIN1_BORDER);
    
    // Corporate Icon Logo Circle acting directly as the Core Trigger Switch
    draw_bubble_logo(20, 13, 10, PAL_LIGHT_CYAN);
    draw_string("Start", 36, 8, PAL_WHITE);
    
    if (num_widgets < 45) {
        widgets[num_widgets].x = 5; widgets[num_widgets].y = 2;
        widgets[num_widgets].w = 80; widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "START_BTN");
        widgets[num_widgets].action_id = 99;
        num_widgets++;
    }

    draw_string("Uble Graphical Core System Environment", 160, 8, PAL_WHITE);

    // Conditionally compose open graphical elements onto viewport buffers
    if (explorer_open) { draw_file_manager(); }
    if (notepad_open)   { draw_notepad(); }
    if (start_menu_open){ draw_start_menu(); }

    // Permanent Embedded Terminal Canvas Frame Configuration
    draw_custom_window(TERM_X, TERM_Y, TERM_WIDTH, TERM_HEIGHT, "Terminal", 44);
    draw_rect(TERM_X + 4, TERM_Y + 22, TERM_WIDTH - 8, TERM_HEIGHT - 26, PAL_BLACK); 
}

/* Click Action Routing Core */
void desktop_evaluate_click(int x, int y) {
    for (int i = 0; i < num_widgets; i++) {
        if (x >= widgets[i].x && x <= (widgets[i].x + widgets[i].w) &&
            y >= widgets[i].y && y <= (widgets[i].y + widgets[i].h)) {
            
            if (widgets[i].action_id == 99) {
                start_menu_open = !start_menu_open;
                desktop_redraw_pipeline();
                return;
            }
            if (widgets[i].action_id == 44) {
                // Clicking close on Terminal hides text context lines cleanly
                draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
                term_cursor_x = TERM_X + 5; term_cursor_y = TERM_Y + 25;
                return;
            }
            if (widgets[i].action_id == 55) {
                explorer_open = false; start_menu_open = false;
                desktop_redraw_pipeline(); return;
            }
            if (widgets[i].action_id == 66) {
                notepad_open = false; start_menu_open = false;
                desktop_redraw_pipeline(); return;
            }
            if (widgets[i].action_id == 101) {
                explorer_open = true; start_menu_open = false;
                desktop_redraw_pipeline(); return;
            }
            if (widgets[i].action_id == 102) {
                notepad_open = true; start_menu_open = false;
                desktop_redraw_pipeline(); return;
            }
            if (widgets[i].action_id == 103) {
                draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
                term_cursor_x = TERM_X + 5; term_cursor_y = TERM_Y + 25;
                start_menu_open = false; desktop_redraw_pipeline(); return;
            }
            if (widgets[i].action_id == 77) {
                // Generate a true native file name string using counting tracks
                char filename_buf[32] = "note";
                int count_cpy = notepad_saved_count;
                int fidx = 4;
                if(count_cpy == 0) { filename_buf[fidx++] = '0'; }
                else {
                    while(count_cpy > 0 && fidx < 20) { filename_buf[fidx++] = (count_cpy % 10) + '0'; count_cpy /= 10; }
                }
                filename_buf[fidx++] = '.'; filename_buf[fidx++] = 't'; filename_buf[fidx++] = 'x'; filename_buf[fidx++] = 't';
                filename_buf[fidx] = '\0';

                int n_idx = fs_create(filename_buf);
                if (n_idx != -1) {
                    if (strlen(notepad_buffer) > 0) { strcpy(files[n_idx].data, notepad_buffer); }
                    else { strcpy(files[n_idx].data, "Empty notepad document text."); }
                    fs_save_to_disk();
                    notepad_saved_count++;
                    
                    term_cursor_x = TERM_X + 5;
                    print_string("Saved document to device partition LBA tracks as: ");
                    print_string(filename_buf); print_string("\n");
                }
                
                notepad_buffer[0] = '\0'; notepad_buf_idx = 0;
                start_menu_open = false;
                desktop_redraw_pipeline();
                return;
            }
            return;
        }
    }
    // Clicking empty space on desktop dimisses dropdown menus
    if (start_menu_open) { start_menu_open = false; desktop_redraw_pipeline(); }
}

/* ==== 17. MAIN EXECUTION SYSTEM ENTRY POINT ==== */
void kernel_main(MultibootInfo* mbi) {
    if (!(mbi->flags & (1 << 11))) { while(1) { __asm__ volatile("cli; hlt"); } }

    fb_mem    = (uint32_t*)((uint32_t)mbi->framebuffer_addr_lower);
    fb_width  = mbi->framebuffer_width;
    fb_height = mbi->framebuffer_height;
    fb_pitch  = mbi->framebuffer_pitch;

    mouse_init();
    users_load_from_disk();
    if (!fs_load_from_disk()) { fs_save_to_disk(); }

    // Seed clean user workspace data configurations safely
    if (fs_find("readme.txt") == -1) {
        int idx = fs_create("readme.txt");
        if(idx != -1) strcpy(files[idx].data, "Welcome to your clean independent desktop build environment!");
    }

    desktop_redraw_pipeline();
    save_mouse_backbuffer();

    static char main_cmd_buf[128];
    int cmd_idx = 0;
    main_cmd_buf[0] = '\0';

    // Baseline application operational shell string prompt
    draw_string("Uble:/$ ", TERM_X + 5, term_cursor_y, PAL_LIGHT_GREEN);

    while (1) {
        mouse_poll_cycle();

        // Localized pointer trace updates bypassing background frame redraw iterations
        if (mouse_x != last_mouse_x || mouse_y != last_mouse_y) {
            clear_old_mouse();         
            save_mouse_backbuffer();   
            draw_mouse_cursor(mouse_x, mouse_y); 
            last_mouse_x = mouse_x; last_mouse_y = mouse_y;
        }

        if (mouse_click && !prev_click) {
            clear_old_mouse();
            desktop_evaluate_click(mouse_x, mouse_y);
            save_mouse_backbuffer();
            draw_mouse_cursor(mouse_x, mouse_y);
        }
        prev_click = mouse_click;

        char key = get_key();
        if (key != 0) {
            if (notepad_open) {
                // Direct typing captures cleanly inside notepad buffer fields
                if (key == '\n') {
                    if (notepad_buf_idx < 500) { notepad_buffer[notepad_buf_idx++] = ' '; notepad_buffer[notepad_buf_idx] = '\0'; }
                } else if (key == '\b') {
                    if (notepad_buf_idx > 0) { notepad_buffer[--notepad_buf_idx] = '\0'; }
                } else if (notepad_buf_idx < 60) { // Keep single row safety limit bounds
                    notepad_buffer[notepad_buf_idx++] = key; notepad_buffer[notepad_buf_idx] = '\0';
                }
                // Update local notepad canvas directly
                draw_rect(NOTEPAD_X + 6, NOTEPAD_Y + 26, NOTEPAD_WIDTH - 12, NOTEPAD_HEIGHT - 62, PAL_WHITE);
                if(notepad_buf_idx > 0) { draw_string(notepad_buffer, NOTEPAD_X + 12, NOTEPAD_Y + 32, PAL_BLACK); }
                else { draw_string("Start typing your document here...", NOTEPAD_X + 12, NOTEPAD_Y + 32, PAL_LIGHT_GRAY); }
} else {
                // Standard prompt tracking route for active execution input
                if (key == '\n') {
                    main_cmd_buf[cmd_idx] = '\0';
                    draw_rect(TERM_X + 5, term_cursor_y, TERM_WIDTH - 10, 12, PAL_BLACK);
                    term_cursor_x = TERM_X + 5;
                    
                    process_command(main_cmd_buf);
                    
                    cmd_idx = 0; main_cmd_buf[0] = '\0';
                    term_cursor_y += 12;
                    if (term_cursor_y >= (TERM_Y + TERM_HEIGHT - 15)) {
                        draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
                        term_cursor_y = TERM_Y + 25;
                    }
                    draw_string("Uble:/$ ", TERM_X + 5, term_cursor_y, PAL_LIGHT_GREEN);
                } else if (key == '\b') {
                    if (cmd_idx > 0) {
                        cmd_idx--; main_cmd_buf[cmd_idx] = '\0';
                        draw_rect(TERM_X + 5 + (8 * 8), term_cursor_y, TERM_WIDTH - 100, 12, PAL_BLACK);
                        draw_string(main_cmd_buf, TERM_X + 5 + (8 * 8), term_cursor_y, PAL_WHITE);
                    }
                } else if (cmd_idx < 60) {
                    main_cmd_buf[cmd_idx++] = key; main_cmd_buf[cmd_idx] = '\0';
                    draw_string(main_cmd_buf, TERM_X + 5 + (8 * 8), term_cursor_y, PAL_WHITE);
                }
            }
        }
        for (volatile int delay = 0; delay < 25000; delay++);
    }
}

/* ==== 18. SYSTEM POWER MANAGEMENT ==== */
void poweroff(void) {
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);
    __asm__ volatile ("cli; hlt");
}