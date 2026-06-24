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
char* strcat(char* dst, const char* src)
{
    char* p = dst;

    while (*p)
        p++;

    while (*src)
        *p++ = *src++;

    *p = '\0';

    return dst;
}

void* memcpy(void* dst, const void* src, int n)
{
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;

    while (n--)
        *d++ = *s++;

    return dst;
}

void* memset(void* dst, int value, int n)
{
    unsigned char* d = (unsigned char*)dst;

    while (n--)
        *d++ = (unsigned char)value;

    return dst;
}
void int_to_string(int n, char* str) {
    int i = 0;
    if (n == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    int temp = n;
    while (temp > 0) { i++; temp /= 10; }
    str[i] = '\0';
    while (n > 0) {
        str[--i] = (n % 10) + '0';
        n /= 10;
    }
}

/* ==== 5. STORAGE STRUCTURE CONFIGURATIONS ==== */
#define MAX_FILES        64
#define MAX_FILENAME     56
#define MAX_FILE_DATA    448

typedef struct {
    char owner[MAX_USERNAME];
    char name[MAX_FILENAME]; // Absolute path format: /desktop/file.txt, /desktop/dir/
    char data[MAX_FILE_DATA];
    uint32_t size;
    uint8_t used;
    uint8_t is_dir;
} File;

#define FS_LBA_START       2048u
#define USERS_MAGIC        0x55534552  
#define FS_SECTOR_SIZE     512u
#define FS_MAGIC           0x46534F32
#define PROMPT_MAX 64

typedef struct {
    uint32_t magic; uint32_t version; uint32_t sectors;
} FSHeader;

#define FS_MAX_SECTORS     16384  
#define USERS_MAX_SECTORS  128
#define USERS_LBA_START    (FS_LBA_START + FS_MAX_SECTORS)

typedef struct {
    char username[MAX_USERNAME]; char password[MAX_PASSWORD]; uint8_t used;
} User;

char main_cmd_buf[128];
int cmd_idx = 0;
/* ==== 6. COLOR PALETTE DEFINITIONS ==== */
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
#define PAL_DARK_BLUE     0x000044FF
#define PAL_LIGHT_GREEN   0x0055FF55
#define PAL_LIGHT_CYAN    0x0055FFFF
#define PAL_LIGHT_RED     0x00FF5555
#define PAL_LIGHT_MAGENTA 0x00FF55FF
#define PAL_YELLOW        0x00FFFF55
#define PAL_WHITE         0x00FFFFFF
#define PAL_DARK_BROWN    0x005A3A1A
#define PAL_DARK_YELLOW   0x00C59B27
#define PAL_DARK_WHITE    0x00DCDCDC
#define PAL_CYAN_BASE     0x008CEBFF
#define PAL_CYAN_LIGHT    0x00BFFAFF
#define PAL_CYAN_DARK     0x003CB0C8

#define WIN1_BACKGROUND   0x001A4D4D 
#define WIN1_HEADER       0x00000088 
#define WIN1_BORDER       0x00DDDDDD 

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

/* Path Tracking Workspaces - Default directly to the active desktop node */
static char current_working_dir[128] = "/desktop/";
// Dragging
static bool dragging_window = false;

static int drag_window_id = 0;

static int drag_offset_x = 0;
static int drag_offset_y = 0;

// Windows ID's
#define WIN_TERM      1
#define WIN_EXPLORER  2
#define WIN_NOTEPAD   3
#define WIN_SETTINGS  4

#define WIN_START_MENU 999

/* Graphical Terminal Window Bounds & Cursors */
static int term_x = 15;
static int term_y = 330;
#define TERM_WIDTH      770
#define TERM_HEIGHT     255
static int term_cursor_x = 0;
static int term_cursor_y = 0;
static bool terminal_open  = true;

#define TERM_HISTORY_SIZE 8192

static char terminal_history[TERM_HISTORY_SIZE];
static int terminal_history_len = 0;

#define TERM_BUFFER_SIZE 8192

static char term_buffer[TERM_BUFFER_SIZE];
static int term_buffer_len = 0;

void terminal_append(const char* text)
{
    if (!text)
        return;

    int len = strlen(text);

    if (term_buffer_len + len >= TERM_BUFFER_SIZE - 1)
        return;

    memcpy(
        term_buffer + term_buffer_len,
        text,
        len);

    term_buffer_len += len;
    term_buffer[term_buffer_len] = '\0';
}

/* File Manager UI Layouts */
static int explorer_x = 20;
static int explorer_y = 45;
#define EXPLORER_WIDTH  370
#define EXPLORER_HEIGHT 270
static bool explorer_open = true;
static int selected_file_idx = -1;

/* Notepad UI Layouts */
static int notepad_x = 400;
static int notepad_y = 45;
#define NOTEPAD_WIDTH   380
#define NOTEPAD_HEIGHT  270
static bool notepad_open = false;
static char notepad_buffer[2048] = "";
static int  notepad_buf_idx = 0;

/* Global Prompt Management State Window */
static bool save_prompt_open   = false;
static bool rename_prompt_open = false;
static bool mkdir_prompt_open  = false;
static char prompt_text_buffer[64] = "";
static int  prompt_text_idx = 0;

/* Settings UI Bounds */
static int settings_x = 220;
static int settings_y = 100;
#define SETTINGS_WIDTH  300
#define SETTINGS_HEIGHT 200
static bool settings_open = false;
static bool start_menu_open = false;

/* UI Click Engine Layout structures */
typedef struct {
    int x, y, w, h;
    char label[32];
    uint32_t action_id; 
    uint32_t linked_data;
} DesktopWidget;

static DesktopWidget widgets[80];
static int num_widgets = 0;

bool point_in_rect(
    int px,
    int py,
    int rx,
    int ry,
    int rw,
    int rh)
{
    return
        px >= rx &&
        px <  rx + rw &&
        py >= ry &&
        py <  ry + rh;
}

/* Forward Function Declarations */
void users_save_to_disk(void); void users_load_from_disk(void);
bool fs_save_to_disk(void); bool fs_load_from_disk(void);
void desktop_redraw_pipeline(void);
void print_string(const char* s);
void put_char(char c);

int window_z_order[4] =
{
    WIN_EXPLORER,
    WIN_NOTEPAD,
    WIN_SETTINGS,
    WIN_TERM
};

static int active_window = WIN_TERM;

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

/* Icons Graphical Renderers */
void draw_folder_icon(uint32_t x, uint32_t y) {
    draw_rect(x + 2, y + 3, 27, 22, PAL_DARK_BROWN);
    draw_rect(x + 3, y + 4, 8, 3, PAL_BROWN);
    draw_rect(x + 3, y + 4, 5, 1, PAL_WHITE);
    draw_rect(x + 3, y + 7, 25, 17, PAL_DARK_YELLOW);
    draw_rect(x + 3, y + 7, 25, 2, PAL_DARK_BROWN);
    draw_rect(x + 2, y + 11, 27, 14, PAL_YELLOW);
    draw_rect(x + 2, y + 11, 27, 1, PAL_WHITE);
    draw_rect(x + 2, y + 12, 1, 12, PAL_WHITE);
    draw_rect(x + 1, y + 11, 1, 14, PAL_DARK_BROWN);
    draw_rect(x + 29, y + 11, 1, 14, PAL_DARK_BROWN);
    draw_rect(x + 2, y + 25, 27, 1, PAL_DARK_BROWN);
}

void draw_file_icon(uint32_t x, uint32_t y) {
    draw_rect(x + 2, y + 1, 28, 28, PAL_BLACK);
    draw_rect(x + 3, y + 2, 26, 26, PAL_WHITE);
    draw_rect(x + 3, y + 2, 26, 1, PAL_LIGHT_GRAY);
    draw_rect(x + 3, y + 3, 1, 25, PAL_LIGHT_GRAY);
    draw_rect(x + 24, y + 1, 6, 6, WIN1_BACKGROUND);
    draw_rect(x + 23, y + 2, 1, 5, PAL_BLACK);
    draw_rect(x + 23, y + 6, 6, 1, PAL_BLACK);
    draw_rect(x + 24, y + 2, 5, 4, PAL_LIGHT_GRAY);
    draw_rect(x + 6, y + 9, 12, 1, PAL_BLACK);
    draw_rect(x + 19, y + 9, 5, 1, PAL_BLACK);
    draw_rect(x + 6, y + 12, 7, 1, PAL_BLACK);
    draw_rect(x + 14, y + 12, 9, 1, PAL_BLACK);
    draw_rect(x + 6, y + 15, 16, 1, PAL_BLACK);
    draw_rect(x + 6, y + 18, 4, 1, PAL_BLACK);
    draw_rect(x + 6, y + 21, 14, 1, PAL_BLACK);
    draw_rect(x + 21, y + 21, 3, 1, PAL_BLACK);
    draw_rect(x + 6, y + 24, 11, 1, PAL_BLACK);
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

    if (num_widgets < 80) {
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
    if (terminal_history_len < TERM_HISTORY_SIZE - 1)
    {
        terminal_history[terminal_history_len++] = c;
        terminal_history[terminal_history_len] = '\0';
    }
    if (c == '\n') {
        term_cursor_x = term_x + 5; term_cursor_y += 12;
        if (term_cursor_y >= (term_y + TERM_HEIGHT - 15)) {
            draw_rect(term_x + 4, term_y + 22, TERM_WIDTH - 8, TERM_HEIGHT - 26, PAL_BLACK);
            term_cursor_y = term_y + 25;
        }
        return;
    } else if (c == '\b') {
        if (term_cursor_x > (term_x + 5)) {
            term_cursor_x -= 8; draw_rect(term_cursor_x, term_cursor_y, 8, 8, PAL_BLACK);
        }
        return;
    }
    draw_char(c, term_cursor_x, term_cursor_y, PAL_WHITE);
    term_cursor_x += 8;
    if (term_cursor_x >= (term_x + TERM_WIDTH - 10)) {
        term_cursor_x = term_x + 5; term_cursor_y += 12;
    }
}
void print_string(const char* s) { terminal_append(s); while (*s) { put_char(*s++); } }

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
    if ((inb(0x64) & 1) == 0) return; if (!(inb(0x64) & 0x20)) return; 
    uint8_t status = inb(0x60); int32_t rel_x  = mouse_read(); int32_t rel_y  = mouse_read();
    if (status & 0x40 || status & 0x80) return; 
    if (status & 0x10) rel_x |= 0xFFFFFF00; if (status & 0x20) rel_y |= 0xFFFFFF00;
    last_mouse_x = mouse_x; last_mouse_y = mouse_y; mouse_x += rel_x; mouse_y -= rel_y; 
    if (mouse_x < 0) mouse_x = 0; if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= (int32_t)fb_width - 12)  mouse_x = fb_width - 13;
    if (mouse_y >= (int32_t)fb_height - 16) mouse_y = fb_height - 17;
    prev_click = mouse_click; mouse_click = (status & 1) ? true : false;
    if (dragging_window && mouse_click)
    {
        int nx = mouse_x - drag_offset_x;
        int ny = mouse_y - drag_offset_y;

        switch (drag_window_id)
        {
            case WIN_EXPLORER:
                explorer_x = nx;
                explorer_y = ny;
                break;

            case WIN_NOTEPAD:
                notepad_x = nx;
                notepad_y = ny;
                break;

            case WIN_SETTINGS:
                settings_x = nx;
                settings_y = ny;
                break;

            case WIN_TERM:
            {
                int dx = nx - term_x;
                int dy = ny - term_y;

                term_x = nx;
                term_y = ny;

                term_cursor_x += dx;
                term_cursor_y += dy;
                break;
            }
        }

        desktop_redraw_pipeline();
    }
    if (!mouse_click)
    {
        dragging_window = false;
    }
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
    /* Improved drive probe using ATA IDENTIFY command. This is more reliable
       than a single status read and helps detect real writable drives. */
    has_hard_drive = false;
    outb(0x1F6, 0xA0); /* select drive 0, head bits */
    outb(0x1F2, 0); outb(0x1F3, 0); outb(0x1F4, 0); outb(0x1F5, 0);
    outb(0x1F7, 0xEC); /* IDENTIFY DEVICE */
    /* Wait for BSY clear */
    if (!ata_wait(0x80, 0x00)) return;
    uint8_t status = inb(0x1F7);
    if (status == 0xFF || status == 0x00) return;
    /* If ERR is set, IDENTIFY failed */
    if (status & 0x01) return;
    /* At this point device responded to IDENTIFY, consider it present */
    has_hard_drive = true;
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

void users_save_to_disk(void) {
    if (!has_hard_drive) return;
    typedef struct { uint32_t magic; uint32_t version; uint32_t count; } UsersHeader;
    UsersHeader header = { USERS_MAGIC, 1, (uint32_t)num_users };
    uint32_t total_bytes = sizeof(UsersHeader) + sizeof(users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors > USERS_MAX_SECTORS) sectors = USERS_MAX_SECTORS;
    static uint8_t sector_buf[FS_SECTOR_SIZE]; uint32_t bytes_written = 0;
    for (uint32_t s = 0; s < sectors; s++) {
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) sector_buf[i] = 0;
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) {
            if (bytes_written < sizeof(UsersHeader)) sector_buf[i] = ((uint8_t*)&header)[bytes_written];
            else if (bytes_written < total_bytes) sector_buf[i] = ((uint8_t*)users)[bytes_written - sizeof(UsersHeader)];
            else break;
            bytes_written++;
        }
        if (!ata_write_sectors(USERS_LBA_START + s, 1, (uint16_t*)sector_buf)) return;
    }
}

void users_load_from_disk(void) {
    if (!has_hard_drive) return;
    static uint8_t sector_buf[FS_SECTOR_SIZE];
    if (!ata_read_sectors(USERS_LBA_START, 1, (uint16_t*)sector_buf)) return;
    typedef struct { uint32_t magic; uint32_t version; uint32_t count; } UsersHeader;
    UsersHeader* header = (UsersHeader*)sector_buf;
    if (header->magic != USERS_MAGIC) return;
    uint32_t total_sectors = header->version ? header->version : 1; /* version field reused as sectors if needed */
    /* But fallback to max sectors reserved for users */
    if (total_sectors > USERS_MAX_SECTORS) total_sectors = USERS_MAX_SECTORS;
    uint32_t bytes_read = 0; uint32_t total_bytes = sizeof(UsersHeader) + sizeof(users);
    for (uint32_t s = 0; s < total_sectors; s++) {
        if (!ata_read_sectors(USERS_LBA_START + s, 1, (uint16_t*)sector_buf)) return;
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) {
            if (bytes_read >= sizeof(UsersHeader) && bytes_read < total_bytes) ((uint8_t*)users)[bytes_read - sizeof(UsersHeader)] = sector_buf[i];
            bytes_read++;
        }
    }
    /* Ensure num_users is sane */
    if (num_users < 0 || num_users > MAX_USERS) num_users = MAX_USERS;
}

bool fs_save_to_disk(void) {
    if (!has_hard_drive) return false;
    
    uint32_t total_bytes = sizeof(FSHeader) + sizeof(files);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    
    // Cap it at your defined partition limit instead of 40 sectors
    if (sectors > FS_MAX_SECTORS) sectors = FS_MAX_SECTORS;
    
    FSHeader header = { FS_MAGIC, 2, sectors };
    static uint8_t sector_buf[FS_SECTOR_SIZE]; 
    uint32_t bytes_written = 0;
    
    for (uint32_t s = 0; s < sectors; s++) {
        // Explicitly zero-fill the padding buffer area for this sector block
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) sector_buf[i] = 0;
        
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) {
            if (bytes_written < sizeof(FSHeader)) {
                sector_buf[i] = ((uint8_t*)&header)[bytes_written];
            } else if (bytes_written < total_bytes) {
                sector_buf[i] = ((uint8_t*)files)[bytes_written - sizeof(FSHeader)];
            }
            // Removed early "break" entirely so bytes_written pads out properly
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
    
    uint32_t total_sectors = header->sectors; 
    uint32_t bytes_read = 0; 
    uint32_t total_bytes = sizeof(FSHeader) + sizeof(files);
    
    // Safety check against absolute max partition limits instead of 40
    if (total_sectors > FS_MAX_SECTORS) total_sectors = FS_MAX_SECTORS;
    
    for (uint32_t s = 0; s < total_sectors; s++) {
        if (!ata_read_sectors(FS_LBA_START + s, 1, (uint16_t*)sector_buf)) return false;
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++) {
            if (bytes_read >= sizeof(FSHeader) && bytes_read < total_bytes) {
                ((uint8_t*)files)[bytes_read - sizeof(FSHeader)] = sector_buf[i];
            }
            bytes_read++;
        }
    }
    return true;
}

bool widget_visible(int index)
{
    int x = widgets[index].x + 2;
    int y = widgets[index].y + 2;

    for (int j = index + 1; j < num_widgets; j++)
    {
        if (x >= widgets[j].x &&
            x <= widgets[j].x + widgets[j].w &&
            y >= widgets[j].y &&
            y <= widgets[j].y + widgets[j].h)
        {
            return false;
        }
    }

    return true;
}

bool path_is_child(const char* parent,
                   const char* child)
{
    int len = strlen(parent);

    if (strncmp(parent,
                child,
                len) != 0)
    {
        return false;
    }

    return true;
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
            fs_save_to_disk();
            return i;
        }
    }
    return -1;
}

/* Recursive Cascading Directory Deletion Core */
void fs_delete_node_internal(
    int idx,
    int depth)
{
    if (idx < 0 || idx >= MAX_FILES)
        return;

    if (!files[idx].used)
        return;

    if (depth > MAX_FILES)
        return;

    if (files[idx].is_dir)
    {
        char prefix[128];
        strcpy(prefix, files[idx].name);

        int prefix_len = strlen(prefix);

        for (int i = 0; i < MAX_FILES; i++)
        {
            if (!files[i].used)
                continue;

            if (i == idx)
                continue;

            if (strncmp(
                    files[i].name,
                    prefix,
                    prefix_len) == 0)
            {
                fs_delete_node_internal(
                    i,
                    depth + 1);
            }
        }
    }
    files[idx].used = 0;
    files[idx].size = 0;
    files[idx].is_dir = 0;
    files[idx].name[0] = '\0';
    files[idx].data[0] = '\0';
}

void fs_delete_node(int idx)
{
    fs_delete_node_internal(
        idx,
        0);
}

void ensure_trailing_slash(char* path)
{
    int len = strlen(path);

    if (len <= 0 || len >= 127)
        return;

    if (path[len - 1] != '/')
    {
        path[len] = '/';
        path[len + 1] = '\0';
    }
}

void build_absolute_path(const char* input, char* output)
{
    if (!input || !output) return;

    output[0] = '\0';

    const char* base = input[0] == '/' ? "" : current_working_dir;

    strcpy(output, base);
    strcat(output, input);
}

void split_first(const char* cmd, char* first, int first_max, const char** rest_out) {
    while (*cmd == ' ') cmd++;  
    int i = 0; while (*cmd && *cmd != ' ') { if (i < first_max - 1) first[i++] = *cmd; cmd++; }
    first[i] = '\0'; while (*cmd == ' ') cmd++; *rest_out = cmd;
}

bool is_direct_child(const char* parent, const char* path)
{
    int parent_len = strlen(parent);

    if (strncmp(parent, path, parent_len) != 0)
        return false;

    const char* child = path + parent_len;

    if (*child == '\0')
        return false;

    while (*child)
    {
        if (*child == '/' && child[1] != '\0')
            return false;

        child++;
    }

    return true;
}

/* ==== 15. INTERACTIVE APPS & ADVANCED VIRTUAL SHELL ==== */
void process_command(const char* cmdline) {
    char cmd[32];
    const char* rest;

    if (!cmdline || !*cmdline) return;

    split_first(cmdline, cmd, sizeof(cmd), &rest);

    if (strcmp(cmd, "help") == 0) {
        print_string("Commands: help, clear, sysinfo, ls, mkdir, touch, cd, write, cat, rm\n");
    }

    else if (strcmp(cmd, "clear") == 0)
    {
        terminal_history_len = 0;
        terminal_history[0] = '\0';

        term_buffer[0] = '\0';
        term_buffer_len = 0;

        draw_rect(
            term_x + 4,
            term_y + 22,
            TERM_WIDTH - 8,
            TERM_HEIGHT - 26,
            PAL_BLACK);

        term_cursor_x = term_x + 5;
        term_cursor_y = term_y + 25;
    }

    else if (strcmp(cmd, "sysinfo") == 0) {
        print_string("Uble OS Operating Core Active.\n");
    }

    else if (strcmp(cmd, "mkdir") == 0) {
        if (!rest || !*rest) {
            print_string("Usage: mkdir <dirname>\n");
            return;
        }

        char abs_path[128];
        build_absolute_path(rest, abs_path);
        ensure_trailing_slash(abs_path);

        if (fs_create(abs_path, 1) != -1) {
            print_string("Dir created.\n");
            fs_save_to_disk();
        } else {
            print_string("Failed to create dir.\n");
        }
    }

    else if (strcmp(cmd, "touch") == 0) {
        if (!rest || !*rest) {
            print_string("Usage: touch <filename>\n");
            return;
        }

        char abs_path[128];
        build_absolute_path(rest, abs_path);

        if (fs_create(abs_path, 0) != -1) {
            print_string("File initialized.\n");
            fs_save_to_disk();
        } else {
            print_string("Failed initialized allocation.\n");
        }
    }

    else if (strcmp(cmd, "cd") == 0) {
        if (!rest || !*rest) {
            strcpy(current_working_dir, "/desktop/");
            return;
        }

        if (strcmp(rest, "..") == 0) {
            if (strcmp(current_working_dir, "/") == 0)
                return;

            int len = strlen(current_working_dir);
            int i = len - 2;

            while (i >= 0 && current_working_dir[i] != '/')
                i--;

            current_working_dir[i + 1] = '\0';
            return;
        }

        char abs_path[128];
        build_absolute_path(rest, abs_path);
        ensure_trailing_slash(abs_path);

        int idx = fs_find(abs_path);

        if (idx != -1 && files[idx].is_dir) {
            strcpy(current_working_dir, abs_path);
        } else {
            print_string("Invalid path descriptor.\n");
        }
    }

    else if (strcmp(cmd, "ls") == 0) {
        print_string("Content map of ");
        print_string(current_working_dir);
        print_string(":\n");

        for (int i = 0; i < MAX_FILES; i++) {
            if (!files[i].used) continue;

            if (is_direct_child(current_working_dir, files[i].name)) {
                const char* sub = files[i].name + strlen(current_working_dir);

                print_string(files[i].is_dir ? "[DIR] " : "[FILE] ");
                print_string(sub);
                print_string("\n");
            }
        }
    }

    else if (strcmp(cmd, "write") == 0) {
        char target_file[64];
        const char* content;

        split_first(rest, target_file, sizeof(target_file), &content);

        if (!target_file[0] || !content || !*content) {
            print_string("Usage: write <file> <text>\n");
            return;
        }

        char abs_path[128];
        build_absolute_path(target_file, abs_path);

        int idx = fs_find(abs_path);
        if (idx == -1) idx = fs_create(abs_path, 0);

        if (idx != -1) {
            int len = strlen(content);
            if (len >= MAX_FILE_DATA) len = MAX_FILE_DATA - 1;

            memcpy(files[idx].data, content, len);
            files[idx].data[len] = '\0';
            files[idx].size = len;

            fs_save_to_disk();
            print_string("Written.\n");
        }
    }

    else if (strcmp(cmd, "cat") == 0) {
        if (!rest || !*rest) {
            print_string("Usage: cat <filename>\n");
            return;
        }

        char abs_path[128];
        build_absolute_path(rest, abs_path);

        int idx = fs_find(abs_path);

        if (idx != -1 && !files[idx].is_dir) {
            print_string(files[idx].data);
            print_string("\n");
        } else {
            print_string("Target description error.\n");
        }
    }

    else if (strcmp(cmd, "rm") == 0) {
        if (!rest || !*rest) {
            print_string("Usage: rm <name>\n");
            return;
        }

        char abs_path[128];
        build_absolute_path(rest, abs_path);

        int idx = fs_find(abs_path);

        if (idx == -1) {
            char try_dir[140];
            strcpy(try_dir, abs_path);

            int len = strlen(try_dir);
            if (try_dir[len - 1] != '/') {
                try_dir[len] = '/';
                try_dir[len + 1] = '\0';
            }

            idx = fs_find(try_dir);
        }

        if (idx != -1) {
            fs_delete_node(idx);
            fs_save_to_disk();
            print_string("Removed.\n");
        } else {
            print_string("Not found.\n");
        }
    }

    else {
        print_string("Unknown layout instruction.\n");
    }
}
/* ==== 16. DESKTOP GRAPHICS PIPELINE & WINDOWS ==== */
void bring_window_to_front(int window_id)
{
    int pos = -1;

    for (int i = 0; i < 4; i++)
    {
        if (window_z_order[i] == window_id)
        {
            pos = i;
            break;
        }
    }

    if (pos == -1)
        return;

    for (int i = pos; i < 3; i++)
    {
        window_z_order[i] = window_z_order[i + 1];
    }

    window_z_order[3] = window_id;
    active_window = window_id;
}

void draw_file_manager(void)
{
    draw_custom_window(explorer_x, explorer_y, EXPLORER_WIDTH, EXPLORER_HEIGHT, "File Manager Workspace", 55);

    draw_rect(explorer_x + 4, explorer_y + 22, EXPLORER_WIDTH - 8, EXPLORER_HEIGHT - 60, PAL_WHITE);

    draw_string("Target Object Node Path", explorer_x + 12, explorer_y + 26, PAL_BLACK);
    draw_string("Type/Size", explorer_x + 245, explorer_y + 26, PAL_BLACK);

    draw_rect(explorer_x + 10, explorer_y + 36, EXPLORER_WIDTH - 20, 1, PAL_DARK_GRAY);

    int print_y = explorer_y + 42;
    int items = 0;

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (!files[i].used)
            continue;

        if (!path_is_child(current_working_dir, files[i].name))
            continue;

        const char* sub_item = files[i].name + strlen(current_working_dir);

        if (*sub_item == '\0')
            continue;

        bool nested = false;
        for (int k = 0; sub_item[k] != '\0'; k++)
        {
            if (sub_item[k] == '/')
            {
                if (sub_item[k + 1] != '\0')
                {
                    nested = true;
                    break;
                }
            }
        }

        if (nested)
            continue;

        uint32_t row_bg = (selected_file_idx == i) ? PAL_LIGHT_BLUE : PAL_WHITE;

        draw_rect(explorer_x + 6, print_y - 2, EXPLORER_WIDTH - 12, 14, row_bg);
        draw_string(sub_item, explorer_x + 12, print_y, PAL_BLACK);

        if (files[i].is_dir)
        {
            draw_string("Folder", explorer_x + 245, print_y, PAL_DARK_GRAY);
        }
        else
        {
            char size_str[16];
            int_to_string(files[i].size, size_str);

            int offset = strlen(size_str);
            if (offset < 15)
            {
                size_str[offset] = 'B';
                size_str[offset + 1] = '\0';
            }

            draw_string(size_str, explorer_x + 245, print_y, PAL_DARK_GRAY);
        }

        if (num_widgets < 80)
        {
            widgets[num_widgets].x = explorer_x + 6;
            widgets[num_widgets].y = print_y - 2;
            widgets[num_widgets].w = EXPLORER_WIDTH - 12;
            widgets[num_widgets].h = 14;

            strcpy(widgets[num_widgets].label, "SELECT_ROW");
            widgets[num_widgets].action_id = 210;
            widgets[num_widgets].linked_data = i;

            num_widgets++;
        }

        print_y += 16;

        items++;
        if (items >= 12)
            break;
    }

    if (items == 0)
    {
        draw_string("[Directory Empty]", explorer_x + 40, explorer_y + 80, PAL_DARK_GRAY);
    }

    // =========================
    // Bottom toolbar
    // =========================

    int bx = explorer_x + 6;
    int by = explorer_y + EXPLORER_HEIGHT - 32;

    draw_rect(bx, by, 75, 24, PAL_LIGHT_GRAY);
    draw_string("New Dir", bx + 10, by + 6, PAL_BLACK);

    if (num_widgets < 80)
    {
        widgets[num_widgets].x = bx;
        widgets[num_widgets].y = by;
        widgets[num_widgets].w = 75;
        widgets[num_widgets].h = 24;

        strcpy(widgets[num_widgets].label, "TRIGGER_MKDIR");
        widgets[num_widgets].action_id = 501;
        num_widgets++;
    }

    draw_rect(bx + 85, by, 75, 24, PAL_LIGHT_GRAY);
    draw_string("Rename", bx + 95, by + 6, PAL_BLACK);

    if (num_widgets < 80)
    {
        widgets[num_widgets].x = bx + 85;
        widgets[num_widgets].y = by;
        widgets[num_widgets].w = 75;
        widgets[num_widgets].h = 24;

        strcpy(widgets[num_widgets].label, "TRIGGER_RENAME");
        widgets[num_widgets].action_id = 502;
        num_widgets++;
    }

    draw_rect(bx + 170, by, 75, 24, PAL_LIGHT_GRAY);
    draw_string("Delete", bx + 180, by + 6, PAL_BLACK);

    if (num_widgets < 80)
    {
        widgets[num_widgets].x = bx + 170;
        widgets[num_widgets].y = by;
        widgets[num_widgets].w = 75;
        widgets[num_widgets].h = 24;

        strcpy(widgets[num_widgets].label, "TRIGGER_DELETE");
        widgets[num_widgets].action_id = 503;
        num_widgets++;
    }

    draw_rect(bx + 255, by, 50, 24, PAL_LIGHT_GRAY);
    draw_string("..", bx + 273, by + 6, PAL_BLACK);

    if (num_widgets < 80)
    {
        widgets[num_widgets].x = bx + 255;
        widgets[num_widgets].y = by;
        widgets[num_widgets].w = 50;
        widgets[num_widgets].h = 24;

        strcpy(widgets[num_widgets].label, "UP_DIR");
        widgets[num_widgets].action_id = 504;
        num_widgets++;
    }
}

void draw_notepad(void) {
    draw_custom_window(notepad_x, notepad_y, NOTEPAD_WIDTH, NOTEPAD_HEIGHT, "Notepad Document Writer", 66);
    draw_rect(notepad_x + 6, notepad_y + 26, NOTEPAD_WIDTH - 12, NOTEPAD_HEIGHT - 62, PAL_WHITE);
    
    if (strlen(notepad_buffer) > 0) {
        draw_string_wrapped(notepad_buffer, notepad_x + 12, notepad_y + 32, NOTEPAD_WIDTH - 24, PAL_BLACK);
    } else {
        draw_string("Type text document context streams...", notepad_x + 12, notepad_y + 32, PAL_LIGHT_GRAY);
    }

    int bx = notepad_x + 10; int by = notepad_y + NOTEPAD_HEIGHT - 30;
    draw_rect(bx, by, 90, 22, PAL_LIGHT_GRAY); draw_string("Save As...", bx + 10, by + 5, PAL_BLACK);

    if (num_widgets < 80) {
        widgets[num_widgets].x = bx; widgets[num_widgets].y = by;
        widgets[num_widgets].w = 90; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "PROMPT_SAVE");
        widgets[num_widgets].action_id = 700; widgets[num_widgets].linked_data = 0;
        num_widgets++;
    }
}

void draw_shared_prompt_box(const char* title, uint32_t commit_action_id) {
    int px = 250; int py = 200; int pw = 300; int ph = 120;
    draw_rect(px, py, pw, ph, PAL_LIGHT_GRAY);
    draw_rect(px, py, pw, 22, WIN1_HEADER);
    draw_string(title, px + 10, py + 5, PAL_WHITE);
    
    int cx = px + pw - 18; int cy = py + 3;
    draw_rect(cx, cy, 14, 14, PAL_RED); draw_string("X", cx + 3, cy + 3, PAL_WHITE);
    if (num_widgets < 80) {
        widgets[num_widgets].x = cx; widgets[num_widgets].y = cy; widgets[num_widgets].w = 14; widgets[num_widgets].h = 14;
        strcpy(widgets[num_widgets].label, "CLOSE_PROMPT"); widgets[num_widgets].action_id = 799; num_widgets++;
    }

    draw_rect(px + 15, py + 40, pw - 30, 24, PAL_WHITE);
    if(strlen(prompt_text_buffer) > 0) {
        draw_string(prompt_text_buffer, px + 20, py + 48, PAL_BLACK);
    }
    
    int bx = px + 100; int by = py + 80;
    draw_rect(bx, by, 100, 24, PAL_DARK_GRAY); draw_string("Confirm", bx + 22, by + 6, PAL_WHITE);
    if (num_widgets < 80) {
        widgets[num_widgets].x = bx; widgets[num_widgets].y = by; widgets[num_widgets].w = 100; widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "COMMIT_PROMPT_BTN"); widgets[num_widgets].action_id = commit_action_id; num_widgets++;
    }
}

void draw_desktop_icons_grid(void) {
    int start_x = 30; int start_y = 60;
    int current_y = start_y;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            if (strncmp(files[i].name, "/desktop/", 9) == 0) {
                const char* localized_name = files[i].name + 9;
                if (*localized_name == '\0' || strcmp(localized_name, "") == 0) continue;
                
                bool is_sub_child = false;
                for (int c = 0; localized_name[c] != '\0'; c++) {
                    if (localized_name[c] == '/' && localized_name[c+1] != '\0') {
                        is_sub_child = true; break;
                    }
                }
                if (is_sub_child) continue;

                if (files[i].is_dir) { draw_folder_icon(start_x, current_y); }
                else { draw_file_icon(start_x, current_y); }

                char clean_label[16]; int k = 0;
                for (; localized_name[k] != '\0' && localized_name[k] != '/' && k < 14; k++) {
                    clean_label[k] = localized_name[k];
                }
                clean_label[k] = '\0';
                draw_string(clean_label, start_x - 4, current_y + 30, PAL_WHITE);

                if (num_widgets < 80) {
                    widgets[num_widgets].x = start_x - 4;   // expand hitbox slightly left
                    widgets[num_widgets].y = current_y - 4; // expand hitbox slightly up
                    widgets[num_widgets].w = 48;            // wider clickable area
                    widgets[num_widgets].h = 56;            // taller clickable area
                    strcpy(widgets[num_widgets].label, "DESK_ICON");
                    widgets[num_widgets].action_id = 900;
                    widgets[num_widgets].linked_data = i;
                    num_widgets++;
                }
                current_y += 65; if (current_y >= 260) { current_y = start_y; start_x += 75; }
            }
        }
    }
}

void draw_settings_panel(void) {
    draw_custom_window(settings_x, settings_y, SETTINGS_WIDTH, SETTINGS_HEIGHT, "System Metrics Engine", 88);
    draw_rect(settings_x + 6, settings_y + 26, SETTINGS_WIDTH - 12, SETTINGS_HEIGHT - 32, PAL_WHITE);
    draw_string("Uble System OS Operational Core", settings_x + 15, settings_y + 35, PAL_BLACK);
    
    // Render Custom 12x12 Pixel-Art Configuration Gear Icon Graphic
    int gx = settings_x + SETTINGS_WIDTH - 30;
    int gy = settings_y + 35;
    draw_rect(gx + 4, gy + 0, 4, 12, PAL_DARK_GRAY);  // Vertical core
    draw_rect(gx + 0, gy + 4, 12, 4, PAL_DARK_GRAY);  // Horizontal core
    draw_rect(gx + 2, gy + 2, 8, 8, PAL_DARK_GRAY);    // Outer wheel
    draw_rect(gx + 4, gy + 4, 4, 4, PAL_WHITE);        // Center hole
    draw_rect(gx + 5, gy + 5, 2, 2, PAL_LIGHT_GRAY);   // Center depth pin

    // Compute active storage metrics directly across system records
    int total_files = 0;
    int total_folders = 0;
    uint32_t aggregated_bytes = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            if (files[i].is_dir) {
                total_folders++;
            } else {
                total_files++;
                aggregated_bytes += files[i].size;
            }
        }
    }

    // Convert values and render file layout statistics inside context view
    char buffer[32];
    
    draw_string("Storage Volume Structure:", settings_x + 15, settings_y + 60, PAL_DARK_GRAY);
    draw_rect(settings_x + 15, settings_y + 72, 180, 1, PAL_LIGHT_GRAY);

    draw_string("Total Data Files:", settings_x + 20, settings_y + 80, PAL_BLACK);
    int_to_string(total_files, buffer);
    draw_string(buffer, settings_x + 180, settings_y + 80, PAL_DARK_BLUE);

    draw_string("Total Directories:", settings_x + 20, settings_y + 96, PAL_BLACK);
    int_to_string(total_folders, buffer);
    draw_string(buffer, settings_x + 180, settings_y + 96, PAL_DARK_BLUE);

    draw_string("Aggregated Bytes Used:", settings_x + 20, settings_y + 112, PAL_BLACK);
    int_to_string((int)aggregated_bytes, buffer);
    int len = strlen(buffer);
    if (len < 31) {
        buffer[len] = 'B';
        buffer[len + 1] = '\0';
    }
    draw_string(buffer, settings_x + 180, settings_y + 112, PAL_RED);
}

void draw_start_menu(void) {
    int mx = 5; int my = 28; int mw = 160; int mh = 80;
    draw_rect(mx, my, mw, mh, PAL_LIGHT_GRAY);
    draw_string("1. File Manager", mx + 10, my + 10, PAL_BLACK);
    draw_string("2. Notepad", mx + 10, my + 32, PAL_BLACK);
    draw_string("3. Console View", mx + 10, my + 54, PAL_BLACK);

    if (num_widgets < 80) {
        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 5; widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_FILE"); widgets[num_widgets].action_id = 101; num_widgets++;
        
        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 27; widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_NOTE"); widgets[num_widgets].action_id = 102; num_widgets++;

        widgets[num_widgets].x = mx; widgets[num_widgets].y = my + 49; widgets[num_widgets].w = mw; widgets[num_widgets].h = 22;
        strcpy(widgets[num_widgets].label, "LAUNCH_TERM"); widgets[num_widgets].action_id = 104; num_widgets++;
    }
}

void redraw_terminal_contents(void)
{
    int x = term_x + 5;
    int y = term_y + 25;

    term_cursor_x = x;
    term_cursor_y = y;

    for (int i = 0; i < terminal_history_len; i++)
    {
        char c = terminal_history[i];

        if (c == '\n')
        {
            term_cursor_x = term_x + 5;
            term_cursor_y += 12;
        }
        else
        {
            draw_char(
                c,
                term_cursor_x,
                term_cursor_y,
                PAL_WHITE);

            term_cursor_x += 8;

            if (term_cursor_x >= (term_x + TERM_WIDTH - 10))
            {
                term_cursor_x = term_x + 5;
                term_cursor_y += 12;
            }
        }
    }
}

void desktop_redraw_pipeline(void)
{
    num_widgets = 0;
    memset(widgets, 0, sizeof(widgets));
    num_widgets = 0;

    draw_rect(0, 0, fb_width, fb_height, WIN1_BACKGROUND);

    draw_rect(0, 0, fb_width, 28, WIN1_HEADER);
    draw_rect(0, 26, fb_width, 2, WIN1_BORDER);

    // ==== START BUTTON GRAPHIC (UNCHANGED) ====
    draw_rect(13, 6,  6, 1, PAL_LIGHT_BLUE);
    draw_rect(13, 17, 6, 1, PAL_LIGHT_BLUE);
    draw_rect(10, 9,  1, 6, PAL_LIGHT_BLUE);
    draw_rect(21, 9,  1, 6, PAL_LIGHT_BLUE);
    draw_rect(11, 8,  2, 1, PAL_LIGHT_BLUE);
    draw_rect(11, 7,  1, 1, PAL_LIGHT_BLUE);
    draw_rect(12, 7,  1, 1, PAL_LIGHT_BLUE);
    draw_rect(19, 8,  2, 1, PAL_LIGHT_BLUE);
    draw_rect(20, 7,  1, 1, PAL_LIGHT_BLUE);
    draw_rect(19, 7,  1, 1, PAL_LIGHT_BLUE);
    draw_rect(11, 15, 2, 1, PAL_LIGHT_BLUE);
    draw_rect(11, 16, 1, 1, PAL_LIGHT_BLUE);
    draw_rect(12, 16, 1, 1, PAL_LIGHT_BLUE);
    draw_rect(19, 15, 2, 1, PAL_LIGHT_BLUE);
    draw_rect(20, 16, 1, 1, PAL_LIGHT_BLUE);
    draw_rect(19, 16, 1, 1, PAL_LIGHT_BLUE);
    draw_rect(13, 16, 6, 1, PAL_LIGHT_CYAN);
    draw_rect(20, 9,  1, 6, PAL_LIGHT_CYAN);
    draw_rect(19, 14, 1, 1, PAL_LIGHT_CYAN);
    draw_rect(13, 7,  3, 1, PAL_WHITE);
    draw_rect(11, 9,  1, 3, PAL_WHITE);
    draw_rect(12, 8,  2, 2, PAL_WHITE);

    draw_string("Start", 26, 8, PAL_WHITE);

    if (num_widgets < 80)
    {
        widgets[num_widgets].x = 5;
        widgets[num_widgets].y = 2;
        widgets[num_widgets].w = 85;
        widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "START_BTN");
        widgets[num_widgets].action_id = 99;
        num_widgets++;
    }

    draw_string("Workspace Context: ", 240, 8, PAL_LIGHT_GRAY);
    draw_string(current_working_dir, 390, 8, PAL_YELLOW);

    draw_desktop_icons_grid();

    for (int z = 0; z < 4; z++)
    {
        switch (window_z_order[z])
        {
            case WIN_EXPLORER:
                if (explorer_open)
                    draw_file_manager();
                break;

            case WIN_NOTEPAD:
                if (notepad_open)
                    draw_notepad();
                break;

            case WIN_SETTINGS:
                if (settings_open)
                    draw_settings_panel();
                break;

            case WIN_TERM:
            if (terminal_open)
            {
                draw_custom_window(
                    term_x,
                    term_y,
                    TERM_WIDTH,
                    TERM_HEIGHT,
                    "Terminal Subsystem Environment Viewport",
                    44);

                draw_rect(
                    term_x + 4,
                    term_y + 22,
                    TERM_WIDTH - 8,
                    TERM_HEIGHT - 26,
                    PAL_BLACK);

                    redraw_terminal_contents();

                draw_string(
                    "Uble:/$ ",
                    term_x + 5,
                    term_cursor_y,
                    PAL_LIGHT_GREEN);

                if (cmd_idx > 0)
                {
                    draw_string(
                        main_cmd_buf,
                        term_x + 5 + (8 * 8),
                        term_cursor_y,
                        PAL_WHITE);
                }
            }
            break;
        }
    }

    if (start_menu_open)
        draw_start_menu();

    if (save_prompt_open)
        draw_shared_prompt_box("Notepad: Save Target Name As", 711);

    if (mkdir_prompt_open)
        draw_shared_prompt_box("Manager: Create New Directory", 712);

    if (rename_prompt_open)
        draw_shared_prompt_box("Manager: Change Resource Identity", 713);
}

bool begin_window_drag(int mx,int my)
{
    if (terminal_open &&
        point_in_rect(
            mx,my,
            term_x,
            term_y,
            TERM_WIDTH,
            22))
    {
        bring_window_to_front(WIN_TERM);

        dragging_window = true;
        drag_window_id = WIN_TERM;

        drag_offset_x = mx - term_x;
        drag_offset_y = my - term_y;

        desktop_redraw_pipeline();
        return true;
    }

    if (settings_open &&
        point_in_rect(
            mx,my,
            settings_x,
            settings_y,
            SETTINGS_WIDTH,
            22))
    {
        bring_window_to_front(WIN_SETTINGS);

        dragging_window = true;
        drag_window_id = WIN_SETTINGS;

        drag_offset_x = mx - settings_x;
        drag_offset_y = my - settings_y;

        desktop_redraw_pipeline();
        return true;
    }

    if (notepad_open &&
        point_in_rect(
            mx,my,
            notepad_x,
            notepad_y,
            NOTEPAD_WIDTH,
            22))
    {
        bring_window_to_front(WIN_NOTEPAD);

        dragging_window = true;
        drag_window_id = WIN_NOTEPAD;

        drag_offset_x = mx - notepad_x;
        drag_offset_y = my - notepad_y;

        desktop_redraw_pipeline();
        return true;
    }

    if (explorer_open &&
        point_in_rect(
            mx,my,
            explorer_x,
            explorer_y,
            EXPLORER_WIDTH,
            22))
    {
        bring_window_to_front(WIN_EXPLORER);

        dragging_window = true;
        drag_window_id = WIN_EXPLORER;

        drag_offset_x = mx - explorer_x;
        drag_offset_y = my - explorer_y;

        desktop_redraw_pipeline();
        return true;
    }

    return false;
}

int get_top_window_at_point(int x, int y)
{
    if (start_menu_open &&
        point_in_rect(x, y, 5, 28, 160, 80))
    {
        return WIN_START_MENU;
    }

    for (int z = 3; z >= 0; z--)
    {
        int win = window_z_order[z];

        switch (win)
        {
            case WIN_TERM:
                if (terminal_open && point_in_rect(x, y, term_x, term_y, TERM_WIDTH, TERM_HEIGHT))
                    return WIN_TERM;
                break;

            case WIN_SETTINGS:
                if (settings_open && point_in_rect(x, y, settings_x, settings_y, SETTINGS_WIDTH, SETTINGS_HEIGHT))
                    return WIN_SETTINGS;
                break;

            case WIN_NOTEPAD:
                if (notepad_open && point_in_rect(x, y, notepad_x, notepad_y, NOTEPAD_WIDTH, NOTEPAD_HEIGHT))
                    return WIN_NOTEPAD;
                break;

            case WIN_EXPLORER:
                if (explorer_open && point_in_rect(x, y, explorer_x, explorer_y, EXPLORER_WIDTH, EXPLORER_HEIGHT))
                    return WIN_EXPLORER;
                break;
        }
    }
    return 0; // Click hit the empty desktop wallpaper
}

bool widget_clickable(int widget_index, int mouse_x, int mouse_y)
{
    int widget_window = 0;
    int wx = widgets[widget_index].x;
    int wy = widgets[widget_index].y;

    // Check widget ownership using Z-order (top-to-bottom)
    for (int z = 3; z >= 0; z--)
    {
        int win = window_z_order[z];
        if (win == WIN_EXPLORER && explorer_open &&
            point_in_rect(wx, wy, explorer_x, explorer_y, EXPLORER_WIDTH, EXPLORER_HEIGHT))
        {
            widget_window = WIN_EXPLORER;
            break;
        }
        if (win == WIN_NOTEPAD && notepad_open &&
            point_in_rect(wx, wy, notepad_x, notepad_y, NOTEPAD_WIDTH, NOTEPAD_HEIGHT))
        {
            widget_window = WIN_NOTEPAD;
            break;
        }
        if (win == WIN_SETTINGS && settings_open &&
            point_in_rect(wx, wy, settings_x, settings_y, SETTINGS_WIDTH, SETTINGS_HEIGHT))
        {
            widget_window = WIN_SETTINGS;
            break;
        }
        if (win == WIN_TERM && terminal_open &&
            point_in_rect(wx, wy, term_x, term_y, TERM_WIDTH, TERM_HEIGHT))
        {
            widget_window = WIN_TERM;
            break;
        }
    }

    int top_window = get_top_window_at_point(mouse_x, mouse_y);

    // If the window at the mouse coordinates is NOT the window that owns the widget, block it.
    if (widget_window != top_window)
    {
        return false;
    }

    if (widget_window == 0)
    {
        return true;
    }

    return (widget_window == top_window);
}
bool point_inside_any_window(int x, int y)
{
    if (explorer_open &&
        point_in_rect(
            x, y,
            explorer_x,
            explorer_y,
            EXPLORER_WIDTH,
            EXPLORER_HEIGHT))
    {
        return true;
    }

    if (notepad_open &&
        point_in_rect(
            x, y,
            notepad_x,
            notepad_y,
            NOTEPAD_WIDTH,
            NOTEPAD_HEIGHT))
    {
        return true;
    }

    if (settings_open &&
        point_in_rect(
            x, y,
            settings_x,
            settings_y,
            SETTINGS_WIDTH,
            SETTINGS_HEIGHT))
    {
        return true;
    }

    if (terminal_open &&
        point_in_rect(
            x, y,
            term_x,
            term_y,
            TERM_WIDTH,
            TERM_HEIGHT))
    {
        return true;
    }

    return false;
}

int get_window_at_point(int x, int y)
{
    for (int z = 3; z >= 0; z--)
    {
        int win = window_z_order[z];

        switch (win)
        {
            case WIN_EXPLORER:
                if (explorer_open &&
                    point_in_rect(x, y, explorer_x, explorer_y, EXPLORER_WIDTH, EXPLORER_HEIGHT))
                    return WIN_EXPLORER;
                break;

            case WIN_NOTEPAD:
                if (notepad_open &&
                    point_in_rect(x, y, notepad_x, notepad_y, NOTEPAD_WIDTH, NOTEPAD_HEIGHT))
                    return WIN_NOTEPAD;
                break;

            case WIN_SETTINGS:
                if (settings_open &&
                    point_in_rect(x, y, settings_x, settings_y, SETTINGS_WIDTH, SETTINGS_HEIGHT))
                    return WIN_SETTINGS;
                break;

            case WIN_TERM:
                if (terminal_open &&
                    point_in_rect(x, y, term_x, term_y, TERM_WIDTH, TERM_HEIGHT))
                    return WIN_TERM;
                break;
        }
    }
    return 0;
}

bool widget_is_on_top_window(int i, int mouse_x, int mouse_y)
{
    int id = widgets[i].action_id;

    // MODAL ALWAYS ACCEPTS INPUT (IMPORTANT)
    if (id == 700 || id == 501 || id == 502 ||
        id == 711 || id == 712 || id == 713 ||
        id == 799 || id == 504)
        return true;

    if (save_prompt_open || mkdir_prompt_open || rename_prompt_open)
    {
        return true;
    }

    int win = get_window_at_point(mouse_x, mouse_y);

    if (win == 0)
        return true;

    switch (win)
    {
        case WIN_EXPLORER:
            return point_in_rect(widgets[i].x, widgets[i].y,
                                 explorer_x, explorer_y,
                                 EXPLORER_WIDTH, EXPLORER_HEIGHT);

        case WIN_NOTEPAD:
            return point_in_rect(widgets[i].x, widgets[i].y,
                                 notepad_x, notepad_y,
                                 NOTEPAD_WIDTH, NOTEPAD_HEIGHT);

        case WIN_SETTINGS:
            return point_in_rect(widgets[i].x, widgets[i].y,
                                 settings_x, settings_y,
                                 SETTINGS_WIDTH, SETTINGS_HEIGHT);

        case WIN_TERM:
            return point_in_rect(widgets[i].x, widgets[i].y,
                                 term_x, term_y,
                                 TERM_WIDTH, TERM_HEIGHT);
    }

    return true;
}

bool is_point_covered_by_window(int x, int y)
{
    for (int z = 3; z >= 0; z--)
    {
        int win = window_z_order[z];

        switch (win)
        {
            case WIN_EXPLORER:
                if (explorer_open &&
                    point_in_rect(x, y,
                        explorer_x, explorer_y,
                        EXPLORER_WIDTH, EXPLORER_HEIGHT))
                    return true;
                break;

            case WIN_NOTEPAD:
                if (notepad_open &&
                    point_in_rect(x, y,
                        notepad_x, notepad_y,
                        NOTEPAD_WIDTH, NOTEPAD_HEIGHT))
                    return true;
                break;

            case WIN_SETTINGS:
                if (settings_open &&
                    point_in_rect(x, y,
                        settings_x, settings_y,
                        SETTINGS_WIDTH, SETTINGS_HEIGHT))
                    return true;
                break;

            case WIN_TERM:
                if (terminal_open &&
                    point_in_rect(x, y,
                        term_x, term_y,
                        TERM_WIDTH, TERM_HEIGHT))
                    return true;
                break;
        }
    }
    return false;
}

bool widget_is_visible_at_click(int i, int click_x, int click_y)
{
    if (save_prompt_open || mkdir_prompt_open || rename_prompt_open)
    {
        int id = widgets[i].action_id;

        if (id == 700 || id == 711 || id == 712 || id == 713 || id == 799)
        {
            return true;
        }

        return false;
    }
    int id = widgets[i].action_id;
    int top_window = get_top_window_at_point(click_x, click_y);

    int widget_window = 0;

    for (int z = 3; z >= 0; z--)
    {
        int win = window_z_order[z];
        if (win == WIN_EXPLORER && explorer_open &&
            point_in_rect(widgets[i].x, widgets[i].y, explorer_x, explorer_y, EXPLORER_WIDTH, EXPLORER_HEIGHT))
        {
            widget_window = WIN_EXPLORER;
            break;
        }
        if (win == WIN_NOTEPAD && notepad_open &&
            point_in_rect(widgets[i].x, widgets[i].y, notepad_x, notepad_y, NOTEPAD_WIDTH, NOTEPAD_HEIGHT))
        {
            widget_window = WIN_NOTEPAD;
            break;
        }
        if (win == WIN_SETTINGS && settings_open &&
            point_in_rect(widgets[i].x, widgets[i].y, settings_x, settings_y, SETTINGS_WIDTH, SETTINGS_HEIGHT))
        {
            widget_window = WIN_SETTINGS;
            break;
        }
        if (win == WIN_TERM && terminal_open &&
            point_in_rect(widgets[i].x, widgets[i].y, term_x, term_y, TERM_WIDTH, TERM_HEIGHT))
        {
            widget_window = WIN_TERM;
            break;
        }
    }

    if (top_window != widget_window)
    {
        return false;
    }

    if (widget_window == 0)
    {
        return (top_window == 0);
    }

    return (widget_window == top_window);
}

void desktop_evaluate_click(int x, int y)
{
    int handled = 0;

    bool modal_open = save_prompt_open || mkdir_prompt_open || rename_prompt_open;
    int clicked_window = get_top_window_at_point(x, y);
    if (start_menu_open)
    {
        if (point_in_rect(x, y, 5, 28, 160, 80))
        {
            clicked_window = 0;
        }
    }

    if (modal_open)
    {
        clicked_window = WIN_TERM; // dummy owner, prevents null routing issues
    }

    if (modal_open)
    {
        for (int i = num_widgets - 1; i >= 0; i--)
        {
            int id = widgets[i].action_id;

            // ONLY allow modal UI widgets to receive clicks when a modal dialogue box is open
            if (!(id == 700 || id == 799 || id == 711 || id == 712 || id == 713))
                continue;

            if (x < widgets[i].x || x > widgets[i].x + widgets[i].w ||
                y < widgets[i].y || y > widgets[i].y + widgets[i].h)
                continue;

            handled = 1;

            if (id == 799)
            {
                save_prompt_open = false;
                mkdir_prompt_open = false;
                rename_prompt_open = false;
            }
            else if (id == 700)
            {
                save_prompt_open = true;
                prompt_text_buffer[0] = '\0';
                prompt_text_idx = 0;
            }
            else if (id == 711)
            {
                if (strlen(prompt_text_buffer) > 0)
                {
                    char abs_path[128];
                    build_absolute_path(prompt_text_buffer, abs_path);
                    int n_idx = fs_find(abs_path);
                    if (n_idx == -1) n_idx = fs_create(abs_path, 0);
                    if (n_idx != -1)
                    {
                        strcpy(files[n_idx].data, notepad_buffer);
                        files[n_idx].size = strlen(notepad_buffer);
                        fs_save_to_disk();
                    }
                }
                save_prompt_open = false;
                prompt_text_buffer[0] = '\0';
                prompt_text_idx = 0;
            }
            else if (id == 712)
            {
                if (strlen(prompt_text_buffer) > 0)
                {
                    char abs_path[128];
                    build_absolute_path(prompt_text_buffer, abs_path);
                    int len = strlen(abs_path);
                    if (len > 0 && abs_path[len - 1] != '/')
                    {
                        abs_path[len] = '/';
                        abs_path[len + 1] = '\0';
                    }
                    if (fs_find(abs_path) == -1)
                    {
                        fs_create(abs_path, 1);
                        fs_save_to_disk();
                    }
                }
                mkdir_prompt_open = false;
                prompt_text_buffer[0] = '\0';
                prompt_text_idx = 0;
            }
            else if (id == 713)
            {
                if (selected_file_idx != -1 && strlen(prompt_text_buffer) > 0)
                {
                    char old_name[128];
                    strcpy(old_name, files[selected_file_idx].name);
                    char new_name[128];
                    build_absolute_path(prompt_text_buffer, new_name);

                    if (files[selected_file_idx].is_dir)
                    {
                        int len = strlen(new_name);
                        if (new_name[len - 1] != '/')
                        {
                            new_name[len] = '/';
                            new_name[len + 1] = '\0';
                        }
                    }

                    int collision = fs_find(new_name);
                    if (collision == -1 || collision == selected_file_idx)
                    {
                        if (files[selected_file_idx].is_dir)
                        {
                            int len = strlen(new_name);
                            if (new_name[len - 1] != '/')
                            {
                                new_name[len] = '/';
                                new_name[len + 1] = '\0';
                            }

                            int old_len = strlen(old_name);
                            char old_dir[128];
                            strcpy(old_dir, old_name);

                            if (old_dir[old_len - 1] != '/')
                            {
                                old_dir[old_len] = '/';
                                old_dir[old_len + 1] = '\0';
                                old_len++;
                            }

                            for (int f = 0; f < MAX_FILES; f++)
                            {
                                if (!files[f].used) continue;
                                if (strncmp(files[f].name, old_dir, old_len) == 0)
                                {
                                    char suffix[128];
                                    strcpy(suffix, files[f].name + old_len);
                                    char updated[128];
                                    strcpy(updated, new_name);
                                    strcat(updated, suffix);
                                    strcpy(files[f].name, updated);
                                }
                            }
                        }
                        else
                        {
                            strcpy(files[selected_file_idx].name, new_name);
                        }
                        fs_save_to_disk();
                    }
                    rename_prompt_open = false;
                    selected_file_idx = -1;
                }
                prompt_text_buffer[0] = '\0';
                prompt_text_idx = 0;
            }
            desktop_redraw_pipeline();
            return;
        }
        return; 
    }

    // Loop backward through registered widgets to process clicking targets
    for (int i = num_widgets - 1; i >= 0; i--)
    {
        int aid = widgets[i].action_id;

        // 1. CHOOSE WIDGET OWNER BASED ON ACTION ID (100% accurate, no geometry guessing)
        int widget_owner_window = 0;
        
        if (aid == 44) 
        {
            widget_owner_window = WIN_TERM;
        }
        else if (aid == 55 || aid == 210 || aid == 501 || aid == 502 || aid == 503 || aid == 504) 
        {
            widget_owner_window = WIN_EXPLORER;
        }
        else if (aid == 66 || aid == 700) 
        {
            widget_owner_window = WIN_NOTEPAD;
        }
        else if (aid == 88) 
        {
            widget_owner_window = WIN_SETTINGS;
        }
        else if (aid == 711 || aid == 712 || aid == 713 || aid == 799)
        {
            // Dialog contexts mirror the top-most visible layer
            widget_owner_window = clicked_window; 
        }
        else if (aid == 99 || aid == 101 || aid == 102 || aid == 104 || aid == 900) 
        {
            widget_owner_window = 0; // Desktop shortcuts / taskbar level
        }
        bool is_hidden_by_foreground_window = false;

        // 2. HARD LOCKDOWN: Find where this widget's window sits in the Z-order array stack
        int widget_z_index = -1;
        if (widget_owner_window == 0) 
        {
            widget_z_index = -1; // Ground floor (Wallpaper)
        } 
        else 
        {
            for (int z = 0; z < 4; z++) 
            {
                if (window_z_order[z] == widget_owner_window) 
                {
                    widget_z_index = z;
                    break;
                }
            }
        }

        bool widget_is_start_menu =
        (
        aid == 101 ||
        aid == 102 ||
        aid == 104
        );

        if (widget_is_start_menu && start_menu_open)
        {
            is_hidden_by_foreground_window = false;
        }

        // 3. VISUAL OCCLUSION CHECK: Check if any open window sitting HIGHER in the Z-stack 
        // completely covers the exact mouse click coordinate (x, y).

        if (!widget_is_start_menu)
        {
            for (int z = widget_z_index + 1; z < 4; z++)
        {
            int upper_win = window_z_order[z];
            if (upper_win == WIN_EXPLORER && explorer_open && point_in_rect(x, y, explorer_x, explorer_y, EXPLORER_WIDTH, EXPLORER_HEIGHT))
                is_hidden_by_foreground_window = true;
            if (upper_win == WIN_NOTEPAD && notepad_open && point_in_rect(x, y, notepad_x, notepad_y, NOTEPAD_WIDTH, NOTEPAD_HEIGHT))
                is_hidden_by_foreground_window = true;
            if (upper_win == WIN_SETTINGS && settings_open && point_in_rect(x, y, settings_x, settings_y, SETTINGS_WIDTH, SETTINGS_HEIGHT))
                is_hidden_by_foreground_window = true;
            if (upper_win == WIN_TERM && terminal_open && point_in_rect(x, y, term_x, term_y, TERM_WIDTH, TERM_HEIGHT))
                is_hidden_by_foreground_window = true;
        }
        }
        // If an overlapping foreground window hides this click point, drop the evaluation instantly!
        if (is_hidden_by_foreground_window)
        {
            continue;
        }

        // 4. HITBOX BOUNDS CHECK (Must click inside the button itself)
        if (x < widgets[i].x || x > widgets[i].x + widgets[i].w ||
            y < widgets[i].y || y > widgets[i].y + widgets[i].h)
        {
            continue;
        }

        handled = 1;

        if (clicked_window != 0)
        {
            active_window = clicked_window;
            bring_window_to_front(clicked_window);
        }

        /* ===== ACTION ROUTINES ===== */
        if (aid == 99)  start_menu_open = !start_menu_open;
        else if (aid == 44)  terminal_open = false;
        else if (aid == 55)  { explorer_open = false; selected_file_idx = -1; }
        else if (aid == 66)  notepad_open = false;
        else if (aid == 88)  settings_open = false;
        else if (aid == 101) explorer_open = true;
        else if (aid == 102) notepad_open = true;
        else if (aid == 104) terminal_open = true;
        else if (aid == 210)
        {
            selected_file_idx = widgets[i].linked_data;
            if (files[selected_file_idx].is_dir)
                strcpy(current_working_dir, files[selected_file_idx].name);
            else
            {
                strcpy(notepad_buffer, files[selected_file_idx].data);
                notepad_buf_idx = strlen(notepad_buffer);
                notepad_open = true;
            }
        }
        else if (aid == 900)
        {
            if (point_inside_any_window(x, y)) return;
            int idx = widgets[i].linked_data;
            if (files[idx].is_dir)
                strcpy(current_working_dir, files[idx].name);
            else
            {
                strcpy(notepad_buffer, files[idx].data);
                notepad_buf_idx = strlen(notepad_buffer);
                notepad_open = true;
            }
        }
        else if (aid == 700)
        {
            save_prompt_open = true;
            prompt_text_buffer[0] = '\0';
            prompt_text_idx = 0;
        }
        else if (aid == 501)
        {
            mkdir_prompt_open = true;
            prompt_text_buffer[0] = '\0';
            prompt_text_idx = 0;
        }
        else if (aid == 502)
        {
            if (selected_file_idx != -1)
            {
                rename_prompt_open = true;
                prompt_text_buffer[0] = '\0';
                prompt_text_idx = 0;
            }
        }
        else if (aid == 503)
        {
            if (selected_file_idx != -1)
            {
                fs_delete_node(selected_file_idx);
                selected_file_idx = -1;
                fs_save_to_disk();
            }
        }
        else if (aid == 504)
        {
            if (strcmp(current_working_dir, "/desktop/") != 0 && strcmp(current_working_dir, "/") != 0)
            {
                int len = strlen(current_working_dir);
                int j = len - 2;
                while (j >= 0 && current_working_dir[j] != '/') j--;
                current_working_dir[j + 1] = '\0';
            }
        }
        desktop_redraw_pipeline();
        return;
    }

    if (!handled)
    {
        if (begin_window_drag(x, y))
            return;

        if (point_inside_any_window(x, y))
            return;
    }

    if (!handled && start_menu_open)
    {
        start_menu_open = false;
        desktop_redraw_pipeline();
    }
}

/* ==== 17. PROCESS ENTRY MAIN LOOP EXECUTION ==== */
#define TERM_BOTTOM (term_y + TERM_HEIGHT - 20)
void kernel_main(MultibootInfo* mbi) {
    if (!(mbi->flags & (1 << 11))) { while(1) { __asm__ volatile("cli; hlt"); } }

    fb_mem    = (uint32_t*)((uint32_t)mbi->framebuffer_addr_lower);
    fb_width  = mbi->framebuffer_width;   fb_height = mbi->framebuffer_height;
    fb_pitch  = mbi->framebuffer_pitch;

    desktop_redraw_pipeline(); save_mouse_backbuffer();
    check_hard_drive_presence(); mouse_init();

    if (has_hard_drive) {
        if (!fs_load_from_disk()) { fs_save_to_disk(); }
        /* Load users table as well; if missing, create blank users sectors */
        users_load_from_disk();
    } else {
        /* No hard drive: create empty /desktop/ as a fallback for RAM-only mode */
        if (fs_find("/desktop/") == -1) { fs_create("/desktop/", 1); }
    }

    desktop_redraw_pipeline();
    term_cursor_x = term_x + 5;
    term_cursor_y = term_y + 25;
    main_cmd_buf[0] = '\0';

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
            bool prompt_active = (save_prompt_open || mkdir_prompt_open || rename_prompt_open);
            
            if (prompt_active) {
                if (key == '\n') {
                    if (save_prompt_open)   desktop_evaluate_click(250 + 150, 285);
                    if (mkdir_prompt_open)  desktop_evaluate_click(250 + 150, 285);
                    if (rename_prompt_open) desktop_evaluate_click(250 + 150, 285);
                } else if (key == '\b') {
                    if (prompt_text_idx > 0) { prompt_text_buffer[--prompt_text_idx] = '\0'; desktop_redraw_pipeline(); }
                } else if (prompt_text_idx < PROMPT_MAX - 1) {
                    prompt_text_buffer[prompt_text_idx++] = key;
                    prompt_text_buffer[prompt_text_idx] = '\0';
                    desktop_redraw_pipeline();
                }
            }
            else
            {
                int active_window = window_z_order[3];

                if (active_window == WIN_NOTEPAD && notepad_open)
                {
                    if (key == '\n')
                    {
                        if (notepad_buf_idx < 2000)
                        {
                            notepad_buffer[notepad_buf_idx++] = '\n';
                            notepad_buffer[notepad_buf_idx] = '\0';
                        }
                    }
                    else if (key == '\b')
                    {
                        if (notepad_buf_idx > 0)
                        {
                            notepad_buffer[--notepad_buf_idx] = '\0';
                        }
                    }
                    else if (notepad_buf_idx < 2000)
                    {
                        notepad_buffer[notepad_buf_idx++] = key;
                        notepad_buffer[notepad_buf_idx] = '\0';
                    }

                    draw_rect(
                        notepad_x + 6,
                        notepad_y + 26,
                        NOTEPAD_WIDTH - 12,
                        NOTEPAD_HEIGHT - 62,
                        PAL_WHITE);

                    if (notepad_buf_idx > 0)
                    {
                        draw_string_wrapped(
                            notepad_buffer,
                            notepad_x + 12,
                            notepad_y + 32,
                            NOTEPAD_WIDTH - 24,
                            PAL_BLACK);
                    }
                    else
                    {
                        draw_string(
                            "Type text document context streams...",
                            notepad_x + 12,
                            notepad_y + 32,
                            PAL_LIGHT_GRAY);
                    }
                }
                else if (active_window == WIN_TERM && terminal_open)
                {
                    if (key == '\n')
                    {
                        main_cmd_buf[cmd_idx] = '\0';

                        term_cursor_x = term_x + 5;
                        term_cursor_y += 12;

                        if (term_cursor_y >= TERM_BOTTOM)
                        {
                            draw_rect(
                                term_x + 4,
                                term_y + 22,
                                TERM_WIDTH - 8,
                                TERM_HEIGHT - 26,
                                PAL_BLACK);

                            term_cursor_y = term_y + 25;
                        }

                        char local_cmd[128];
                        strcpy(local_cmd, main_cmd_buf);

                        cmd_idx = 0;
                        main_cmd_buf[0] = '\0';

                        process_command(local_cmd);

                        desktop_redraw_pipeline();
                    }
                    else if (key == '\b')
                    {
                        if (cmd_idx > 0)
                        {
                            cmd_idx--;
                            main_cmd_buf[cmd_idx] = '\0';

                            desktop_redraw_pipeline();
                        }
                    }
                    else if (cmd_idx < 50)
                    {
                        main_cmd_buf[cmd_idx++] = key;
                        main_cmd_buf[cmd_idx] = '\0';

                        desktop_redraw_pipeline();
                    }
                }
            }
        }
        for (volatile int delay = 0; delay < 14000; delay++);
    }
}

void poweroff(void) {
    outw(0x604, 0x2000); outw(0xB004, 0x2000); outw(0x4004, 0x3400);
    __asm__ volatile ("cli; hlt");
}