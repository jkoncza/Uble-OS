/* Uble OS kernel - no #include <...> */
__attribute__((section(".multiboot")))
const unsigned long multiboot_header[] = {
    0x1BADB002,                 // magic
    0x00000003,                 // flags (ALIGN | MEMINFO)
    -(0x1BADB002 + 0x00000003)  // checksum
};

/* Basic types */
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef int            bool;

#define true  1
#define false 0

/* VGA text mode */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEM     ((uint16_t*)0xB8000)
#define COLOR       0x0F  /* white on black */

/* Shell / users / filesystem */
#define MAX_CMD_LENGTH  80
#define MAX_USERS       10
#define MAX_FILES       32
#define MAX_FILENAME    16
#define MAX_FILE_DATA   256

/* Disk layout for FS */
#define FS_LBA_START    2048u  /* where we store FS on disk */
#define FS_SECTOR_SIZE  512u

/* User structure */
typedef struct {
    char username[MAX_CMD_LENGTH];
    char password[MAX_CMD_LENGTH];
    bool used;
} User;

/* File structure (simple in-memory FS) */
typedef struct __attribute__((packed)) {
    char name[MAX_FILENAME];
    char data[MAX_FILE_DATA];
    uint8_t used;
} File;

/* Globals */
static int cursor_row = 0;
static int cursor_col = 0;

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
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].name[0] = '\0';
        files[i].data[0] = '\0';
    }
}

void fs_init(void) {
    fs_clear_all();

    files[0].used = 1;
    strncpy(files[0].name, "readme", MAX_FILENAME);
    strncpy(files[0].data,
            "Welcome to Uble OS!\n"
            "Type 'help' to see commands.\n",
            MAX_FILE_DATA);
}

int fs_find(const char* name) {
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int fs_create(const char* name) {
    int i;
    if (fs_find(name) != -1) {
        return -1;
    }
    for (i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            strncpy(files[i].name, name, MAX_FILENAME);
            files[i].data[0] = '\0';
            return i;
        }
    }
    return -1;
}

bool fs_delete(const char* name) {
    int idx = fs_find(name);
    if (idx == -1) return false;
    files[idx].used = 0;
    return true;
}

/* ==== User management ==== */

void users_init(void) {
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
            strcpy(users[i].username, username);
            strcpy(users[i].password, password);
            num_users++;
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
            return true;
        }
    }
    return false;
}

/* ==== ATA PIO disk I/O (primary master, LBA28) ==== */

static void ata_wait_busy_clear(void) {
    while (inb(0x1F7) & 0x80) {
        /* wait while BSY set */
    }
}

static void ata_wait_drq(void) {
    uint8_t s;
    do {
        s = inb(0x1F7);
    } while (!(s & 0x08)); /* DRQ */
}

void ata_read_sectors(uint32_t lba, uint8_t count, uint16_t* buf) {
    int i, j;

    ata_wait_busy_clear();

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t)(lba & 0xFF));
    outb(0x1F4, (uint8_t)((lba >> 8) & 0xFF));
    outb(0x1F5, (uint8_t)((lba >> 16) & 0xFF));
    outb(0x1F7, 0x20); /* READ SECTORS */

    for (i = 0; i < count; i++) {
        ata_wait_busy_clear();
        ata_wait_drq();
        for (j = 0; j < 256; j++) {
            buf[i * 256 + j] = inw(0x1F0);
        }
    }
}

void ata_write_sectors(uint32_t lba, uint8_t count, const uint16_t* buf) {
    int i, j;

    ata_wait_busy_clear();

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t)(lba & 0xFF));
    outb(0x1F4, (uint8_t)((lba >> 8) & 0xFF));
    outb(0x1F5, (uint8_t)((lba >> 16) & 0xFF));
    outb(0x1F7, 0x30); /* WRITE SECTORS */

    for (i = 0; i < count; i++) {
        ata_wait_busy_clear();
        ata_wait_drq();
        for (j = 0; j < 256; j++) {
            outw(0x1F0, buf[i * 256 + j]);
        }
    }
}

/* Serialize files[] to disk and back */

void fs_save_to_disk(void) {
    uint32_t total_bytes = sizeof(files);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    uint16_t buffer[(sizeof(files) + 1) / 2]; /* round up to words */
    uint32_t i;

    /* Copy files[] into buffer as raw bytes */
    uint8_t* src = (uint8_t*)files;
    uint8_t* dst = (uint8_t*)buffer;
    for (i = 0; i < sizeof(files); i++) {
        dst[i] = src[i];
    }
    /* Zero pad remaining bytes in last sector if any */
    for (; i < sectors * FS_SECTOR_SIZE; i++) {
        dst[i] = 0;
    }

    ata_write_sectors(FS_LBA_START, (uint8_t)sectors, buffer);
    print_string("Filesystem saved to disk.\n");
}

void fs_load_from_disk(void) {
    uint32_t total_bytes = sizeof(files);
    uint32_t sectors = (total_bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    uint16_t buffer[(sizeof(files) + 1) / 2];
    uint32_t i;

    ata_read_sectors(FS_LBA_START, (uint8_t)sectors, buffer);

    uint8_t* src = (uint8_t*)buffer;
    uint8_t* dst = (uint8_t*)files;
    for (i = 0; i < sizeof(files); i++) {
        dst[i] = src[i];
    }

    print_string("Filesystem loaded from disk.\n");
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
    print_string("  savefs               - Save filesystem to disk\n");
    print_string("  loadfs               - Load filesystem from disk\n");
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
    int i;
    bool any = false;
    for (i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
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
    if (!name || !*name) {
        print_string("Usage: touch <name>\n");
        return;
    }
    if (fs_create(name) == -1) {
        print_string("Cannot create file (exists or full)\n");
    }
}

void cmd_write(const char* name, const char* text) {
    int idx;
    if (!name || !*name || !text) {
        print_string("Usage: write <name> <text>\n");
        return;
    }
    idx = fs_find(name);
    if (idx == -1) {
        idx = fs_create(name);
        if (idx == -1) {
            print_string("Cannot create file\n");
            return;
        }
    }
    strncpy(files[idx].data, text, MAX_FILE_DATA);
}

void cmd_rm(const char* name) {
    if (!name || !*name) {
        print_string("Usage: rm <name>\n");
        return;
    }
    if (!fs_delete(name)) {
        print_string("No such file\n");
    }
}

/* Parse first word and rest */
void split_first(const char* cmd, char* first, int first_max,
                 const char** rest_out) {
    int i = 0;
    while (*cmd == ' ') cmd++;
    while (*cmd && *cmd != ' ' && i < first_max - 1) {
        first[i++] = *cmd++;
    }
    first[i] = '\0';
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
    } else if (strcmp(cmd, "savefs") == 0) {
        fs_save_to_disk();
    } else if (strcmp(cmd, "loadfs") == 0) {
        fs_load_from_disk();
    } else {
        print_string("Unknown command\n");
    }
}

/* ==== Shell loop ==== */

void shell_loop(void) {
    char cmdline[MAX_CMD_LENGTH];

    while (1) {
        print_string("\nUble$ ");
        read_line(cmdline, MAX_CMD_LENGTH);
        process_command(cmdline);
    }
}

/* ==== Kernel entry ==== */

void kernel_main(void) {
    clear_screen();
    print_string("Uble OS kernel\n");
    print_string("Default user: admin / admin\n");
    print_string("Type 'help' for commands.\n\n");

    users_init();
    fs_init();

    shell_loop();
}
