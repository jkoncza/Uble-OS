/* ========================================================================
 * UBLE GRAPHICAL OPERATING SYSTEM - UPGRADED DESKTOP PERFORMANCE CORE
 * PIXEL-PERFECT FRAMEBUFFER DESKTOP ENVIRONMENT WITH GRAPHICAL STATE
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
    0,             // Linear Framebuffer Graphics Mode
    800,           // Screen Width (Pixels)
    600,           // Screen Height (Pixels)
    32             // 32-Bit ARGB layout
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
    uint32_t flags; uint32_t mem_lower; uint32_t mem_upper; uint32_t boot_device;
    uint32_t cmdline; uint32_t mods_count; uint32_t mods_addr; uint32_t syms[4];
    uint32_t mmap_length; uint32_t mmap_addr; uint32_t drives_length; uint32_t drives_addr;
    uint32_t config_table; uint32_t boot_loader_name; uint32_t apm_table;
    uint32_t vbe_control_info; uint32_t vbe_mode_info; uint16_t vbe_mode;
    uint16_t vbe_interface_seg; uint16_t vbe_interface_off; uint16_t vbe_interface_len;
    uint32_t framebuffer_addr_lower; uint32_t framebuffer_addr_upper;
    uint32_t framebuffer_pitch; uint32_t framebuffer_width; uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp; uint8_t  framebuffer_type;
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
    int len = 0; while (*s++) len++; return len;
}
int strncmp(const char* a, const char* b, int n) {
    while (n > 0 && *a && (*a == *b)) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

/* ==== 5. STORAGE STRUCTURE CONFIGURATIONS ==== */
#define MAX_FILES        512
#define MAX_FILENAME     64
#define MAX_FILE_DATA    16384  

typedef struct {
    char owner[MAX_USERNAME];
    char name[MAX_FILENAME]; // Stores full path format: /dir/file.txt or /dir/
    char data[MAX_FILE_DATA];
    uint32_t size;
    uint8_t used;
    uint8_t is_dir;
} File;

#define FS_LBA_START       2048u
#define USERS_MAGIC        0x55534552  
#define FS_SECTOR_SIZE     512u
#define FS_MAGIC           0x46534F32  

typedef struct {
    uint32_t magic; uint32_t version; uint32_t sectors;
} FSHeader;

#define FS_MAX_SECTORS     16384  
#define USERS_MAX_SECTORS  128
#define USERS_LBA_START    (FS_LBA_START + FS_MAX_SECTORS)

typedef struct {
    char username[MAX_USERNAME]; char password[MAX_PASSWORD]; uint8_t used;
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
static bool has_hard_drive = false;

/* Hardware Cursor Variables */
static int32_t mouse_x         = 400;
static int32_t mouse_y         = 300;
static int32_t last_mouse_x    = 400;
static int32_t last_mouse_y    = 300;
static bool    mouse_click     = false;
static bool    prev_click      = false;
static uint32_t mouse_back_buf[16 * 16]; 

/* Path Tracking Workspaces */
static char current_working_dir[128] = "/";

/* Graphical Terminal Window Bounds & Cursors */
#define TERM_X          15
#define TERM_Y          330
#define TERM_WIDTH      770
#define TERM_HEIGHT     255
static int term_cursor_x   = TERM_X + 5;
static int term_cursor_y   = TERM_Y + 25;
static bool terminal_open  = true;

/* File Manager UI Layouts */
#define EXPLORER_X      20
#define EXPLORER_Y      45
#define EXPLORER_WIDTH  370
#define EXPLORER_HEIGHT 270
static bool explorer_open = false;
static int selected_file_idx = -1;

/* Notepad UI Layouts */
#define NOTEPAD_X       400
#define NOTEPAD_Y       45
#define NOTEPAD_WIDTH   380
#define NOTEPAD_HEIGHT  270
static bool notepad_open = false;
static char notepad_buffer[2048] = "";
static int  notepad_buf_idx = 0;

/* Save Name Prompt Floating State Window */
static bool save_prompt_open = false;
static char save_name_buffer[64] = "";
static int  save_name_idx = 0;

/* Settings UI Bounds */
#define SETTINGS_X      220
#define SETTINGS_Y      100
#define SETTINGS_WIDTH  300
#define SETTINGS_HEIGHT 200
static bool settings_open = false;

/* Start Menu Interface State */
static bool start_menu_open = false;

/* UI Click Engine */
typedef struct {
    int x, y, w, h;
    char label[32];
    uint32_t action_id; 
    uint32_t linked_data;
} DesktopWidget;

static DesktopWidget widgets[60];
static int num_widgets = 0;

/* Declarations */
void users_save_to_disk(void); void users_load_from_disk(void);
bool fs_save_to_disk(void); bool fs_load_from_disk(void);
void desktop_redraw_pipeline(void);
void print_string(const char* s);
void put_char(char c);

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

/* ==== 9. LOW-LEVEL PIXEL PRIMITIVES ENGINES ==== */
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
    uint8_t glyph = (uint8_t)c; if (glyph >= 128) return;
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

/* Advanced Wrapping Engine for Notepad view limits */
void draw_string_wrapped(const char* s, uint32_t x_start, uint32_t y_start, uint32_t max_width, uint32_t color) {
    uint32_t cur_x = x_start;
    uint32_t cur_y = y_start;
    while (*s) {
        if (*s == '\n' || (cur_x + 8 >= x_start + max_width)) {
            cur_x = x_start;
            cur_y += 12;
        }
        if (*s != '\n') {
            draw_char(*s, cur_x, cur_y, color);
            cur_x += 8;
        }
        s++;
    }
}

void draw_bubble_logo(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    for (int y = -r; y <= (int)r; y++) {
        for (int x = -r; x <= (int)r; x++) {
            if (x*x + y*y <= (int)(r*r)) {
                if (x*x + y*y >= (int)((r-2)*(r-2))) { put_pixel(cx + x, cy + y, PAL_WHITE); }
                else if (x == -(int)(r/3) && y == -(int)(r/3)) {
                    put_pixel(cx + x, cy + y, PAL_WHITE); put_pixel(cx + x + 1, cy + y, PAL_WHITE);
                } else { put_pixel(cx + x, cy + y, color); }
            }
        }
    }
}
void draw_gear_icon(uint32_t x, uint32_t y, uint32_t color) {
    draw_rect(x + 5, y + 2, 4, 10, color); draw_rect(x + 2, y + 5, 10, 4, color);
    draw_rect(x + 3, y + 3, 8, 8, color); draw_rect(x + 5, y + 5, 4, 4, PAL_DARK_GRAY);
}
void int_to_str(int num, char* str) {
    int i = 0; if (num == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    int rem = num; char temp[16]; int t_idx = 0;
    while (rem > 0) { temp[t_idx++] = (rem % 10) + '0'; rem /= 10; }
    while (t_idx > 0) { str[i++] = temp[--t_idx]; }
    str[i] = '\0';
}

void draw_custom_window(int x, int y, int w, int h, const char* title, uint32_t close_act_id) {
    draw_rect(x, y, w, h, PAL_LIGHT_GRAY); 
    draw_rect(x, y, w, 22, WIN1_HEADER);   
    draw_rect(x, y, w, 2, WIN1_BORDER); draw_rect(x, y + 20, w, 2, WIN1_BORDER);
    draw_rect(x, y, 2, h, WIN1_BORDER); draw_rect(x + w - 2, y, 2, h, WIN1_BORDER);
    draw_rect(x, y + h - 2, w, 2, WIN1_BORDER);
    draw_string(title, x + 6, y + 6, PAL_WHITE);

    int cx = x + w - 18; int cy = y + 3;
    draw_rect(cx, cy, 14, 14, PAL_RED); draw_string("X", cx + 3, cy + 3, PAL_WHITE);

    if (num_widgets < 60) {
        widgets[num_widgets].x = cx; widgets[num_widgets].y = cy;
        widgets[num_widgets].w = 14; widgets[num_widgets].h = 14;
        strcpy(widgets[num_widgets].label, "CLOSE");
        widgets[num_widgets].action_id = close_act_id; widgets[num_widgets].linked_data = 0;
        num_widgets++;
    }
}

/* ==== 10. TERMINAL EMULATOR LOGIC ==== */
void put_char(char c) {
    if (!terminal_open) return;
    if (c == '\n') {
        term_cursor_x = TERM_X + 5; term_cursor_y += 12;
        if (term_cursor_y >= (TERM_Y + TERM_HEIGHT - 15)) {
            draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
            term_cursor_y = TERM_Y + 25;
        }
        return;
    } else if (c == '\b') {
        if (term_cursor_x > (TERM_X + 5)) {
            term_cursor_x -= 8; draw_rect(term_cursor_x, term_cursor_y, 8, 8, PAL_BLACK);
        }
        return;
    }
    draw_char(c, term_cursor_x, term_cursor_y, PAL_WHITE);
    term_cursor_x += 8;
    if (term_cursor_x >= (TERM_X + TERM_WIDTH - 10)) {
        term_cursor_x = TERM_X + 5; term_cursor_y += 12;
    }
}
void print_string(const char* s) { while (*s) { put_char(*s++); } }

/* ==== 11. PORT LEVEL I/O INTERFACE BUS ==== */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret; __asm__ volatile ("inb %1, %0" : "=a"(ret) : "dN"(port)); return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "dN"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t ret; __asm__ volatile ("inw %1, %0" : "=a"(ret) : "dN"(port)); return ret;
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" :: "a"(val), "dN"(port));
}

/* ==== 12. PS/2 MOUSE DRIVER ==== */
void mouse_wait(uint8_t type) {
    uint32_t timeout = 2000;
    while (timeout--) {
        if (type == 0) { if ((inb(0x64) & 1) == 1) return; } 
        else { if ((inb(0x64) & 2) == 0) return; }
    }
}
void mouse_write(uint8_t write) {
    mouse_wait(1); outb(0x64, 0xD4); mouse_wait(1); outb(0x60, write);
}
uint8_t mouse_read(void) { mouse_wait(0); return inb(0x60); }
void mouse_init(void) {
    mouse_wait(1); outb(0x64, 0xA8); mouse_wait(1); outb(0x64, 0x20);  
    mouse_wait(0); uint8_t status = (inb(0x60) | 2); 
    mouse_wait(1); outb(0x64, 0x60); mouse_wait(1); outb(0x60, status); 
    mouse_write(0xF6); mouse_read(); mouse_write(0xF4); mouse_read();     
}
void mouse_poll_cycle(void) {
    if ((inb(0x64) & 1) == 0) return; if (!(inb(0x64) & 0x20))  return; 
    uint8_t status = inb(0x60); int32_t rel_x  = mouse_read(); int32_t rel_y  = mouse_read();
    if (status & 0x40 || status & 0x80) return; 
    if (status & 0x10) rel_x |= 0xFFFFFF00; if (status & 0x20) rel_y |= 0xFFFFFF00;
    last_mouse_x = mouse_x; last_mouse_y = mouse_y; mouse_x += rel_x; mouse_y -= rel_y; 
    if (mouse_x < 0) mouse_x = 0; if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= (int32_t)fb_width - 12)  mouse_x = fb_width - 13;
    if (mouse_y >= (int32_t)fb_height - 16) mouse_y = fb_height - 17;
    prev_click = mouse_click; mouse_click = (status & 1) ? true : false;
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

/* ==== 14. ATA STORAGE DRIVER ENGINE ==== */
#define ATA_TIMEOUT 400
static bool ata_wait(uint8_t mask, uint8_t value) {
    for (int i = 0; i < ATA_TIMEOUT; i++) { if ((inb(0x1F7) & mask) == value) return true; } return false;
}
void check_hard_drive_presence(void) {
    uint8_t status = inb(0x1F7); has_hard_drive = (status == 0xFF || status == 0x00) ? false : true;
}
bool ata_read_sectors(uint32_t lba, uint8_t count, uint16_t* buf) {
    if (!has_hard_drive || count == 0) return false;
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba); outb(0x1F4, (uint8_t)(lba >> 8)); outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);
    for (int i = 0; i < count; i++) {
        if (!ata_wait(0x08, 0x08)) return false;
        for (int j = 0; j < 256; j++) buf[i * 256 + j] = inw(0x1F0);
    }
    return true;
}
bool ata_write_sectors(uint32_t lba, uint8_t count, uint16_t* buf) {
    if (!has_hard_drive || count == 0) return false;
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba); outb(0x1F4, (uint8_t)(lba >> 8)); outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);
    for (int i = 0; i < count; i++) {
        if (!ata_wait(0x08, 0x08)) return false;
        for (int j = 0; j < 256; j++) outw(0x1F0, buf[i * 256 + j]);
    }
    outb(0x1F7, 0xE7); ata_wait(0x80, 0x00); return true;
}

bool fs_save_to_disk(void) {
    if (!has_hard_drive) return false;
    uint32_t total_bytes = sizeof(FSHeader) + sizeof(files);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors > 40) sectors = 40;
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
    if (!has_hard_drive) return false;
    static uint8_t sector_buf[FS_SECTOR_SIZE];
    if (!ata_read_sectors(FS_LBA_START, 1, (uint16_t*)sector_buf)) return false;
    FSHeader* header = (FSHeader*)sector_buf;
    if (header->magic != FS_MAGIC || header->version != 2) return false;
    uint32_t total_sectors = header->sectors; uint32_t bytes_read = 0; uint32_t total_bytes = sizeof(FSHeader) + sizeof(files);
    if (total_sectors > 40) total_sectors = 40;
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
    if (!has_hard_drive) return;
    uint32_t total_bytes = sizeof(users_magic) + sizeof(users) + sizeof(num_users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    static uint16_t buffer[256 * 4]; uint8_t* dst = (uint8_t*)buffer;
    for (uint32_t i = 0; i < sectors * FS_SECTOR_SIZE; i++) dst[i] = 0;
    uint32_t off = 0;
    for (uint32_t i = 0; i < sizeof(users_magic); i++) dst[off++] = ((uint8_t*)&users_magic)[i];
    for (uint32_t i = 0; i < sizeof(users); i++) dst[off++] = ((uint8_t*)users)[i];
    for (uint32_t i = 0; i < sizeof(num_users); i++) dst[off++] = ((uint8_t*)&num_users)[i];
    ata_write_sectors(USERS_LBA_START, (uint8_t)sectors, buffer);
}
void users_load_from_disk(void) {
    if (!has_hard_drive) { num_users = 0; return; }
    uint32_t total_bytes = sizeof(users_magic) + sizeof(users) + sizeof(num_users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    static uint16_t buffer[256 * 4]; uint8_t* src = (uint8_t*)buffer;
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
int fs_create(const char* name, uint8_t is_dir) {
    if (fs_find(name) != -1) return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].is_dir = is_dir;
            strcpy(files[i].owner, "admin");
            strcpy(files[i].name, name);
            files[i].data[0] = '\0';
            files[i].size = 0;
            return i;
        }
    }
    return -1;
}

/* ==== 15. INTERACTIVE APPS & ADVANCED VIRTUAL SHELL ==== */
void build_absolute_path(const char* input, char* output) {
    if (input[0] == '/') {
        strcpy(output, input);
    } else {
        strcpy(output, current_working_dir);
        if (strcmp(current_working_dir, "/") != 0) {
            int len = strlen(output);
            output[len] = '/'; output[len+1] = '\0';
        }
        int out_len = strlen(output);
        strcpy(output + out_len, input);
    }
}

void split_first(const char* cmd, char* first, int first_max, const char** rest_out) {
    while (*cmd == ' ') cmd++;  
    int i = 0; while (*cmd && *cmd != ' ') { if (i < first_max - 1) first[i++] = *cmd; cmd++; }
    first[i] = '\0'; while (*cmd == ' ') cmd++; *rest_out = cmd;
}

void process_command(const char* cmdline) {
    char cmd[32]; const char* rest;
    if (!cmdline || !*cmdline) return;
    split_first(cmdline, cmd, sizeof(cmd), &rest);
    
    if (strcmp(cmd, "help") == 0) {
        print_string("Commands: help, clear, sysinfo, ls, mkdir, touch, cd, write, cat, rm\n");
    } else if (strcmp(cmd, "clear") == 0) {
        draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
        term_cursor_x = TERM_X + 5; term_cursor_y = TERM_Y + 25;
    } else if (strcmp(cmd, "sysinfo") == 0) {
        print_string("Uble OS Virtual Workspace Kernel Layer active.\n");
    } else if (strcmp(cmd, "mkdir") == 0) {
        if (!*rest) { print_string("Usage: mkdir <dirname>\n"); return; }
        char abs_path[128]; build_absolute_path(rest, abs_path);
        // Add trailing directory slash marker
        int len = strlen(abs_path);
        if (abs_path[len-1] != '/') { abs_path[len] = '/'; abs_path[len+1] = '\0'; }
        if (fs_create(abs_path, 1) != -1) { print_string("Directory created successfully.\n"); fs_save_to_disk(); }
        else { print_string("Error creating directory resource mapping.\n"); }
    } else if (strcmp(cmd, "touch") == 0) {
        if (!*rest) { print_string("Usage: touch <filename>\n"); return; }
        char abs_path[128]; build_absolute_path(rest, abs_path);
        if (fs_create(abs_path, 0) != -1) { print_string("File resource initialized.\n"); fs_save_to_disk(); }
        else { print_string("Error: Resource node conflicts or allocation full.\n"); }
    } else if (strcmp(cmd, "cd") == 0) {
        if (!*rest || strcmp(rest, "/") == 0) { strcpy(current_working_dir, "/"); return; }
        if (strcmp(rest, "..") == 0) {
            if (strcmp(current_working_dir, "/") == 0) return;
            int len = strlen(current_working_dir);
            int i = len - 2;
            while (i >= 0 && current_working_dir[i] != '/') i--;
            if (i < 0) i = 0;
            current_working_dir[i + 1] = '\0';
            return;
        }
        char abs_path[128]; build_absolute_path(rest, abs_path);
        int len = strlen(abs_path);
        if (abs_path[len-1] != '/') { abs_path[len] = '/'; abs_path[len+1] = '\0'; }
        int idx = fs_find(abs_path);
        if (idx != -1 && files[idx].is_dir) { strcpy(current_working_dir, abs_path); }
        else { print_string("Directory path descriptor not found.\n"); }
    } else if (strcmp(cmd, "ls") == 0) {
        print_string("Directory listing for "); print_string(current_working_dir); print_string(":\n");
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].used) {
                if (strncmp(files[i].name, current_working_dir, strlen(current_working_dir)) == 0) {
                    const char* sub = files[i].name + strlen(current_working_dir);
                    if (*sub != '\0') {
                        print_string(files[i].is_dir ? "[DIR] " : "[FILE] ");
                        print_string(sub); print_string("\n");
                    }
                }
            }
        }
    } else if (strcmp(cmd, "write") == 0) {
        char target_file[64]; const char* content;
        split_first(rest, target_file, sizeof(target_file), &content);
        if (!target_file[0] || !*content) { print_string("Usage: write <file> <text_content>\n"); return; }
        char abs_path[128]; build_absolute_path(target_file, abs_path);
        int idx = fs_find(abs_path);
        if (idx == -1) idx = fs_create(abs_path, 0);
        if (idx != -1) {
            strcpy(files[idx].data, content); files[idx].size = strlen(content);
            print_string("Data stream written onto node disk blocks.\n"); fs_save_to_disk();
        } else { print_string("Write processing error inside storage block allocations.\n"); }
    } else if (strcmp(cmd, "cat") == 0) {
        if (!*rest) { print_string("Usage: cat <filename>\n"); return; }
        char abs_path[128]; build_absolute_path(rest, abs_path);
        int idx = fs_find(abs_path);
        if (idx != -1 && !files[idx].is_dir) { print_string(files[idx].data); print_string("\n"); }
        else { print_string("Failed to fetch requested target binary or file mapping description.\n"); }
    } else if (strcmp(cmd, "rm") == 0) {
        if (!*rest) { print_string("Usage: rm <filename>\n"); return; }
        char abs_path[128]; build_absolute_path(rest, abs_path);
        int idx = fs_find(abs_path);
        if (idx == -1) {
            char try_dir[140]; strcpy(try_dir, abs_path); int len = strlen(try_dir);
            if(try_dir[len-1] != '/') { try_dir[len] = '/'; try_dir[len+1] = '\0'; }
            idx = fs_find(try_dir);
        }
        if (idx != -1) { files[idx].used = 0; print_string("Node successfully removed.\n"); fs_save_to_disk(); }
        else { print_string("Resource reference not found.\n"); }
    } else {
        print_string("Unknown operational layout instruction.\n");
    }
}

/* ==== 16. DESKTOP GRAPHICS PIPELINE & MANAGEMENT ==== */
void draw_file_manager(void) {
    draw_custom_window(EXPLORER_X, EXPLORER_Y, EXPLORER_WIDTH, EXPLORER_HEIGHT, "File Manager", 55);
    draw_rect(EXPLORER_X + 4, EXPLORER_Y + 22, EXPLORER_WIDTH - 8, EXPLORER_HEIGHT - 60, PAL_WHITE);
    
    // Header tags
    draw_string("Resource Name", EXPLORER_X + 12, EXPLORER_Y + 26, PAL_BLACK);
    draw_string("Type", EXPLORER_X + 240, EXPLORER_Y + 26, PAL_BLACK);
    draw_rect(EXPLORER_X + 10, EXPLORER_Y + 36, EXPLORER_WIDTH - 20, 1, PAL_DARK_GRAY);

    int print_y = EXPLORER_Y + 42; int items = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            uint32_t row_bg = (selected_file_idx == i) ? PAL_LIGHT_BLUE : PAL_LIGHT_GRAY;
            draw_rect(EXPLORER_X + 8, print_y - 2, EXPLORER_WIDTH - 16, 14, row_bg);
            draw_string(files[i].name, EXPLORER_X + 12, print_y, PAL_BLACK);
            draw_string(files[i].is_dir ? "Directory" : "Text Doc", EXPLORER_X + 240, print_y, PAL_DARK_GRAY);
            
            if (num_widgets < 60) {
                widgets[num_widgets].x = EXPLORER_X + 8; widgets[num_widgets].y = print_y - 2;
                widgets[num_widgets].w = EXPLORER_WIDTH - 16; widgets[num_widgets].h = 14;
                strcpy(widgets[num_widgets].label, "SELECT_ROW");
                widgets[num_widgets].action_id = 210; widgets[num_widgets].linked_data = i;
                num_widgets++;
            }
            print_y += 16; items++; if (items >= 10) break;
        }
    }
    if (items == 0) { draw_string("[Workspace Node List Empty]", EXPLORER_X + 60, EXPLORER_Y + 100, PAL_DARK_GRAY); }

    // Bottom action operations strip layout matrix
    int bx = EXPLORER_X + 6; int by = EXPLORER_Y + EXPLORER_HEIGHT - 32;
    // Button: New Folder
    draw_rect(bx, by, 85, 24, PAL_LIGHT_GRAY); draw_string("New Dir", bx + 10, by + 6, PAL_BLACK);
    if (num_widgets < 60) {
        widgets[num_widgets].x = bx; widgets[num_widgets].y = by; widgets[num_widgets].w = 85; widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "NEW_DIR_BTN"); widgets[num_widgets].action_id = 501; num_widgets++;
    }
    // Button: Rename
    draw_rect(bx + 90, by, 75, 24, PAL_LIGHT_GRAY); draw_string("Rename", bx + 100, by + 6, PAL_BLACK);
    if (num_widgets < 60) {
        widgets[num_widgets].x = bx + 90; widgets[num_widgets].y = by; widgets[num_widgets].w = 75; widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "RENAME_BTN"); widgets[num_widgets].action_id = 502; num_widgets++;
    }
    // Button: Delete
    draw_rect(bx + 170, by, 75, 24, PAL_LIGHT_GRAY); draw_string("Delete", bx + 180, by + 6, PAL_BLACK);
    if (num_widgets < 60) {
        widgets[num_widgets].x = bx + 170; widgets[num_widgets].y = by; widgets[num_widgets].w = 75; widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "DELETE_BTN"); widgets[num_widgets].action_id = 503; num_widgets++;
    }
}

void draw_notepad(void) {
    draw_custom_window(NOTEPAD_X, NOTEPAD_Y, NOTEPAD_WIDTH, NOTEPAD_HEIGHT, "Notepad Engine", 66);
    draw_rect(NOTEPAD_X + 6, NOTEPAD_Y + 26, NOTEPAD_WIDTH - 12, NOTEPAD_HEIGHT - 62, PAL_WHITE);
    
    // Clean Wrapped Document Content Stream Display Rendering Pipeline
    if (strlen(notepad_buffer) > 0) {
        draw_string_wrapped(notepad_buffer, NOTEPAD_X + 12, NOTEPAD_Y + 32, NOTEPAD_WIDTH - 24, PAL_BLACK);
    } else {
        draw_string("Compose regular layout documents...", NOTEPAD_X + 12, NOTEPAD_Y + 32, PAL_LIGHT_GRAY);
    }

    int bx = NOTEPAD_X + 10; int by = NOTEPAD_Y + NOTEPAD_HEIGHT - 30;
    draw_rect(bx, by, 90, 22, PAL_LIGHT_GRAY); draw_string("Save As...", bx + 10, by + 5, PAL_BLACK);

    if (num_widgets < 60) {
        widgets[num_widgets].x = bx; widgets[num_widgets].y = by;
        widgets[num_widgets].w = 90; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "PROMPT_SAVE");
        widgets[num_widgets].action_id = 700; widgets[num_widgets].linked_data = 0;
        num_widgets++;
    }
}

void draw_save_prompt(void) {
    int px = 250; int py = 200; int pw = 300; int ph = 120;
    draw_rect(px, py, pw, ph, PAL_LIGHT_GRAY);
    draw_rect(px, py, pw, 22, WIN1_HEADER);
    draw_string("Enter File Desired Name:", px + 10, py + 5, PAL_WHITE);
    
    // Text input box rectangle
    draw_rect(px + 15, py + 40, pw - 30, 24, PAL_WHITE);
    if(strlen(save_name_buffer) > 0) {
        draw_string(save_name_buffer, px + 20, py + 48, PAL_BLACK);
    }
    
    // Interactive Accept Action Box Button
    int bx = px + 100; int by = py + 80;
    draw_rect(bx, by, 100, 24, PAL_DARK_GRAY);
    draw_string("Commit Save", bx + 6, by + 6, PAL_WHITE);
    
    if (num_widgets < 60) {
        widgets[num_widgets].x = bx; widgets[num_widgets].y = by; widgets[num_widgets].w = 100; widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "COMMIT_SAVE");
        widgets[num_widgets].action_id = 711; widgets[num_widgets].linked_data = 0; num_widgets++;
    }
}

void draw_settings_panel(void) {
    draw_custom_window(SETTINGS_X, SETTINGS_Y, SETTINGS_WIDTH, SETTINGS_HEIGHT, "System Metrics", 88);
    draw_rect(SETTINGS_X + 6, SETTINGS_Y + 26, SETTINGS_WIDTH - 12, SETTINGS_HEIGHT - 32, PAL_WHITE);
    draw_string("Uble OS Virtual Storage Core", SETTINGS_X + 15, SETTINGS_Y + 35, PAL_BLACK);
    draw_rect(SETTINGS_X + 12, SETTINGS_Y + 48, SETTINGS_WIDTH - 24, 1, PAL_DARK_GRAY);
    draw_string("Memory Allocation: 32 MB Base RAM", SETTINGS_X + 15, SETTINGS_Y + 60, PAL_BLACK);
    int file_count = 0; for (int i = 0; i < MAX_FILES; i++) { if (files[i].used) file_count++; }
    char f_buf[16]; int_to_str(file_count, f_buf);
    draw_string("Active System Nodes: ", SETTINGS_X + 15, SETTINGS_Y + 85, PAL_BLACK);
    draw_string(f_buf, SETTINGS_X + 180, SETTINGS_Y + 85, PAL_BLUE);
}

void draw_start_menu(void) {
    int mx = 5; int my = 28; int mw = 160; int mh = 80;
    draw_rect(mx, my, mw, mh, PAL_LIGHT_GRAY);
    draw_string("1. File Manager", mx + 10, my + 10, PAL_BLACK);
    draw_string("2. Notepad", mx + 10, my + 32, PAL_BLACK);
    draw_string("3. Open Terminal", mx + 10, my + 54, PAL_BLACK);

    if (num_widgets < 60) {
        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 5; widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_FILE"); widgets[num_widgets].action_id = 101; num_widgets++;
        
        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 27; widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_NOTE"); widgets[num_widgets].action_id = 102; num_widgets++;

        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 49; widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_TERM"); widgets[num_widgets].action_id = 104; num_widgets++;
    }
}

void desktop_redraw_pipeline(void) {
    num_widgets = 0; draw_rect(0, 0, fb_width, fb_height, WIN1_BACKGROUND);
    
    // Upper System Desktop Status Bar Stripe
    draw_rect(0, 0, fb_width, 28, WIN1_HEADER); draw_rect(0, 26, fb_width, 2, WIN1_BORDER);
    draw_bubble_logo(20, 13, 10, PAL_LIGHT_CYAN); draw_string("Start", 36, 8, PAL_WHITE);
    if (num_widgets < 60) {
        widgets[num_widgets].x = 5; widgets[num_widgets].y = 2; widgets[num_widgets].w = 80; widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "START_BTN"); widgets[num_widgets].action_id = 99; num_widgets++;
    }

    int gx = fb_width - 30; draw_gear_icon(gx, 6, PAL_WHITE);
    if (num_widgets < 60) {
        widgets[num_widgets].x = gx - 2; widgets[num_widgets].y = 4; widgets[num_widgets].w = 18; widgets[num_widgets].h = 18;
        strcpy(widgets[num_widgets].label, "SET_GEAR"); widgets[num_widgets].action_id = 300; num_widgets++;
    }

    draw_string("Uble System OS", 160, 8, PAL_WHITE);
    draw_string("Path: ", 320, 8, PAL_LIGHT_GRAY); draw_string(current_working_dir, 370, 8, PAL_YELLOW);

    if (explorer_open)   { draw_file_manager(); }
    if (notepad_open)    { draw_notepad(); }
    if (settings_open)   { draw_settings_panel(); }
    if (start_menu_open) { draw_start_menu(); }
    if (save_prompt_open){ draw_save_prompt(); }

    if (terminal_open) {
        draw_custom_window(TERM_X, TERM_Y, TERM_WIDTH, TERM_HEIGHT, "Terminal Engine Workspace Console", 44);
        draw_rect(TERM_X + 4, TERM_Y + 22, TERM_WIDTH - 8, TERM_HEIGHT - 26, PAL_BLACK); 
    }
}

/* UI Click Event Routing Matrices */
void desktop_evaluate_click(int x, int y) {
    for (int i = 0; i < num_widgets; i++) {
        if (x >= widgets[i].x && x <= (widgets[i].x + widgets[i].w) &&
            y >= widgets[i].y && y <= (widgets[i].y + widgets[i].h)) {
            
            if (widgets[i].action_id == 99) { start_menu_open = !start_menu_open; desktop_redraw_pipeline(); return; }
            if (widgets[i].action_id == 300) { settings_open = !settings_open; start_menu_open = false; desktop_redraw_pipeline(); return; }
            if (widgets[i].action_id == 44) { terminal_open = false; desktop_redraw_pipeline(); return; }
            if (widgets[i].action_id == 55) { explorer_open = false; selected_file_idx = -1; desktop_redraw_pipeline(); return; }
            if (widgets[i].action_id == 66) { notepad_open = false; desktop_redraw_pipeline(); return; }
            if (widgets[i].action_id == 88) { settings_open = false; desktop_redraw_pipeline(); return; }
            if (widgets[i].action_id == 101) { explorer_open = true; start_menu_open = false; desktop_redraw_pipeline(); return; }
            if (widgets[i].action_id == 102) { notepad_open = true; start_menu_open = false; desktop_redraw_pipeline(); return; }
            if (widgets[i].action_id == 104) { terminal_open = true; start_menu_open = false; desktop_redraw_pipeline(); return; }
            
            if (widgets[i].action_id == 210) { // Select item inside the active grid list row
                selected_file_idx = widgets[i].linked_data;
                desktop_redraw_pipeline(); return;
            }
            if (widgets[i].action_id == 700) { // Initiate layout save name prompt window mapping
                save_prompt_open = true; save_name_buffer[0] = '\0'; save_name_idx = 0;
                desktop_redraw_pipeline(); return;
            }
            if (widgets[i].action_id == 711) { // Process file saving with input string layout name buffer 
                if (strlen(save_name_buffer) > 0) {
                    char abs_path[128]; build_absolute_path(save_name_buffer, abs_path);
                    int n_idx = fs_create(abs_path, 0);
                    if (n_idx != -1) {
                        strcpy(files[n_idx].data, notepad_buffer); files[n_idx].size = strlen(notepad_buffer);
                        fs_save_to_disk();
                    }
                }
                save_prompt_open = false; notepad_buffer[0] = '\0'; notepad_buf_idx = 0;
                desktop_redraw_pipeline(); return;
            }
            if (widgets[i].action_id == 501) { // New directory action trigger loop
                save_prompt_open = true; save_name_buffer[0] = '\0'; save_name_idx = 0; return;
            }
            if (widgets[i].action_id == 503) { // Clear selected workspace record item
                if (selected_file_idx != -1) {
                    files[selected_file_idx].used = 0; selected_file_idx = -1;
                    fs_save_to_disk(); desktop_redraw_pipeline();
                }
                return;
            }
            return;
        }
    }
    if (start_menu_open) { start_menu_open = false; desktop_redraw_pipeline(); }
}

/* ==== 17. PROCESS ENTRY MAIN LOOP EXECUTION ==== */
void kernel_main(MultibootInfo* mbi) {
    if (!(mbi->flags & (1 << 11))) { while(1) { __asm__ volatile("cli; hlt"); } }

    fb_mem    = (uint32_t*)((uint32_t)mbi->framebuffer_addr_lower);
    fb_width  = mbi->framebuffer_width;   fb_height = mbi->framebuffer_height;
    fb_pitch  = mbi->framebuffer_pitch;

    desktop_redraw_pipeline(); save_mouse_backbuffer();
    check_hard_drive_presence(); mouse_init();

    if (has_hard_drive) {
        users_load_from_disk();
        if (!fs_load_from_disk()) { fs_save_to_disk(); }
    }
    if (fs_find("/readme.txt") == -1) {
        int idx = fs_create("/readme.txt", 0);
        if(idx != -1) { strcpy(files[idx].data, "Welcome to the system environment core layout!"); files[idx].size = strlen(files[idx].data); }
    }

    desktop_redraw_pipeline();
    static char main_cmd_buf[128]; int cmd_idx = 0; main_cmd_buf[0] = '\0';

    term_cursor_x = TERM_X + 5;
    if (terminal_open) { draw_string("Uble:/$ ", TERM_X + 5, term_cursor_y, PAL_LIGHT_GREEN); }

    while (1) {
        mouse_poll_cycle();

        if (mouse_x != last_mouse_x || mouse_y != last_mouse_y) {
            clear_old_mouse(); save_mouse_backbuffer();   
            draw_mouse_cursor(mouse_x, mouse_y); last_mouse_x = mouse_x; last_mouse_y = mouse_y;
        }
        if (mouse_click && !prev_click) {
            clear_old_mouse(); desktop_evaluate_click(mouse_x, mouse_y);
            save_mouse_backbuffer(); draw_mouse_cursor(mouse_x, mouse_y);
        }
        prev_click = mouse_click;

        char key = get_key();
        if (key != 0) {
            if (save_prompt_open) { // Send keystrokes directly to the floating Save Name input prompt dialog box
                if (key == '\n') {
                    // Map name directly to dynamic creation loops
                    if (strlen(save_name_buffer) > 0) {
                        char abs_path[128]; build_absolute_path(save_name_buffer, abs_path);
                        // Check if we are generating a directory folder asset
                        if (notepad_open) {
                            int n_idx = fs_create(abs_path, 0);
                            if (n_idx != -1) {
                                strcpy(files[n_idx].data, notepad_buffer); files[n_idx].size = strlen(notepad_buffer);
                            }
                        } else { // Directory mapping action requested from manager click button layouts
                            int len = strlen(abs_path);
                            if(abs_path[len-1] != '/') { abs_path[len] = '/'; abs_path[len+1] = '\0'; }
                            fs_create(abs_path, 1);
                        }
                        fs_save_to_disk();
                    }
                    save_prompt_open = false; notepad_buffer[0] = '\0'; notepad_buf_idx = 0;
                    desktop_redraw_pipeline();
                } else if (key == '\b') {
                    if (save_name_idx > 0) { save_name_buffer[--save_name_idx] = '\0'; desktop_redraw_pipeline(); }
                } else if (save_name_idx < 30) {
                    save_name_buffer[save_name_idx++] = key; save_name_buffer[save_name_idx] = '\0';
                    desktop_redraw_pipeline();
                }
            }
            else if (notepad_open) {
                if (key == '\n') {
                    if (notepad_buf_idx < 2000) { notepad_buffer[notepad_buf_idx++] = '\n'; notepad_buffer[notepad_buf_idx] = '\0'; }
                } else if (key == '\b') {
                    if (notepad_buf_idx > 0) { notepad_buffer[--notepad_buf_idx] = '\0'; }
                } else if (notepad_buf_idx < 2000) {
                    notepad_buffer[notepad_buf_idx++] = key; notepad_buffer[notepad_buf_idx] = '\0';
                }
                // Refresh only text layout field safely
                draw_rect(NOTEPAD_X + 6, NOTEPAD_Y + 26, NOTEPAD_WIDTH - 12, NOTEPAD_HEIGHT - 62, PAL_WHITE);
                if(notepad_buf_idx > 0) { draw_string_wrapped(notepad_buffer, NOTEPAD_X + 12, NOTEPAD_Y + 32, NOTEPAD_WIDTH - 24, PAL_BLACK); }
                else { draw_string("Compose regular layout documents...", NOTEPAD_X + 12, NOTEPAD_Y + 32, PAL_LIGHT_GRAY); }
            } 
            else if (terminal_open) {
                if (key == '\n') {
                    main_cmd_buf[cmd_idx] = '\0';
                    draw_rect(TERM_X + 5, term_cursor_y, TERM_WIDTH - 10, 12, PAL_BLACK);
                    term_cursor_x = TERM_X + 5;
                    
                    process_command(main_cmd_buf);
                    
                    cmd_idx = 0; main_cmd_buf[0] = '\0'; term_cursor_y += 12;
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
        for (volatile int delay = 0; delay < 12000; delay++);
    }
}

void poweroff(void) {
    outw(0x604, 0x2000); outw(0xB004, 0x2000); outw(0x4004, 0x3400);
    __asm__ volatile ("cli; hlt");
}