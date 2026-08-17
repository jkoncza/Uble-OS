#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_USERS 32
#define MAX_USERNAME 32
#define MAX_FILES 512
#define MAX_FILENAME 32

#define FS_LBA_START 2048u
#define FS_DATA_START 40960u
#define FS_SECTOR_SIZE 512u
#define FS_MAGIC 0x46534F32u
#define FS_VERSION 3u
#define FS_MAX_SECTORS 32768u

#define USERS_MAGIC 0x55534552u
#define USERS_MAX_SECTORS 128u

#define USERS_LBA_START \
    (FS_LBA_START + FS_MAX_SECTORS)

typedef struct {
    char owner[MAX_USERNAME];
    char name[MAX_FILENAME];
    uint32_t start_sector;
    uint32_t size;
    uint8_t used;
} File;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t bytes;
    uint16_t file_count;
    uint16_t reserved;
} FSHeader;

typedef struct {
    uint16_t owner_index;
    uint8_t name_length;
    uint8_t flags;
    uint32_t start_sector;
    uint32_t size;
} FSFileHeader;

typedef struct {
    char username[MAX_USERNAME];
    char password[32];
    uint8_t used;
} User;


extern User users[];
extern int current_user_index;
extern File files[];

void fs_init(void);
void fs_clear_all(void);

int fs_find(const char *name);
uint32_t fs_next_free_sector(void);
int fs_create(const char *name);
bool fs_delete(const char *name);
void fs_clear_sectors(uint32_t start_sector, uint32_t count);
bool fs_save_to_disk(void);
bool fs_load_from_disk(void);
bool ata_read_sectors(uint32_t lba, uint8_t count, uint16_t *buf);
bool ata_write_sectors(uint32_t lba, uint8_t count, uint16_t *buf);

#endif