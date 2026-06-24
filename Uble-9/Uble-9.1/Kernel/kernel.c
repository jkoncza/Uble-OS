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

typedef struct {
    uint8_t magic[4];     // 'U', 'B', 'L', 'E'
    uint16_t version;     
    uint16_t program_len; 
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

/* ==== 4. STRINGS STANDARDS COMPATIBILITY UTILITIES (Moved up to fix declaration error) ==== */
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
int strncmp(const char* a, const char* b, int n) {
    while (n > 0 && *a && (*a == *b)) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}
int strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}
char* strncpy(char* dst, const char* src, int n) {
    int i;
    for (i = 0; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}
char* strchr(const char* s, int c) {
    while (*s) { if (*s == (char)c) return (char*)s; s++; }
    if (c == 0) return (char*)s;
    return 0;
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

/* ==== 6. VIRTUAL VM INSTRUCTION CONSTANTS ==== */
#define OP_HALT         0x00
#define OP_PRINT_CHAR   0x01
#define OP_PRINT_NL     0x02
#define OP_PRINT_STR    0x03  
#define OP_LOAD_REG     0x04  
#define OP_ADD          0x05  
#define OP_SUB          0x06  
#define OP_PRINT_REG    0x07  
#define OP_JMP          0x08  
#define OP_JZ           0x09  
#define OP_JNZ          0x0A  
#define OP_CMP          0x0B  
#define OP_INC          0x0C  
#define OP_DEC          0x0D  
#define OP_SET_COLOR    0x0E  
#define OP_READ_REG     0x0F  
#define OP_MUL          0x10  
#define OP_DIV          0x11  
#define OP_MOD          0x12  

/* ==== 7. EGA/VGA COMPATIBLE 16-COLOR PALETTE DEFINITIONS ==== */
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
#define PAL_LIGHT_GREEN   0x00FF5555 
#define PAL_LIGHT_CYAN    0x0055FFFF
#define PAL_LIGHT_RED     0x00FF5555
#define PAL_LIGHT_MAGENTA 0x00FF55FF
#define PAL_YELLOW        0x00FFFF55
#define PAL_WHITE         0x00FFFFFF

/* Windows 1.0 Interface Accent Specific Colors Mapping */
#define WIN1_BACKGROUND   0x00008080 // Solid Teal Desktop Color
#define WIN1_HEADER       0x000000AA // Dark Royal Blue Title bars
#define WIN1_BORDER       0x00FFFFFF // Bright crisp frame borders

/* ==== 8. GLOBAL PERSISTENT INSTANCE MEMORY VOLUMES ==== */
static uint32_t* fb_mem    = 0;
static uint32_t  fb_width  = 0;
static uint32_t  fb_height = 0;
static uint32_t  fb_pitch  = 0;

uint32_t users_magic       = USERS_MAGIC;
User users[MAX_USERS];
File files[MAX_FILES];
int  num_users             = 0;
int  current_user_index    = -1;

/* Hardware Cursor Coordinates */
static int32_t mouse_x     = 400;
static int32_t mouse_y     = 300;
static bool    mouse_click = false;
static bool    prev_click  = false;

/* Graphical Terminal App Window Bounds & Text Cursor Placement */
#define TERM_X          15
#define TERM_Y          350
#define TERM_WIDTH      770
#define TERM_HEIGHT     235

static int term_cursor_x   = TERM_X + 5;
static int term_cursor_y   = TERM_Y + 25;
static uint32_t draw_color = PAL_WHITE;

/* Dynamic File Manager State Window Boundaries Tracking */
typedef struct {
    int x, y, w, h;
    char label[32];
    uint32_t action_id; 
    bool is_icon;
} DesktopWidget;

static DesktopWidget widgets[32];
static int num_widgets = 0;

/* Internal forward declarations mappings */
void users_save_to_disk(void);
void users_load_from_disk(void);
bool fs_save_to_disk(void);
bool fs_load_from_disk(void);
void desktop_redraw_pipeline(void);

/* ==== 9. MINIMAL BITMAP FONT ENGINES (8x8 Standard Text Matrix Maps) ==== */
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

/* ==== 10. LOW-LEVEL PIXEL PRIMITIVES GRAPHICS ENGINES ==== */
void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height) return;
    fb_mem[y * (fb_pitch / 4) + x] = color;
}

void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t i = 0; i < h; i++) {
        for (uint32_t j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    uint8_t glyph = (uint8_t)c;
    if (glyph >= 128) return;
    for (int row = 0; row < 8; row++) {
        uint8_t bits = font_bitmap[glyph][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

void draw_string(const char* s, uint32_t x, uint32_t y, uint32_t color) {
    while (*s) {
        draw_char(*s, x, y, color);
        x += 8;
        s++;
    }
}

/* Clear screen using a solid background color variable color */
void clear_screen(void) {
    draw_rect(0, 0, fb_width, fb_height, WIN1_BACKGROUND);
}

/* Render an anti-aliased mathematical vector bubble emblem (Logo) */
void draw_bubble_logo(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    for (int y = -r; y <= (int)r; y++) {
        for (int x = -r; x <= (int)r; x++) {
            if (x*x + y*y <= (int)(r*r)) {
                if (x*x + y*y >= (int)((r-2)*(r-2))) {
                    put_pixel(cx + x, cy + y, PAL_WHITE);
                } else if (x == -(int)(r/3) && y == -(int)(r/3)) {
                    put_pixel(cx + x, cy + y, PAL_WHITE);
                    put_pixel(cx + x + 1, cy + y, PAL_WHITE);
                    put_pixel(cx + x, cy + y + 1, PAL_WHITE);
                } else {
                    put_pixel(cx + x, cy + y, color);
                }
            }
        }
    }
}

/* Renders a functional Windows 1.0 style frame panel canvas window wrapper */
void draw_win1_window(int x, int y, int w, int h, const char* title) {
    draw_rect(x, y, w, h, PAL_LIGHT_GRAY); 
    draw_rect(x, y, w, 22, WIN1_HEADER);   
    draw_rect(x, y, w, 2, WIN1_BORDER);
    draw_rect(x, y + 20, w, 2, WIN1_BORDER);
    draw_rect(x, y, 2, h, WIN1_BORDER);
    draw_rect(x + w - 2, y, 2, h, WIN1_BORDER);
    draw_rect(x, y + h - 2, w, 2, WIN1_BORDER);
    draw_string(title, x + 6, y + 6, PAL_WHITE);
}

/* Renders dynamic desktop items inside frame allocations maps */
void draw_desktop_icon(int x, int y, const char* title, uint32_t act_id) {
    draw_rect(x + 10, y, 20, 20, PAL_YELLOW);
    draw_rect(x + 12, y + 4, 16, 2, PAL_BLACK);
    draw_rect(x + 12, y + 10, 12, 2, PAL_BLACK);
    draw_string(title, x, y + 24, PAL_WHITE);

    if (num_widgets < 32) {
        widgets[num_widgets].x = x;
        widgets[num_widgets].y = y;
        widgets[num_widgets].w = 45;
        widgets[num_widgets].h = 40;
        strcpy(widgets[num_widgets].label, title); // Now safe from declaration compiler warning errors
        widgets[num_widgets].action_id = act_id;
        widgets[num_widgets].is_icon = true;
        num_widgets++;
    }
}

/* ==== 11. GRAPHICS ORIENTED EMULATED OUTPUT TERMINAL PIPELINE ==== */
void put_char(char c) {
    if (c == '\n') {
        term_cursor_x = TERM_X + 5;
        term_cursor_y += 12;
        if (term_cursor_y >= (TERM_Y + TERM_HEIGHT - 15)) {
            // Scroll / clear back to top of graphical window application body text zone
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
    draw_char(c, term_cursor_x, term_cursor_y, draw_color);
    term_cursor_x += 8;
    if (term_cursor_x >= (TERM_X + TERM_WIDTH - 10)) {
        term_cursor_x = TERM_X + 5;
        term_cursor_y += 12;
        if (term_cursor_y >= (TERM_Y + TERM_HEIGHT - 15)) {
            draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
            term_cursor_y = TERM_Y + 25;
        }
    }
}

void print_string(const char* s) {
    while (*s) { put_char(*s++); }
}

void print_int(int num) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (num == 0) { put_char('0'); return; }
    bool negative = false;
    if (num < 0) { negative = true; num = -num; }
    while (num > 0) { buf[--i] = (num % 10) + '0'; num /= 10; }
    if (negative) { put_char('-'); }
    print_string(&buf[i]);
}

/* ==== 12. PORT LEVEL I/O HARDWARE INTERFACE BUS ==== */
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

/* ==== 13. PS/2 MOUSE DRIVER INITIALIZATION AND STREAM PROCESSING ==== */
void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (type == 0) {
            if ((inb(0x64) & 1) == 1) return;
        } else {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init(void) {
    mouse_wait(1);
    outb(0x64, 0xA8); 
    mouse_wait(1);
    outb(0x64, 020);  
    mouse_wait(0);
    uint8_t status = (inb(0x60) | 2); 
    mouse_wait(1);
    outb(0x64, 0x60); 
    mouse_wait(1);
    outb(0x60, status); 
    mouse_write(0xF6); 
    mouse_read();     
    mouse_write(0xF4); 
    mouse_read();     
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

    mouse_x += rel_x;
    mouse_y -= rel_y; 

    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= (int32_t)fb_width)  mouse_x = fb_width - 1;
    if (mouse_y >= (int32_t)fb_height) mouse_y = fb_height - 1;

    prev_click = mouse_click;
    mouse_click = (status & 1) ? true : false;
}

void draw_mouse_cursor(int32_t mx, int32_t my) {
    uint32_t c = PAL_WHITE;
    uint32_t o = PAL_BLACK;
    for(int i=0; i<10; i++) put_pixel(mx+i, my, o);
    for(int i=0; i<10; i++) put_pixel(mx, my+i, o);
    for(int i=0; i<8; i++)  put_pixel(mx+i, my+i, c);
    put_pixel(mx+1, my+2, c); put_pixel(mx+2, my+3, c);
    put_pixel(mx+3, my+4, c); put_pixel(mx+4, my+5, c);
}

/* ==== 14. PS/2 HARDWARE KEYBOARD ROUTINES CALL ENGINE ==== */
static const char scancode_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
};

char get_key(void) {
    uint8_t status = inb(0x64);
    if (status & 1) {
        uint8_t sc = inb(0x60);
        if (sc & 0x80) return 0;
        if (sc == 0x1C) return '\n';
        if (sc == 0x0E) return '\b';
        if (sc < 128) return scancode_table[sc];
    }
    return 0;
}

/* ==== 15. SECTOR DRIVEN HIGH-SPEED STORAGE ENGINES ==== */
void fs_clear_all(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        for (uint32_t j = 0; j < sizeof(File); j++) ((uint8_t*)&files[i])[j] = 0;
    }
}
void fs_init(void) {
    if (!fs_load_from_disk()) { fs_clear_all(); fs_save_to_disk(); }
}
int fs_find(const char* name) {
    if (current_user_index == -1) return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0 && strcmp(files[i].owner, users[current_user_index].username) == 0) return i;
    }
    return -1;
}
int fs_create(const char* name) {
    if (current_user_index == -1) return -1;
    if (fs_find(name) != -1) return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            for (uint32_t j = 0; j < sizeof(File); j++) ((uint8_t*)&files[i])[j] = 0;
            files[i].used = 1;
            strncpy(files[i].owner, users[current_user_index].username, MAX_USERNAME);
            strncpy(files[i].name, name, MAX_FILENAME);
            return i;
        }
    }
    return -1;
}
bool fs_delete(const char* name) {
    if (current_user_index == -1) return false;
    int idx = fs_find(name);
    if (idx == -1) return false;
    for (uint32_t i = 0; i < sizeof(File); i++) ((uint8_t*)&files[idx])[i] = 0;
    return true;
}
void users_init(void) {
    users_load_from_disk();
    if (num_users <= 0 || num_users > MAX_USERS) {
        for (int i = 0; i < MAX_USERS; i++) {
            users[i].used = false; users[i].username[0] = '\0'; users[i].password[0] = '\0';
        }
        users[0].used = true;
        strcpy(users[0].username, "admin");
        strcpy(users[0].password, "admin");
        num_users = 1;
        users_save_to_disk();
    }
    current_user_index = 0; 
}
bool login(const char* username, const char* password) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].used && strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            current_user_index = i; return true;
        }
    }
    return false;
}
bool add_user(const char* username, const char* password) {
    if (num_users >= MAX_USERS) return false;
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].used && strcmp(users[i].username, username) == 0) return false;
    }
    for (int i = 0; i < MAX_USERS; i++) {
        if (!users[i].used) {
            users[i].used = true;
            strncpy(users[i].username, username, MAX_USERNAME);
            strncpy(users[i].password, password, MAX_PASSWORD);
            num_users++; users_save_to_disk(); return true;
        }
    }
    return false;
}
bool del_user(const char* username) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].used && strcmp(users[i].username, username) == 0) {
            users[i].used = false; num_users--;
            if (i == current_user_index) current_user_index = -1;
            for (int f = 0; f < MAX_FILES; f++) {
                if (files[f].used && strcmp(files[f].owner, username) == 0) {
                    for (uint32_t j = 0; j < sizeof(File); j++) ((uint8_t*)&files[f])[j] = 0;
                }
            }
            users_save_to_disk(); fs_save_to_disk(); return true;
        }
    }
    return false;
}

/* ==== 16. ATA PIO CONTEXT TRACK DIRECTIVES ==== */
#define ATA_TIMEOUT 100000
static bool ata_wait(uint8_t mask, uint8_t value) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        if ((inb(0x1F7) & mask) == value) return true;
    }
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
    if (sectors > FS_MAX_SECTORS) return false;
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
    if (header->magic != FS_MAGIC || header->version != 2 || header->sectors > FS_MAX_SECTORS) return false;
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
    if (sectors > USERS_MAX_SECTORS) return;
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
    if (sectors > USERS_MAX_SECTORS) { num_users = 0; return; }
    static uint16_t buffer[256 * USERS_MAX_SECTORS]; uint8_t* src = (uint8_t*)buffer;
    if (!ata_read_sectors(USERS_LBA_START, sectors, buffer)) { num_users = 0; return; }
    uint32_t off = 0; uint32_t magic;
    for (uint32_t i = 0; i < sizeof(magic); i++) ((uint8_t*)&magic)[i] = src[off++];
    if (magic != USERS_MAGIC) { num_users = 0; return; }
    for (uint32_t i = 0; i < sizeof(users); i++) ((uint8_t*)users)[i] = src[off++];
    for (uint32_t i = 0; i < sizeof(num_users); i++) ((uint8_t*)&num_users)[i] = src[off++];
}

/* ==== 17. SYSTEM EXECUTIVE DIAGNOSTIC SUITES SYSTEM CONTROL ==== */
void cmd_help(void) {
    print_string("Available Commands:\n help, clear, sysinfo, run\n");
}
void app_sysinfo(void) {
    print_string("Uble Graphical Engine OS. IA-32 Protected Mode Framebuffer Active.\n");
}

void cmd_run(const char* name) {
    int idx = fs_find(name);
    if (idx == -1) { print_string("Asset trace failure.\n"); return; }
    File* f = &files[idx];
    uint8_t* raw_bytes = (uint8_t*)f->data;

    if (raw_bytes[0] == 0xEB) { 
        UbleStructuredNativeHeader* native_hdr = (UbleStructuredNativeHeader*)f->data;
        if (native_hdr->magic[0] != 'U' || native_hdr->magic[1] != 'B' || native_hdr->magic[2] != 'L' || native_hdr->magic[3] != 'E') return;
        typedef void (*native_execution_block_t)(void);
        native_execution_block_t run_native_app = (native_execution_block_t)f->data;
        run_native_app();
        return;
    }
    if (raw_bytes[0] == 'U' && raw_bytes[1] == 'B' && raw_bytes[2] == 'L' && raw_bytes[3] == 'E') {
        UbleHeader* header = (UbleHeader*)f->data;
        uint8_t* bytecode = (uint8_t*)(f->data + sizeof(UbleHeader));
        uint16_t pc = 0; bool running = true;
        int32_t virtual_registers[8] = {0};

        while (running && pc < header->program_len) {
            uint8_t opcode = bytecode[pc++];
            switch (opcode) {
                case OP_HALT:       running = false; break;
                case OP_PRINT_CHAR: put_char((char)bytecode[pc++]); break;
                case OP_PRINT_NL:   put_char('\n'); break;
                case OP_LOAD_REG: { uint8_t r = bytecode[pc++]; virtual_registers[r] = (int32_t)bytecode[pc++]; break; }
                case OP_ADD:      { uint8_t a = bytecode[pc++]; uint8_t b = bytecode[pc++]; virtual_registers[a] += virtual_registers[b]; break; }
                case OP_SUB:      { uint8_t a = bytecode[pc++]; uint8_t b = bytecode[pc++]; virtual_registers[a] -= virtual_registers[b]; break; }
                case OP_PRINT_REG:{ uint8_t r = bytecode[pc++]; print_int(virtual_registers[r]); break; }
                default: running = false; break;
            }
        }
    }
}

void split_first(const char* cmd, char* first, int first_max, const char** rest_out) {
    int i = 0; while (*cmd == ' ') cmd++;  
    while (*cmd && *cmd != ' ') { if (i < first_max - 1) first[i++] = *cmd; cmd++; }
    first[i] = '\0'; while (*cmd == ' ') cmd++; *rest_out = cmd;
}

void process_command(const char* cmdline) {
    char cmd[32]; const char* rest;
    if (!cmdline || !*cmdline) return;
    split_first(cmdline, cmd, sizeof(cmd), &rest);
    if (strcmp(cmd, "help") == 0) cmd_help();
    else if (strcmp(cmd, "clear") == 0) { draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK); term_cursor_x = TERM_X + 5; term_cursor_y = TERM_Y + 25; }
    else if (strcmp(cmd, "sysinfo") == 0) app_sysinfo();
    else if (strcmp(cmd, "run") == 0) { char name[32]; const char* d; split_first(rest, name, sizeof(name), &d); cmd_run(name); }
    else { print_string("Unknown shell context instruction.\n"); }
}

/* ==== 18. INTERACTIVE COMPOSITION DESKTOP WINDOWS MANAGER PIPELINE ==== */
void desktop_redraw_pipeline(void) {
    num_widgets = 0;
    clear_screen();

    // Top Windows 1.0 Interface System Main Status Strip Accent bar
    draw_rect(0, 0, fb_width, 28, WIN1_HEADER);
    draw_rect(0, 26, fb_width, 2, WIN1_BORDER);
    draw_string("Uble Graphical Core Environment v1.0", 12, 8, PAL_WHITE);

    // Bubble corporate logotype top right
    draw_bubble_logo(760, 13, 10, PAL_LIGHT_CYAN);

    // Populate Desktop Icons
    draw_desktop_icon(30,  50, "Help.App",  1);
    draw_desktop_icon(100, 50, "SysInfo",   2);
    draw_desktop_icon(170, 50, "DiskWipe",  3);
    draw_desktop_icon(240, 50, "ExecVM",    4);

    // Completely Graphic Windows 1.0 Terminal App Window Structure
    draw_win1_window(TERM_X, TERM_Y, TERM_WIDTH, TERM_HEIGHT, "Terminal.Exe (Pixel-Perfect Graphic Output)");
    draw_rect(TERM_X + 4, TERM_Y + 22, TERM_WIDTH - 8, TERM_HEIGHT - 26, PAL_BLACK); 
}

/* Click Hit Coordinates Intercept Engine */
void desktop_evaluate_click(int x, int y) {
    for (int i = 0; i < num_widgets; i++) {
        if (x >= widgets[i].x && x <= (widgets[i].x + widgets[i].w) &&
            y >= widgets[i].y && y <= (widgets[i].y + widgets[i].h)) {
            
            // Re-point and draw command pipeline logs purely inside the window coordinates space
            term_cursor_x = TERM_X + 5; 
            draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK); 
            
            if (widgets[i].action_id == 1) cmd_help();
            else if (widgets[i].action_id == 2) app_sysinfo();
            else if (widgets[i].action_id == 3) { fs_clear_all(); fs_save_to_disk(); print_string("Storage allocation indexes zeroed.\n"); }
            else if (widgets[i].action_id == 4) { print_string("Launching active runtime systems definitions...\n"); }
            return;
        }
    }
}

/* ==== 19. REFACTORIZED PRIMARY OS INTERACTION LOOP ==== */
void shell_loop(void) {
    // Left empty/deprecated to merge direct multitasking pipeline into kernel_main execution thread below
}

/* ==== 20. ABSOLUTE KERNEL RUNTIME ENTRY POINT ==== */
void kernel_main(MultibootInfo* mbi) {
    if (!(mbi->flags & (1 << 11))) {
        while(1) { __asm__ volatile("cli; hlt"); }
    }

    fb_mem    = (uint32_t*)((uint32_t)mbi->framebuffer_addr_lower);
    fb_width  = mbi->framebuffer_width;
    fb_height = mbi->framebuffer_height;
    fb_pitch  = mbi->framebuffer_pitch;

    mouse_init();
    users_init();
    fs_init();

    desktop_redraw_pipeline();

    static char main_cmd_buf[128];
    int cmd_idx = 0;
    
    main_cmd_buf[0] = '\0';

    while (1) {
        // Redraw desktop components layout completely via framebuffer writes
        desktop_redraw_pipeline();
        
        // Print interactive active prompt symbols bound entirely inside Terminal.Exe bounds
        draw_string("Uble:/admin$ ", TERM_X + 5, term_cursor_y, PAL_LIGHT_GREEN);
        draw_string(main_cmd_buf, TERM_X + 5 + (13 * 8), term_cursor_y, PAL_WHITE);
        
        mouse_poll_cycle();

        if (mouse_click && !prev_click) {
            desktop_evaluate_click(mouse_x, mouse_y);
        }

        char key = get_key();
        if (key == '\n') {
            main_cmd_buf[cmd_idx] = '\0';
            term_cursor_y += 12;
            
            // Redirect text writing execution results down to your custom graphics font layer
            term_cursor_x = TERM_X + 5;
            process_command(main_cmd_buf);
            
            cmd_idx = 0;
            main_cmd_buf[0] = '\0';
            term_cursor_y += 12;
            
            if (term_cursor_y >= (TERM_Y + TERM_HEIGHT - 15)) {
                draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
                term_cursor_y = TERM_Y + 25;
            }
        } else if (key == '\b') {
            if (cmd_idx > 0) {
                cmd_idx--;
                main_cmd_buf[cmd_idx] = '\0';
            }
        } else if (key != 0 && cmd_idx < 60) {
            main_cmd_buf[cmd_idx++] = key;
            main_cmd_buf[cmd_idx] = '\0';
        }

        // Draw cursor sprite layer above the interface background
        draw_mouse_cursor(mouse_x, mouse_y);

        for (volatile int delay = 0; delay < 40000; delay++);
    }
}

void poweroff(void) {
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);
    __asm__ volatile ("cli; hlt");
}