
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
char* strstrncpy(char* dst, const char* src, int n) {
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

char* strcat(char* dst, const char* src) {
    char* out = dst;
    while (*out) out++;
    while (*src) { *out++ = *src++; }
    *out = '\0';
    return dst;
}

/* ==== 5. STORAGE STRUCTURE CONFIGURATIONS ==== */
#define MAX_FILES        512
#define MAX_FILENAME     32
#define MAX_PATH         64
#define MAX_FILE_DATA    16384  

typedef struct {
    char owner[MAX_USERNAME];
    char name[MAX_FILENAME];
    char parent[MAX_PATH];
    char data[MAX_FILE_DATA];
    uint8_t used;
    uint8_t is_folder;
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

#define FS_MAX_SECTORS     16515  // changed to fit 512 files with 16KB data each
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
#define PAL_LIGHT_GREEN   0x0055FF55 
#define PAL_LIGHT_CYAN    0x0055FFFF
#define PAL_LIGHT_RED     0x00FF5555
#define PAL_LIGHT_MAGENTA 0x00FF55FF
#define PAL_YELLOW        0x00FFFF55
#define PAL_WHITE         0x00FFFFFF

#define WIN1_BACKGROUND   0x00008080 
#define WIN1_HEADER       0x000000AA 
#define WIN1_BORDER       0x00FFFFFF 

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

/* Hardware Cursor Coordinates and Trail Tracking buffers */
static int32_t mouse_x         = 400;
static int32_t mouse_y         = 300;
static int32_t last_mouse_x    = 400;
static int32_t last_mouse_y    = 300;
static bool    mouse_click     = false;
static bool    prev_click      = false;
static uint32_t mouse_back_buf[16 * 16]; // Backbuffer to save pixel data under mouse cursor

/* Graphical Terminal App Window Bounds & Text Cursor Placement */
#define TERM_X          15
#define TERM_Y          330
#define TERM_WIDTH      770
#define TERM_HEIGHT     255

static int term_cursor_x   = TERM_X + 5;
static int term_cursor_y   = TERM_Y + 25;
static uint32_t draw_color = PAL_WHITE;
static char system_status[64] = "FS status: init...";

void set_system_status(const char* msg) {
    int i = 0;
    while (*msg && i < (int)(sizeof(system_status) - 1)) {
        system_status[i++] = *msg++;
    }
    system_status[i] = '\0';
}

/* File Explorer Window Component Parameters */
#define EXPLORER_X      350
#define EXPLORER_Y      45
#define EXPLORER_WIDTH  435
#define EXPLORER_HEIGHT 270
static bool start_menu_open = false;
static bool terminal_open = true;
static bool notepad_open = false;
static bool explorer_open = false;
static int notepad_file_index = -1;
static char notepad_open_name[MAX_FILENAME] = "Untitled.txt";
static char explorer_path[MAX_PATH] = "/";
static int explorer_item_indices[32];
static int explorer_item_count = 0;

/* Dynamic Widget Array Interface Tracking */
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
void draw_file_explorer_contents(void);
int fs_find_in_dir(const char* parent, const char* name);
int fs_create_file(const char* name, const char* parent);
int fs_create_folder(const char* name, const char* parent);

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

void clear_screen(void) {
    draw_rect(0, 0, fb_width, fb_height, WIN1_BACKGROUND);
}

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
        strcpy(widgets[num_widgets].label, title);
        widgets[num_widgets].action_id = act_id;
        widgets[num_widgets].is_icon = true;
        num_widgets++;
    }
}

void draw_window_frame(int x, int y, int w, int h, const char* title) {
    draw_rect(x, y, w, h, PAL_LIGHT_GRAY);
    draw_rect(x, y, w, 22, WIN1_HEADER);
    draw_rect(x, y, w, 2, WIN1_BORDER);
    draw_rect(x, y + 20, w, 2, WIN1_BORDER);
    draw_rect(x, y, 2, h, WIN1_BORDER);
    draw_rect(x + w - 2, y, 2, h, WIN1_BORDER);
    draw_rect(x, y + h - 2, w, 2, WIN1_BORDER);
    draw_string(title, x + 6, y + 6, PAL_WHITE);
    draw_rect(x + w - 20, y + 4, 16, 16, PAL_RED);
    draw_string("X", x + w - 16, y + 6, PAL_WHITE);
}

void draw_button(int x, int y, int w, int h, const char* label) {
    draw_rect(x, y, w, h, PAL_LIGHT_BLUE);
    draw_rect(x, y, w, 1, PAL_BLACK);
    draw_rect(x, y + h - 1, w, 1, PAL_BLACK);
    draw_rect(x, y, 1, h, PAL_BLACK);
    draw_rect(x + w - 1, y, 1, h, PAL_BLACK);
    draw_string(label, x + 4, y + 4, PAL_BLACK);
}

void format_number(int value, char* out, int max) {
    int i = max - 1;
    out[i] = '\0';
    if (value == 0) {
        if (max > 1) out[--i] = '0';
    } else {
        bool neg = false;
        if (value < 0) { neg = true; value = -value; }
        while (value > 0 && i > 0) {
            out[--i] = (value % 10) + '0';
            value /= 10;
        }
        if (neg && i > 0) out[--i] = '-';
    }
    int start = i;
    int dst = 0;
    while (out[start]) { out[dst++] = out[start++]; }
    out[dst] = '\0';
}

int fs_find_path(const char* name, const char* parent) {
    return fs_find_in_dir(parent, name);
}

int fs_create_unique_name(const char* base, const char* ext, const char* parent, bool is_folder) {
    char candidate[MAX_FILENAME];
    int index = 0;
    while (index < 1000) {
        int len = 0;
        const char* p = base;
        while (*p && len < MAX_FILENAME - 1) {
            candidate[len++] = *p++;
        }
        if (index > 0) {
            char numbuf[16];
            format_number(index, numbuf, sizeof(numbuf));
            char* q = numbuf;
            while (*q && len < MAX_FILENAME - 1) {
                candidate[len++] = *q++;
            }
        }
        const char* e = ext;
        while (*e && len < MAX_FILENAME - 1) {
            candidate[len++] = *e++;
        }
        candidate[len] = '\0';
        if (fs_find_in_dir(parent, candidate) == -1) {
            if (is_folder) return fs_create_folder(candidate, parent);
            return fs_create_file(candidate, parent);
        }
        index++;
    }
    return -1;
}

/* ==== 11. GRAPHICS ORIENTED EMULATED OUTPUT TERMINAL PIPELINE ==== */
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

/* ==== 13. PS/2 MOUSE DRIVER INITIALIZATION AND SAVED BACKBUFFER SELECTION ==== */
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
    outb(0x64, 0x20);  
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
    // Only query if the data out port signals active mouse mouse packets
    if ((inb(0x64) & 1) == 0) return; 
    if (!(inb(0x64) & 0x20))  return; 

    uint8_t status = inb(0x60);
    int32_t rel_x  = mouse_read();
    int32_t rel_y  = mouse_read();

    if (status & 0x40 || status & 0x80) return; 

    if (status & 0x10) rel_x |= 0xFFFFFF00; 
    if (status & 0x20) rel_y |= 0xFFFFFF00;

    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    mouse_x += rel_x;
    mouse_y -= rel_y; 

    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= (int32_t)fb_width - 12)  mouse_x = fb_width - 13;
    if (mouse_y >= (int32_t)fb_height - 16) mouse_y = fb_height - 17;

    prev_click = mouse_click;
    mouse_click = (status & 1) ? true : false;
}

/* Restores data under old mouse path cleanly, bypassing full redraw flashing */
void clear_old_mouse(void) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 12; x++) {
            put_pixel(last_mouse_x + x, last_mouse_y + y, mouse_back_buf[y * 12 + x]);
        }
    }
}

/* Captures background state before painting the cursor arrow shape */
void save_mouse_backbuffer(void) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 12; x++) {
            mouse_back_buf[y * 12 + x] = get_pixel(mouse_x + x, mouse_y + y);
        }
    }
}

/* Highly Defined Windows Classic High-Contrast Arrow Cursor Layout */
void draw_mouse_cursor(int32_t mx, int32_t my) {
    // 12x16 High Contrast Classic Black and White Sharp Pointer Map
    static const char cursor_matrix[16][12] = {
        "W...........",
        "WW..........",
        "WBW.........",
        "WBBW........",
        "WBBBW.......",
        "WBBBBW......",
        "WBBBBBW.....",
        "WBBBBBBW....",
        "WBBBBBBBW...",
        "WBBBBBBBBW..",
        "WBBBBWWWWWW.",
        "WBBWWB......",
        "WBW..WB.....",
        "WW....WB....",
        "......WB....",
        ".......W...."
    };

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 12; x++) {
            char p = cursor_matrix[y][x];
            if (p == 'W')      put_pixel(mx + x, my + y, PAL_WHITE);
            else if (p == 'B') put_pixel(mx + x, my + y, PAL_BLACK);
        }
    }
}

/* ==== 14. PS/2 HARDWARE KEYBOARD ROUTINES CALL ENGINE ==== */
static const char scancode_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
};

char get_key(void) {
    // Intercept checking bits so mouse streams don't bleed into typing string data
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

/* ==== 15. SECTOR DRIVEN HIGH-SPEED STORAGE ENGINES ==== */
void fs_clear_all(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        for (uint32_t j = 0; j < sizeof(File); j++) ((uint8_t*)&files[i])[j] = 0;
    }
}
void fs_init(void) {
    if (!fs_load_from_disk()) {
        fs_clear_all();
        if (fs_save_to_disk()) {
            set_system_status("FS created and saved to disk.");
        } else {
            set_system_status("FS created but save failed.");
        }
    } else {
        set_system_status("FS loaded from disk.");
    }
}
int fs_find_in_dir(const char* parent, const char* name) {
    if (current_user_index == -1) return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0 && strcmp(files[i].owner, users[current_user_index].username) == 0 && strcmp(files[i].parent, parent) == 0) return i;
    }
    return -1;
}
int fs_find_folder(const char* parent, const char* name) {
    int idx = fs_find_in_dir(parent, name);
    if (idx == -1) return -1;
    return files[idx].is_folder ? idx : -1;
}
int fs_create_item(const char* name, const char* parent, bool is_folder) {
    if (current_user_index == -1) return -1;
    if (fs_find_in_dir(parent, name) != -1) return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            for (uint32_t j = 0; j < sizeof(File); j++) ((uint8_t*)&files[i])[j] = 0;
            files[i].used = 1;
            files[i].is_folder = is_folder ? 1 : 0;
            strcpy(files[i].owner, users[current_user_index].username);
            strcpy(files[i].name, name);
            strcpy(files[i].parent, parent);
            return i;
        }
    }
    return -1;
}
int fs_create_file(const char* name, const char* parent) {
    return fs_create_item(name, parent, false);
}
int fs_create_folder(const char* name, const char* parent) {
    return fs_create_item(name, parent, true);
}
bool fs_delete(const char* name, const char* parent) {
    if (current_user_index == -1) return false;
    int idx = fs_find_in_dir(parent, name);
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

bool ata_read_sectors_lba(uint32_t lba, uint32_t count, uint16_t* buf) {
    while (count > 0) {
        uint8_t chunk = (count > 255) ? 255 : (uint8_t)count;
        if (!ata_read_sectors(lba, chunk, buf)) return false;
        lba += chunk;
        buf += chunk * 256;
        count -= chunk;
    }
    return true;
}

bool ata_write_sectors_lba(uint32_t lba, uint32_t count, uint16_t* buf) {
    while (count > 0) {
        uint8_t chunk = (count > 255) ? 255 : (uint8_t)count;
        if (!ata_write_sectors(lba, chunk, buf)) return false;
        lba += chunk;
        buf += chunk * 256;
        count -= chunk;
    }
    return true;
}

bool fs_save_to_disk(void) {
    uint32_t total_bytes = sizeof(FSHeader) + sizeof(files);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors > FS_MAX_SECTORS) {
        set_system_status("FS save failed: too many sectors.");
        return false;
    }
    FSHeader header = { FS_MAGIC, 2, sectors };

    static uint16_t buffer[256 * FS_MAX_SECTORS];
    uint8_t* dst = (uint8_t*)buffer;

    for (uint32_t i = 0; i < sectors * FS_SECTOR_SIZE; i++)
        dst[i] = 0;

    for (uint32_t i = 0; i < sizeof(FSHeader); i++)
        dst[i] = ((uint8_t*)&header)[i];

    for (uint32_t i = 0; i < sizeof(files); i++)
        dst[sizeof(FSHeader) + i] = ((uint8_t*)files)[i];

    if (!ata_write_sectors_lba(FS_LBA_START, sectors, buffer)) {
        set_system_status("FS save failed: disk write error.");
        return false;
    }
    set_system_status("FS saved to disk.");
    return true;
}

bool fs_load_from_disk(void) {
    static uint16_t buffer[256 * FS_MAX_SECTORS];
    uint8_t* src = (uint8_t*)buffer;

    if (!ata_read_sectors_lba(FS_LBA_START, 1, buffer)) {
        set_system_status("FS load failed: no disk data.");
        return false;
    }

    FSHeader* header = (FSHeader*)src;
    if (header->magic != FS_MAGIC || header->version != 2 || header->sectors == 0 || header->sectors > FS_MAX_SECTORS) return false;

    if (!ata_read_sectors_lba(FS_LBA_START, header->sectors, buffer)) return false;

    uint8_t* f = src + sizeof(FSHeader);
    for (uint32_t i = 0; i < sizeof(files); i++)
        ((uint8_t*)files)[i] = f[i];

    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            files[i].owner[MAX_USERNAME - 1] = '\0';
            files[i].name[MAX_FILENAME - 1] = '\0';
            files[i].parent[MAX_PATH - 1] = '\0';
            files[i].data[MAX_FILE_DATA - 1] = '\0';
        }
    }
    return true;
}

void users_save_to_disk(void) {
    uint32_t total_bytes = sizeof(users_magic) + sizeof(users) + sizeof(num_users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors > USERS_MAX_SECTORS) return;

    static uint16_t buffer[256 * USERS_MAX_SECTORS];
    uint8_t* dst = (uint8_t*)buffer;

    for (uint32_t i = 0; i < sectors * FS_SECTOR_SIZE; i++)
        dst[i] = 0;

    uint32_t off = 0;
    for (uint32_t i = 0; i < sizeof(users_magic); i++)
        dst[off++] = ((uint8_t*)&users_magic)[i];
    for (uint32_t i = 0; i < sizeof(users); i++)
        dst[off++] = ((uint8_t*)users)[i];
    for (uint32_t i = 0; i < sizeof(num_users); i++)
        dst[off++] = ((uint8_t*)&num_users)[i];

    ata_write_sectors(USERS_LBA_START, (uint8_t)sectors, buffer);
}

void users_load_from_disk(void) {
    uint32_t total_bytes = sizeof(users_magic) + sizeof(users) + sizeof(num_users);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors > USERS_MAX_SECTORS) { num_users = 0; return; }

    static uint16_t buffer[256 * USERS_MAX_SECTORS];
    uint8_t* src = (uint8_t*)buffer;

    if (!ata_read_sectors(USERS_LBA_START, sectors, buffer)) { num_users = 0; return; }

    uint32_t off = 0;
    uint32_t magic;
    for (uint32_t i = 0; i < sizeof(magic); i++)
        ((uint8_t*)&magic)[i] = src[off++];

    if (magic != USERS_MAGIC) { num_users = 0; return; }

    for (uint32_t i = 0; i < sizeof(users); i++)
        ((uint8_t*)users)[i] = src[off++];
    for (uint32_t i = 0; i < sizeof(num_users); i++)
        ((uint8_t*)&num_users)[i] = src[off++];

    if (users_magic != USERS_MAGIC || num_users < 0 || num_users > MAX_USERS) {
        num_users = 0;
    }
}

/* ==== 17. SYSTEM EXECUTIVE DIAGNOSTIC SUITES SYSTEM CONTROL ==== */
void cmd_help(void) {
    print_string("Available Commands:\n help, clear, sysinfo, explorer\n");
}
void app_sysinfo(void) {
    print_string("Uble Graphical Engine OS. IA-32 Protected Mode Framebuffer Active.\n");
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
    else if (strcmp(cmd, "explorer") == 0) { explorer_open = true; desktop_redraw_pipeline(); }
    else { print_string("Unknown shell context instruction.\n"); }
}

/* ==== 18. INTERACTIVE COMPOSITION DESKTOP WINDOWS MANAGER PIPELINE ==== */
void build_explorer_listing(void) {
    explorer_item_count = 0;
    for (int i = 0; i < MAX_FILES && explorer_item_count < 32; i++) {
        if (files[i].used && strcmp(files[i].owner, users[current_user_index].username) == 0 && strcmp(files[i].parent, explorer_path) == 0) {
            explorer_item_indices[explorer_item_count++] = i;
        }
    }
}

void open_file_in_notepad(int idx) {
    if (idx < 0 || idx >= MAX_FILES) return;
    notepad_open = true;
    notepad_file_index = idx;
    strcpy(notepad_open_name, files[idx].name);
    terminal_open = false;
}

void draw_start_menu(void) {
    int sx = 5;
    int sy = 28;
    int sw = 160;
    int sh = 110;
    draw_rect(sx, sy, sw, sh, PAL_LIGHT_GRAY);
    draw_rect(sx, sy, sw, 1, PAL_BLACK);
    draw_string("Start Menu", sx + 8, sy + 8, PAL_WHITE);
    draw_button(sx + 8, sy + 30, 140, 20, "Terminal");
    if (num_widgets < 32) { widgets[num_widgets].x = sx + 8; widgets[num_widgets].y = sy + 30; widgets[num_widgets].w = 140; widgets[num_widgets].h = 20; strcpy(widgets[num_widgets].label, "StartTerminal"); widgets[num_widgets].action_id = 101; widgets[num_widgets].is_icon = false; num_widgets++; }
    draw_button(sx + 8, sy + 56, 140, 20, "Notepad");
    if (num_widgets < 32) { widgets[num_widgets].x = sx + 8; widgets[num_widgets].y = sy + 56; widgets[num_widgets].w = 140; widgets[num_widgets].h = 20; strcpy(widgets[num_widgets].label, "StartNotepad"); widgets[num_widgets].action_id = 102; widgets[num_widgets].is_icon = false; num_widgets++; }
    draw_button(sx + 8, sy + 82, 140, 20, "File Explorer");
    if (num_widgets < 32) { widgets[num_widgets].x = sx + 8; widgets[num_widgets].y = sy + 82; widgets[num_widgets].w = 140; widgets[num_widgets].h = 20; strcpy(widgets[num_widgets].label, "StartExplorer"); widgets[num_widgets].action_id = 103; widgets[num_widgets].is_icon = false; num_widgets++; }
}

void draw_notepad_window(void) {
    int x = 100;
    int y = 90;
    int w = 600;
    int h = 380;
    draw_window_frame(x, y, w, h, "Notepad.Exe");
    draw_rect(x + 4, y + 22, w - 8, h - 26, PAL_WHITE);
    draw_string("File:", x + 10, y + 30, PAL_BLACK);
    draw_string(notepad_open_name, x + 50, y + 30, PAL_BLUE);
    draw_button(x + 460, y + 30, 100, 18, "Save File");
    if (num_widgets < 32) { widgets[num_widgets].x = x + 460; widgets[num_widgets].y = y + 30; widgets[num_widgets].w = 100; widgets[num_widgets].h = 18; strcpy(widgets[num_widgets].label, "SaveNote"); widgets[num_widgets].action_id = 70; widgets[num_widgets].is_icon = false; num_widgets++; }
    if (num_widgets < 32) { widgets[num_widgets].x = x + w - 20; widgets[num_widgets].y = y + 4; widgets[num_widgets].w = 16; widgets[num_widgets].h = 16; strcpy(widgets[num_widgets].label, "CloseNotepad"); widgets[num_widgets].action_id = 57; widgets[num_widgets].is_icon = false; num_widgets++; }

    int print_y = y + 54;
    int data_offset = 0;
    if (notepad_file_index >= 0) {
        const char* src = files[notepad_file_index].data;
        while (src[data_offset] != '\0' && print_y < y + h - 10) {
            char linebuf[64];
            int li = 0;
            while (li < 60 && src[data_offset] != '\0' && src[data_offset] != '\n') {
                linebuf[li++] = src[data_offset++];
            }
            linebuf[li] = '\0';
            draw_string(linebuf, x + 10, print_y, PAL_BLACK);
            print_y += 12;
            if (src[data_offset] == '\n') data_offset++;
        }
    }
}

void draw_file_explorer_contents(void) {
    draw_window_frame(EXPLORER_X, EXPLORER_Y, EXPLORER_WIDTH, EXPLORER_HEIGHT, "Explorer.Exe - File Manager");
    draw_rect(EXPLORER_X + 4, EXPLORER_Y + 22, EXPLORER_WIDTH - 8, EXPLORER_HEIGHT - 26, PAL_WHITE);
    draw_string("Path:", EXPLORER_X + 12, EXPLORER_Y + 30, PAL_BLACK);
    draw_string(explorer_path, EXPLORER_X + 60, EXPLORER_Y + 30, PAL_BLUE);
    draw_button(EXPLORER_X + 250, EXPLORER_Y + 26, 90, 18, "New File");
    if (num_widgets < 32) { widgets[num_widgets].x = EXPLORER_X + 250; widgets[num_widgets].y = EXPLORER_Y + 26; widgets[num_widgets].w = 90; widgets[num_widgets].h = 18; strcpy(widgets[num_widgets].label, "NewFile"); widgets[num_widgets].action_id = 60; widgets[num_widgets].is_icon = false; num_widgets++; }
    draw_button(EXPLORER_X + 350, EXPLORER_Y + 26, 90, 18, "New Folder");
    if (num_widgets < 32) { widgets[num_widgets].x = EXPLORER_X + 350; widgets[num_widgets].y = EXPLORER_Y + 26; widgets[num_widgets].w = 90; widgets[num_widgets].h = 18; strcpy(widgets[num_widgets].label, "NewFolder"); widgets[num_widgets].action_id = 61; widgets[num_widgets].is_icon = false; num_widgets++; }
    if (strcmp(explorer_path, "/") != 0) {
        draw_button(EXPLORER_X + 450, EXPLORER_Y + 26, 80, 18, "Up Folder");
        if (num_widgets < 32) { widgets[num_widgets].x = EXPLORER_X + 450; widgets[num_widgets].y = EXPLORER_Y + 26; widgets[num_widgets].w = 80; widgets[num_widgets].h = 18; strcpy(widgets[num_widgets].label, "UpFolder"); widgets[num_widgets].action_id = 62; widgets[num_widgets].is_icon = false; num_widgets++; }
    }

    draw_string("Name", EXPLORER_X + 15, EXPLORER_Y + 50, PAL_BLACK);
    draw_string("Type", EXPLORER_X + 220, EXPLORER_Y + 50, PAL_BLACK);
    draw_string("Size", EXPLORER_X + 330, EXPLORER_Y + 50, PAL_BLACK);
    draw_string("Owner", EXPLORER_X + 430, EXPLORER_Y + 50, PAL_BLACK);
    draw_rect(EXPLORER_X + 10, EXPLORER_Y + 64, EXPLORER_WIDTH - 20, 1, PAL_DARK_GRAY);
    build_explorer_listing();
    int print_y = EXPLORER_Y + 74;
    if (explorer_item_count == 0) {
        draw_string("[Empty folder. Use New File or New Folder to populate.]", EXPLORER_X + 15, print_y, PAL_DARK_GRAY);
        return;
    }
    for (int i = 0; i < explorer_item_count; i++) {
        int idx = explorer_item_indices[i];
        if (!files[idx].used) continue;
        char type_label[16];
        if (files[idx].is_folder) strcpy(type_label, "<DIR>"); else strcpy(type_label, "FILE");
        draw_string(files[idx].name, EXPLORER_X + 15, print_y, files[idx].is_folder ? PAL_LIGHT_BLUE : PAL_BLACK);
        draw_string(type_label, EXPLORER_X + 220, print_y, PAL_BLACK);
        if (!files[idx].is_folder) {
            int file_size = strlen(files[idx].data);
            char sizebuf[12]; int pos = 10; sizebuf[11]='\0'; if(file_size==0) sizebuf[--pos]='0'; while(file_size>0 && pos>0) { sizebuf[--pos] = (file_size % 10) + '0'; file_size /= 10; }
            draw_string(&sizebuf[pos], EXPLORER_X + 330, print_y, PAL_DARK_GRAY);
        }
        draw_string(files[idx].owner, EXPLORER_X + 430, print_y, PAL_BLACK);
        if (num_widgets < 32) {
            widgets[num_widgets].x = EXPLORER_X + 10;
            widgets[num_widgets].y = print_y - 2;
            widgets[num_widgets].w = EXPLORER_WIDTH - 20;
            widgets[num_widgets].h = 14;
            strcpy(widgets[num_widgets].label, files[idx].name);
            widgets[num_widgets].action_id = 400 + i;
            widgets[num_widgets].is_icon = false;
            num_widgets++;
        }
        print_y += 14;
    }
}

void desktop_redraw_pipeline(void) {
    num_widgets = 0;
    clear_screen();

    // Top Windows 1.0 Interface System Main Status Strip Accent bar
    draw_rect(0, 0, fb_width, 28, WIN1_HEADER);
    draw_rect(0, 26, fb_width, 2, WIN1_BORDER);

    // Bubble corporate logotype top left as the designated Start Switch
    draw_bubble_logo(20, 13, 10, PAL_LIGHT_CYAN);
    draw_string("Start", 36, 8, PAL_WHITE);

    // Register Start button zone boundary parameters
    if (num_widgets < 32) {
        widgets[num_widgets].x = 5; widgets[num_widgets].y = 2;
        widgets[num_widgets].w = 80; widgets[num_widgets].h = 24;
        strcpy(widgets[num_widgets].label, "StartMenu");
        widgets[num_widgets].action_id = 99;
        widgets[num_widgets].is_icon = false;
        num_widgets++;
    }

    draw_string("Uble Graphical Core Environment v1.0", 160, 8, PAL_WHITE);
    draw_string(system_status, 160, 18, PAL_LIGHT_GRAY);
    draw_string("Welcome, admin", fb_width - 140, 8, PAL_WHITE);

    // Populate Desktop Icons
    draw_desktop_icon(30,  50, "Help.App",  1);
    draw_desktop_icon(100, 50, "SysInfo",   2);
    draw_desktop_icon(170, 50, "Explorer",  5);
    draw_desktop_icon(240, 50, "Notepad",   6);

    if (terminal_open) {
        draw_window_frame(TERM_X, TERM_Y, TERM_WIDTH, TERM_HEIGHT, "Terminal.Exe");
        draw_rect(TERM_X + 4, TERM_Y + 22, TERM_WIDTH - 8, TERM_HEIGHT - 26, PAL_BLACK);
        if (num_widgets < 32) {
            widgets[num_widgets].x = TERM_X + TERM_WIDTH - 20; widgets[num_widgets].y = TERM_Y + 4;
            widgets[num_widgets].w = 16; widgets[num_widgets].h = 16;
            strcpy(widgets[num_widgets].label, "CloseTerminal");
            widgets[num_widgets].action_id = 56;
            widgets[num_widgets].is_icon = false;
            num_widgets++;
        }
    }

    if (explorer_open) {
        draw_file_explorer_contents();
        if (num_widgets < 32) {
            widgets[num_widgets].x = EXPLORER_X + EXPLORER_WIDTH - 20; widgets[num_widgets].y = EXPLORER_Y + 4;
            widgets[num_widgets].w = 16; widgets[num_widgets].h = 16;
            strcpy(widgets[num_widgets].label, "CloseExplorer");
            widgets[num_widgets].action_id = 55;
            widgets[num_widgets].is_icon = false;
            num_widgets++;
        }
    }

    if (notepad_open) {
        draw_notepad_window();
    }

    if (start_menu_open) {
        draw_start_menu();
    }
}

/* Click Hit Coordinates Intercept Engine */
void desktop_evaluate_click(int x, int y) {
    for (int i = 0; i < num_widgets; i++) {
        if (x >= widgets[i].x && x <= (widgets[i].x + widgets[i].w) &&
            y >= widgets[i].y && y <= (widgets[i].y + widgets[i].h)) {

            if (widgets[i].action_id == 99) {
                start_menu_open = !start_menu_open;
                desktop_redraw_pipeline();
                return;
            }
            if (widgets[i].action_id == 55) {
                explorer_open = false;
                desktop_redraw_pipeline();
                return;
            }
            if (widgets[i].action_id == 56) {
                terminal_open = false;
                desktop_redraw_pipeline();
                return;
            }
            if (widgets[i].action_id == 57) {
                notepad_open = false;
                if (notepad_file_index >= 0) fs_save_to_disk();
                notepad_file_index = -1;
                desktop_redraw_pipeline();
                return;
            }
            if (widgets[i].action_id == 60) {
                if (fs_create_unique_name("NewFile", ".txt", explorer_path, false) != -1) {
                    fs_save_to_disk();
                    desktop_redraw_pipeline();
                }
                return;
            }
            if (widgets[i].action_id == 61) {
                if (fs_create_unique_name("NewFolder", "", explorer_path, true) != -1) {
                    fs_save_to_disk();
                    desktop_redraw_pipeline();
                }
                return;
            }
            if (widgets[i].action_id == 62) {
                if (strcmp(explorer_path, "/") != 0) {
                    int len = strlen(explorer_path);
                    while (len > 1 && explorer_path[len - 1] == '/') explorer_path[--len] = '\0';
                    while (len > 1 && explorer_path[len - 1] != '/') explorer_path[--len] = '\0';
                    if (len == 0) strcpy(explorer_path, "/");
                }
                desktop_redraw_pipeline();
                return;
            }
            if (widgets[i].action_id == 70) {
                if (notepad_file_index >= 0) {
                    fs_save_to_disk();
                    desktop_redraw_pipeline();
                }
                return;
            }
            if (widgets[i].action_id == 101) {
                terminal_open = true;
                start_menu_open = false;
                desktop_redraw_pipeline();
                return;
            }
            if (widgets[i].action_id == 102) {
                int idx = fs_create_unique_name("Untitled", ".txt", explorer_path, false);
                if (idx >= 0) {
                    strcpy(files[idx].data, "");
                    fs_save_to_disk();
                    open_file_in_notepad(idx);
                    start_menu_open = false;
                    desktop_redraw_pipeline();
                }
                return;
            }
            if (widgets[i].action_id == 103 || widgets[i].action_id == 104) {
                explorer_open = true;
                start_menu_open = false;
                desktop_redraw_pipeline();
                return;
            }
            if (widgets[i].action_id == 1) { cmd_help(); return; }
            if (widgets[i].action_id == 2) { app_sysinfo(); return; }
            if (widgets[i].action_id == 5) { explorer_open = true; desktop_redraw_pipeline(); print_string("Launched File Explorer interface matrix.\n"); return; }
            if (widgets[i].action_id == 6) { int idx = fs_create_unique_name("Untitled", ".txt", explorer_path, false); if (idx >= 0) { open_file_in_notepad(idx); fs_save_to_disk(); desktop_redraw_pipeline(); } return; }

            if (widgets[i].action_id >= 400 && widgets[i].action_id < 500) {
                int list_index = widgets[i].action_id - 400;
                if (list_index >= 0 && list_index < explorer_item_count) {
                    int idx = explorer_item_indices[list_index];
                    if (files[idx].is_folder) {
                        if (strcmp(explorer_path, "/") == 0) {
                            strcpy(explorer_path, "/");
                            int prefix_len = 1;
                            if (strlen(files[idx].name) < MAX_PATH - prefix_len) {
                                strcat(explorer_path, files[idx].name);
                            }
                        } else {
                            if (strlen(explorer_path) + strlen(files[idx].name) + 2 < MAX_PATH) {
                                if (explorer_path[strlen(explorer_path) - 1] != '/') strcat(explorer_path, "/");
                                strcat(explorer_path, files[idx].name);
                            }
                        }
                        desktop_redraw_pipeline();
                        return;
                    }
                    open_file_in_notepad(idx);
                    desktop_redraw_pipeline();
                    return;
                }
            }

            return;
        }
    }
}

/* ==== 19. ABSOLUTE KERNEL RUNTIME ENTRY POINT ==== */
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

    // Seed mock virtual directory files if storage sector maps are fresh
    if (fs_find_in_dir("/", "Welcome.txt") == -1) {
        int idx = fs_create_file("Welcome.txt", "/");
        if(idx != -1) strcpy(files[idx].data, "Welcome to the new Graphical desktop configuration build!");
    }
    if (fs_find_in_dir("/", "System.sys") == -1) {
        int idx = fs_create_file("System.sys", "/");
        if(idx != -1) strcpy(files[idx].data, "Kernel Executable Binaries Config Streams Active.");
    }

    // Baseline Full Render Redraw Pipeline
    desktop_redraw_pipeline();
    save_mouse_backbuffer();

    static char main_cmd_buf[128];
    int cmd_idx = 0;
    main_cmd_buf[0] = '\0';

    // Print static prompt starting text once outside the cycle to maintain stability
    draw_string("Uble:/admin$ ", TERM_X + 5, term_cursor_y, PAL_LIGHT_GREEN);

    while (1) {
        mouse_poll_cycle();

        // Selective Smart Pointer Rendering (Triggers only if the mouse coordinates shift)
        if (mouse_x != last_mouse_x || mouse_y != last_mouse_y) {
            clear_old_mouse();         // Wipe old mouse position using local saved background slice array
            save_mouse_backbuffer();   // Snip current background where mouse moves to next
            draw_mouse_cursor(mouse_x, mouse_y); // Render defined hi-contrast arrow sprite
            
            last_mouse_x = mouse_x;
            last_mouse_y = mouse_y;
        }

        if (mouse_click && !prev_click) {
            // Restore context background to evaluate clicks cleanly without trailing traces
            clear_old_mouse();
            desktop_evaluate_click(mouse_x, mouse_y);
            save_mouse_backbuffer();
            draw_mouse_cursor(mouse_x, mouse_y);
        }
        prev_click = mouse_click;

        char key = get_key();
        if (notepad_open && notepad_file_index >= 0) {
            if (key != 0) {
                if (key == '\b') {
                    int len = strlen(files[notepad_file_index].data);
                    if (len > 0) files[notepad_file_index].data[len - 1] = '\0';
                } else if (key == '\n') {
                    int len = strlen(files[notepad_file_index].data);
                    if (len < MAX_FILE_DATA - 1) {
                        files[notepad_file_index].data[len++] = '\n';
                        files[notepad_file_index].data[len] = '\0';
                    }
                } else {
                    int len = strlen(files[notepad_file_index].data);
                    if (len < MAX_FILE_DATA - 1) {
                        files[notepad_file_index].data[len++] = key;
                        files[notepad_file_index].data[len] = '\0';
                    }
                }
                fs_save_to_disk();
                desktop_redraw_pipeline();
            }
        } else {
            if (key == '\n') {
                main_cmd_buf[cmd_idx] = '\0';
                draw_rect(TERM_X + 5, term_cursor_y, TERM_WIDTH - 10, 12, PAL_BLACK);
                term_cursor_x = TERM_X + 5;
                process_command(main_cmd_buf);
                cmd_idx = 0;
                main_cmd_buf[0] = '\0';
                term_cursor_y += 12;
                if (term_cursor_y >= (TERM_Y + TERM_HEIGHT - 15)) {
                    draw_rect(TERM_X + 5, TERM_Y + 25, TERM_WIDTH - 10, TERM_HEIGHT - 30, PAL_BLACK);
                    term_cursor_y = TERM_Y + 25;
                }
                draw_string("Uble:/admin$ ", TERM_X + 5, term_cursor_y, PAL_LIGHT_GREEN);
            } else if (key == '\b') {
                if (cmd_idx > 0) {
                    cmd_idx--;
                    main_cmd_buf[cmd_idx] = '\0';
                    draw_rect(TERM_X + 5 + (13 * 8), term_cursor_y, TERM_WIDTH - 120, 12, PAL_BLACK);
                    draw_string(main_cmd_buf, TERM_X + 5 + (13 * 8), term_cursor_y, PAL_WHITE);
                }
            } else if (key != 0 && cmd_idx < 60) {
                main_cmd_buf[cmd_idx++] = key;
                main_cmd_buf[cmd_idx] = '\0';
                draw_string(main_cmd_buf, TERM_X + 5 + (13 * 8), term_cursor_y, PAL_WHITE);
            }
        }

        for (volatile int delay = 0; delay < 20000; delay++);
    }
}

void poweroff(void) {
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);
    __asm__ volatile ("cli; hlt");
}