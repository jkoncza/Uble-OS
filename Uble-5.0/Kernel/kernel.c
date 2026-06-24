__attribute__((section(".multiboot")))
const unsigned long multiboot_header[] = {
    0x1BADB002,
    0x00000003,
    -(0x1BADB002 + 0x00000003)
};

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef int            bool;

#define true  1
#define false 0

#define MAX_USERNAME 32
#define MAX_PASSWORD 32

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEM     ((uint16_t*)0xB8000)
#define COLOR       0x0F

#define MAX_CMD_LENGTH  80
#define MAX_USERS       10
#define MAX_FILES       32
#define MAX_FILENAME    16
#define MAX_FILE_DATA   256

typedef struct {
    char owner[MAX_USERNAME];
    char name[MAX_FILENAME];
    char data[MAX_FILE_DATA];
    uint8_t used;
} File;

#define FS_LBA_START    2048u
#define USERS_MAGIC 0x55534552  // "USER"
#define FS_SECTOR_SIZE  512u

#define FS_MAGIC 0x46534F31  // "FSO1"

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t sectors;
} FSHeader;

#define FS_MAX_SECTORS     1024   // 512 KB
#define USERS_MAX_SECTORS  64


#define USERS_LBA_START  (FS_LBA_START + FS_MAX_SECTORS)

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    uint8_t used;
} User;


void users_save_to_disk(void);
void users_load_from_disk(void);
bool fs_save_to_disk(void);
bool fs_load_from_disk(void);

/* Globals */
static int cursor_row = 0;
static int cursor_col = 0;

uint32_t users_magic = USERS_MAGIC;

User users[MAX_USERS];
File files[MAX_FILES];

int  num_users          = 0;
int  current_user_index = -1;

/* Forward declarations */
void kernel_main(void);

/* ==== Low-level port I/O ==== */

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

/* ==== VGA text output ==== */

static void scroll_if_needed(void) {
    int row, col;
    uint16_t *vga = VGA_MEM;

    if (cursor_row < VGA_HEIGHT)
        return;

    /* Scroll up one line */
    for (row = 1; row < VGA_HEIGHT; row++) {
        for (col = 0; col < VGA_WIDTH; col++) {
            vga[(row - 1) * VGA_WIDTH + col] =
                vga[row * VGA_WIDTH + col];
        }
    }

    /* Clear last line */
    for (col = 0; col < VGA_WIDTH; col++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
            (uint16_t)(' ' | (COLOR << 8));
    }

    cursor_row = VGA_HEIGHT - 1;
}

static void put_char_at(char c, int row, int col) {
    uint16_t *vga = VGA_MEM;
    vga[row * VGA_WIDTH + col] = (uint16_t)(c | (COLOR << 8));
}

static void put_char(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        scroll_if_needed();
        return;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            put_char_at(' ', cursor_row, cursor_col);
        }
        return;
    }

    put_char_at(c, cursor_row, cursor_col);
    cursor_col++;
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
        scroll_if_needed();
    }
}

void print_string(const char* s) {
    while (*s) {
        put_char(*s++);
    }
}

void clear_screen(void) {
    int row, col;
    for (row = 0; row < VGA_HEIGHT; row++) {
        for (col = 0; col < VGA_WIDTH; col++) {
            put_char_at(' ', row, col);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}

/* ==== Minimal string functions ==== */

int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, int n) {
    while (n > 0 && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

int strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

char* strcpy(char* dst, const char* src) {
    char* ret = dst;
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
    return ret;
}

char* strncpy(char* dst, const char* src, int n) {
    int i;
    for (i = 0; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return dst;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    if (c == 0) return (char*)s;
    return 0;
}

/* ==== Keyboard input (polling, scancode set 1) ==== */

static const char scancode_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,
    '\\','z','x','c','v','b','n','m',',','.','/',
    0,
    '*',
    0,
    ' ',
};

char get_key(void) {
    while (1) {
        uint8_t status = inb(0x64);
        if (status & 1) {
            uint8_t sc = inb(0x60);

            if (sc & 0x80) {
                continue;
            }

            if (sc == 0x1C) {
                return '\n';
            } else if (sc == 0x0E) {
                return '\b';
            }

            if (sc < 128) {
                char c = scancode_table[sc];
                if (c != 0) {
                    return c;
                }
            }
        }
    }
}

void read_line(char* buf, int max) {
    int idx = 0;
    while (1) {
        char c = get_key();

        if (c == '\n') {
            put_char('\n');
            buf[idx] = '\0';
            return;
        } else if (c == '\b') {
            if (idx > 0) {
                idx--;
                put_char('\b');
            }
        } else {
            if (idx < max - 1 && c >= 32 && c <= 126) {
                buf[idx++] = c;
                put_char(c);
            }
        }
    }
}

/* ==== Simple in-memory filesystem ==== */

void fs_clear_all(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        for (uint32_t j = 0; j < sizeof(File); j++)
            ((uint8_t*)&files[i])[j] = 0;
    }
}


void fs_init(void) {
    if (!fs_load_from_disk()) {
        fs_clear_all();
        fs_save_to_disk();
    }
}



int fs_find(const char* name) {
    if (current_user_index == -1)
        return -1;

    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (files[i].used &&
            strcmp(files[i].name, name) == 0 &&
            strcmp(files[i].owner, users[current_user_index].username) == 0) {
            return i;
        }
    }
    return -1;
}

int fs_create(const char* name) {
    if (current_user_index == -1)
        return -1;
    if (fs_find(name) != -1)
        return -1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {

            /* clear the entry FIRST */
            for (uint32_t j = 0; j < sizeof(File); j++)
                ((uint8_t*)&files[i])[j] = 0;

            files[i].used = 1;
            strncpy(files[i].owner, users[current_user_index].username, MAX_USERNAME);
            files[i].owner[MAX_USERNAME - 1] = '\0';
            strncpy(files[i].name, name, MAX_FILENAME);
            files[i].name[MAX_FILENAME - 1] = '\0';
            return i;
        }
    }
    return -1;
}


bool fs_delete(const char* name) {
    if (current_user_index == -1)
        return false;

    int idx = fs_find(name);
    if (idx == -1) return false;
    for (uint32_t i = 0; i < sizeof(File); i++)
        ((uint8_t*)&files[idx])[i] = 0;
    return true;
}

/* ==== User management ==== */

void users_init(void) {
    users_load_from_disk();

    if (num_users <= 0 || num_users > MAX_USERS) {
        int i;
        for (i = 0; i < MAX_USERS; i++) {
            users[i].used = false;
            users[i].username[0] = '\0';
            users[i].password[0] = '\0';
        }

        users[0].used = true;
        strcpy(users[0].username, "admin");
        strcpy(users[0].password, "admin");
        num_users = 1;

        users_save_to_disk();
    }

    current_user_index = -1;
}


bool login(const char* username, const char* password) {
    int i;
    for (i = 0; i < MAX_USERS; i++) {
        if (users[i].used &&
            strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0) {
            current_user_index = i;
            return true;
        }
    }
    return false;
}

bool add_user(const char* username, const char* password) {
    int i;
    if (num_users >= MAX_USERS) return false;

    for (i = 0; i < MAX_USERS; i++) {
        if (users[i].used &&
            strcmp(users[i].username, username) == 0) {
            return false;
        }
    }

    for (i = 0; i < MAX_USERS; i++) {
        if (!users[i].used) {
            users[i].used = true;
            strncpy(users[i].username, username, MAX_USERNAME);
            users[i].username[MAX_USERNAME - 1] = '\0';
            strncpy(users[i].password, password, MAX_PASSWORD);
            users[i].password[MAX_PASSWORD - 1] = '\0';

            num_users++;
            users_save_to_disk();
            return true;

        }
    }
    return false;
}

bool del_user(const char* username) {
    int i;
    for (i = 0; i < MAX_USERS; i++) {
        if (users[i].used &&
            strcmp(users[i].username, username) == 0) {
            users[i].used = false;
            num_users--;
            if (i == current_user_index)
            current_user_index = -1;
            users_save_to_disk();
            return true;


        }
    }
    return false;
}

/* ==== ATA PIO disk I/O (primary master, LBA28) ==== */

#define ATA_TIMEOUT 100000

static bool ata_wait(uint8_t mask, uint8_t value) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t s = inb(0x1F7);
        if ((s & mask) == value)
            return true;
    }
    return false;
}

bool ata_read_sectors(uint32_t lba, uint8_t count, uint16_t* buf) {
    if (count == 0 || count > 255)
        return false;

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);

    for (int i = 0; i < count; i++) {
        if (!ata_wait(0x88, 0x08)) {
            print_string("ATA read timeout\n");
            return false;
        }

        for (int j = 0; j < 256; j++) {
            buf[i * 256 + j] = inw(0x1F0);
        }
    }
    return true;
}




bool ata_write_sectors(uint32_t lba, uint8_t count, uint16_t* buf) {
    if (count == 0 || count > 255)
    return false;

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);

    for (int i = 0; i < count; i++) {
        if (!ata_wait(0x88, 0x08)) {
            print_string("ATA write timeout\n");
            return false;
        }

        for (int j = 0; j < 256; j++) {
            outw(0x1F0, buf[i * 256 + j]);
        }
    }

    outb(0x1F7, 0xE7);
    ata_wait(0x80, 0x00);
    return true;
}



/* Serialize files[] to disk and back */

bool fs_save_to_disk(void) {
    uint32_t total_bytes =
        sizeof(FSHeader) + sizeof(files);

    uint32_t sectors =
        (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;

    if (sectors > FS_MAX_SECTORS) {
        print_string("FS too large\n");
        return false;
        }


    FSHeader header = { FS_MAGIC, 1, sectors };

    static uint16_t buffer[256 * FS_MAX_SECTORS];
    uint8_t* dst = (uint8_t*)buffer;

    for (uint32_t i = 0; i < sectors * FS_SECTOR_SIZE; i++)
        dst[i] = 0;

    /* header */
    for (uint32_t i = 0; i < sizeof(FSHeader); i++)
        dst[i] = ((uint8_t*)&header)[i];

    /* files */
    for (uint32_t i = 0; i < sizeof(files); i++)
        dst[sizeof(FSHeader) + i] = ((uint8_t*)files)[i];

    if (sectors > FS_MAX_SECTORS) {
    print_string("FS exceeds reserved disk space\n");
    return false;
}

if (!ata_write_sectors(FS_LBA_START, (uint8_t)sectors, buffer)) {
    print_string("FS write failed\n");
    return false;
}

return true;
}


void users_save_to_disk(void) {
    uint32_t total_bytes =
        sizeof(users_magic) + sizeof(users) + sizeof(num_users);

    uint32_t sectors =
        (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;

    if (sectors > 4) {
        print_string("User DB too large\n");
        return;
    }

    static uint16_t buffer[256 * 4];
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

    if (sectors > USERS_MAX_SECTORS) {
    print_string("User DB exceeds reserved disk space\n");
    return;
}

    ata_write_sectors(USERS_LBA_START, (uint8_t)sectors, buffer);

}



bool fs_load_from_disk(void) {
    static uint16_t buffer[256 * FS_MAX_SECTORS];
    uint8_t* src = (uint8_t*)buffer;

    ata_read_sectors(FS_LBA_START, 1, buffer);

    FSHeader* header = (FSHeader*)src;

    if (header->magic != FS_MAGIC ||
    header->version != 1 ||
    header->sectors == 0 ||
    header->sectors > FS_MAX_SECTORS) {
    return false;
}


    ata_read_sectors(FS_LBA_START, header->sectors, buffer);

    uint8_t* f = src + sizeof(FSHeader);
    for (uint32_t i = 0; i < sizeof(files); i++)
        ((uint8_t*)files)[i] = f[i];
    /* sanity-check file entries */
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            files[i].owner[MAX_USERNAME - 1] = '\0';
            files[i].name[MAX_FILENAME - 1] = '\0';
            files[i].data[MAX_FILE_DATA - 1] = '\0';
        }
    }
    return true;
}


void users_load_from_disk(void) {
    uint32_t total_bytes =
        sizeof(users_magic) + sizeof(users) + sizeof(num_users);

    uint32_t sectors =
        (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;

    if (sectors > 4) {
        num_users = 0;
        return;
    }

    static uint16_t buffer[256 * 4];
    uint8_t* src = (uint8_t*)buffer;

    if (!ata_read_sectors(USERS_LBA_START, sectors, buffer)) {
    num_users = 0;
    return;
    }


    uint32_t off = 0;

    /* magic */
    uint32_t magic;
    for (uint32_t i = 0; i < sizeof(magic); i++)
    ((uint8_t*)&magic)[i] = src[off++];

    if (magic != USERS_MAGIC) {
    num_users = 0;
    return;
    }


    /* users[] */
    for (uint32_t i = 0; i < sizeof(users); i++)
        ((uint8_t*)users)[i] = src[off++];

    /* num_users */
    for (uint32_t i = 0; i < sizeof(num_users); i++)
        ((uint8_t*)&num_users)[i] = src[off++];

    /* validate */
    if (users_magic != USERS_MAGIC ||
        num_users < 0 ||
        num_users > MAX_USERS) {
        num_users = 0;  // force admin recreation
    }
}



/* ==== Commands ==== */

void cmd_help(void) {
    print_string("Uble commands:\n");
    print_string("  help                 - Show this help\n");
    print_string("  clear                - Clear screen\n");
    print_string("  echo <text>          - Print text\n");
    print_string("  login <u> <p>        - Log in\n");
    print_string("  adduser <u> <p>      - Add user (must be logged in)\n");
    print_string("  deluser <u>          - Delete user (admin only)\n");
    print_string("  whoami               - Show current user\n");
    print_string("  users                - List users\n");
    print_string("  ls                   - List files\n");
    print_string("  cat <name>           - Show file contents\n");
    print_string("  touch <name>         - Create empty file\n");
    print_string("  write <name> <text>  - Write file\n");
    print_string("  rm <name>            - Delete file\n");
}

void cmd_clear(void) {
    clear_screen();
}

void cmd_echo(const char* args) {
    if (args && *args) {
        print_string(args);
    }
    print_string("\n");
}

void cmd_whoami(void) {
    if (current_user_index == -1) {
        print_string("Not logged in\n");
    } else {
        print_string(users[current_user_index].username);
        print_string("\n");
    }
}

void cmd_users(void) {
    int i;
    for (i = 0; i < MAX_USERS; i++) {
        if (users[i].used) {
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

    int i;
    bool any = false;
    for (i = 0; i < MAX_FILES; i++) {
        if (files[i].used &&
            strcmp(files[i].owner, users[current_user_index].username) == 0) {
            print_string(files[i].name);
            print_string("\n");
            any = true;
        }
    }
    if (!any) {
        print_string("(no files)\n");
    }
}

void cmd_cat(const char* name) {
    int idx;
    if (current_user_index == -1) {
        print_string("Must be logged in\n");
        return;
    }
    if (!name || !*name) {
        print_string("Usage: cat <name>\n");
        return;
    }
    idx = fs_find(name);
    if (idx == -1) {
        print_string("No such file\n");
        return;
    }
    print_string(files[idx].data);
    print_string("\n");
}

void cmd_touch(const char* name) {
    if (current_user_index == -1) {
        print_string("Must be logged in\n");
        return;
    }
    if (!name || !*name) {
        print_string("Usage: touch <name>\n");
        return;
    }

    if (strlen(name) >= MAX_FILENAME) {
        print_string("Filename too long\n");
        return;
    }

    int idx = fs_create(name);
    if (idx == -1) {
        print_string("Cannot create file (exists or full)\n");
        return;
    }

    if (!fs_save_to_disk())
        print_string("Disk write failed\n");
}



void cmd_write(const char* name, const char* text) {
    if (current_user_index == -1) {
        print_string("Must be logged in\n");
        return;
    }
    if (!name || !*name || !text) {
        print_string("Usage: write <name> <text>\n");
        return;
    }

    // Check if filename exceeds MAX_FILENAME size
    if (strlen(name) >= MAX_FILENAME) {
        print_string("Filename too long.\n");
        return;
    }

    int idx = fs_find(name);
    if (idx == -1) {
        // If the file doesn't exist, create it
        idx = fs_create(name);
        if (idx == -1) {
            print_string("Cannot create file\n");
            return;
        }
    }

    // Copy text into the file data and save
    for (uint32_t i = 0; i < MAX_FILE_DATA; i++)
        files[idx].data[i] = 0;
    strncpy(files[idx].data, text, MAX_FILE_DATA);
    if (!fs_save_to_disk())
        print_string("Disk write failed\n");
}


void cmd_rm(const char* name) {
    if (current_user_index == -1) {
        print_string("Must be logged in\n");
        return;
    }
    if (!name || !*name) {
        print_string("Usage: rm <name>\n");
        return;
    }
    if (!fs_delete(name)) {
        print_string("No such file\n");
    }
    if (!fs_save_to_disk())
        print_string("Disk write failed\n");
}

/* Parse first word and rest */
void split_first(const char* cmd, char* first, int first_max, const char** rest_out) {
    int i = 0;
    while (*cmd == ' ') cmd++;  // Skip leading spaces
    while (*cmd && *cmd != ' ' && i < first_max - 1) {
        first[i++] = *cmd++;
    }
    first[i] = '\0';

    // Skip spaces between first argument and the rest
    while (*cmd == ' ') cmd++;
    *rest_out = cmd;
}


/* Process a full command line */
void process_command(const char* cmdline) {
    char cmd[32];
    const char* rest;

    if (!cmdline || !*cmdline) return;

    split_first(cmdline, cmd, sizeof(cmd), &rest);

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(rest);
    } else if (strcmp(cmd, "whoami") == 0) {
        cmd_whoami();
    } else if (strcmp(cmd, "users") == 0) {
        cmd_users();
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "cat") == 0) {
        char name[32];
        const char* dummy;
        split_first(rest, name, sizeof(name), &dummy);
        cmd_cat(name);
    } else if (strcmp(cmd, "touch") == 0) {
        char name[32];
        const char* dummy;
        split_first(rest, name, sizeof(name), &dummy);
        cmd_touch(name);
    } else if (strcmp(cmd, "write") == 0) {
        char name[32];
        const char* text;
        split_first(rest, name, sizeof(name), &text);
        cmd_write(name, text);
    } else if (strcmp(cmd, "rm") == 0) {
        char name[32];
        const char* dummy;
        split_first(rest, name, sizeof(name), &dummy);
        cmd_rm(name);
    } else if (strcmp(cmd, "login") == 0) {
        char user[32];
        const char* pass;
        split_first(rest, user, sizeof(user), &pass);
        if (!*user || !*pass) {
            print_string("Usage: login <user> <pass>\n");
        } else if (login(user, pass)) {
            print_string("Login successful\n");
        } else {
            print_string("Invalid credentials\n");
        }
    } else if (strcmp(cmd, "adduser") == 0) {
        char user[32];
        const char* pass;
        if (current_user_index == -1) {
            print_string("Must be logged in\n");
            return;
        }
        split_first(rest, user, sizeof(user), &pass);
        if (!*user || !*pass) {
            print_string("Usage: adduser <user> <pass>\n");
        } else if (add_user(user, pass)) {
            print_string("User added\n");
        } else {
            print_string("Cannot add user\n");
        }
    } else if (strcmp(cmd, "deluser") == 0) {
        char user[32];
        const char* dummy;
        if (current_user_index == -1) {
            print_string("Must be logged in\n");
            return;
        }
        if (strcmp(users[current_user_index].username, "admin") != 0) {
            print_string("Only admin can delete users\n");
            return;
        }
        split_first(rest, user, sizeof(user), &dummy);
        if (!*user) {
            print_string("Usage: deluser <user>\n");
        } else if (del_user(user)) {
            print_string("User deleted\n");
        } else {
            print_string("No such user\n");
        }
    } else {
        print_string("Unknown command\n");
    }
}

/* ==== Shell loop ==== */

void shell_loop(void) {
    char cmdline[MAX_CMD_LENGTH];

    while (1) {
        print_string("\n");
        if (current_user_index == -1) {
            print_string("Uble$ ");
        } else {
            print_string("Uble/");
            print_string(users[current_user_index].username);
            print_string("$ ");
        }
        read_line(cmdline, MAX_CMD_LENGTH);
        process_command(cmdline);
    }
}

/* ==== Kernel entry ==== */

void kernel_main(void) {
    clear_screen();
    print_string("Uble OS\n");
    print_string("Default user: admin / admin\n");
    print_string("Type 'help' for commands.\n\n");

    users_init();   // loads users from disk
    fs_init();

    shell_loop();
}

void poweroff(void) {
    /* QEMU / Bochs / VirtualBox */
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);

    __asm__ volatile ("cli; hlt");
}
