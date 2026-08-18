#include "fs.h"
#include "io.h"
extern void print_string(const char *s);

extern int strcmp(const char *a, const char *b);
extern char *strncpy(char *dst, const char *src, int n);

extern User users[];
extern int current_user_index;

File files[MAX_FILES];

void fs_clear_all(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        for (uint32_t j = 0; j < sizeof(File); j++) {
            ((uint8_t *)&files[i])[j] = 0;
        }
    }
}

void fs_init(void) {
    if (!fs_load_from_disk()) {
        fs_clear_all();
        fs_save_to_disk();
    }
}

int fs_find(const char *name) {
    if (current_user_index == -1)
        return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0 && strcmp(files[i].owner,users[current_user_index].username) == 0) {
            return i;
        }
    }
    return -1;
}

uint32_t fs_next_free_sector(void) {
    for (uint32_t candidate = FS_DATA_START; candidate < FS_DATA_START + FS_MAX_SECTORS; candidate++) {
        bool conflict = false;
        for (int i = 0; i < MAX_FILES; i++) {
            if (!files[i].used)
                continue;
            if (files[i].size == 0)
                continue;
            uint32_t start = files[i].start_sector;
            uint32_t sectors = (files[i].size + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
            if (candidate >= start && candidate < start + sectors) {
                conflict = true;
                break;
            }
        }
        if (!conflict)
            return candidate;
    }
    return 0;
}

int fs_create(const char *name) {
    if (current_user_index == -1)
        return -1;
    if (fs_find(name) != -1)
        return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used)
            continue;
        for (uint32_t j = 0; j < sizeof(File); j++) {
            ((uint8_t *)&files[i])[j] = 0;
        }
        files[i].used = 1;
        strncpy(files[i].owner,users[current_user_index].username,MAX_USERNAME);
        files[i].owner[MAX_USERNAME - 1] = '\0';
        strncpy(files[i].name,name,MAX_FILENAME
        );
        files[i].name[MAX_FILENAME - 1] = '\0';
        files[i].start_sector = 0;
        files[i].size = 0;
        return i;
    }
    return -1;
}

bool fs_delete(const char *name) {
    if (current_user_index == -1)
        return false;
    int idx = fs_find(name);
    if (idx == -1)
        return false;
    uint32_t sectors = (files[idx].size + 511) / 512;
    if (sectors > 0) {
        fs_clear_sectors(files[idx].start_sector,sectors);
    }
    for (uint32_t i = 0; i < sizeof(File); i++) {
        ((uint8_t *)&files[idx])[i] = 0;
    }
    files[idx].used = 0;
    files[idx].size = 0;
    return true;
}

#define ATA_TIMEOUT 5000

static bool ata_wait(uint8_t mask,uint8_t value) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t s = inb(0x1F7);
        if (s & 0x01)
            return false;
        if (s & 0x20)
            return false;
        if ((s & mask) == value)
            return true;
    }
    return false;
}

bool ata_read_sectors(uint32_t lba,uint8_t count,uint16_t *buf) {
    if (count == 0)
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

bool ata_write_sectors(uint32_t lba,uint8_t count,uint16_t *buf) {
    if (count == 0)
        return false;
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);
    for (int i = 0; i < count; i++) {
        if (!ata_wait(0x08, 0x08)) {
            print_string("ATA write DRQ timeout\n");
            return false;
        }
        for (int j = 0; j < 256; j++) {
            outw(0x1F0,buf[i * 256 + j]);
        }
        if (!ata_wait(0x80, 0x00)) {
            print_string("ATA write finish timeout\n");
            return false;
        }
    }
    return true;
}

void fs_clear_sectors(uint32_t start_sector,uint32_t count) {
    static uint8_t zero_buffer[512];
    for (uint32_t i = 0; i < 512; i++) {
        zero_buffer[i] = 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        ata_write_sectors(start_sector + i,1,(uint16_t *)zero_buffer);
    }
}

bool fs_save_to_disk(void) {
    static uint8_t sector_buf[FS_SECTOR_SIZE];
    FSHeader header;
    header.magic = FS_MAGIC;
    header.version = FS_VERSION;
    header.bytes = sizeof(FSHeader);
    header.file_count = 0;
    header.reserved = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used)
            continue;
        uint32_t name_length = 0;
        while (name_length < MAX_FILENAME && files[i].name[name_length] != '\0') {
            name_length++;
        }
        header.bytes += sizeof(FSFileHeader);
        header.bytes += name_length;
        header.file_count++;
        if (header.file_count == 0xFFFF)
            return false;
    }
    uint32_t sectors = (header.bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors == 0)
        sectors = 1;
    if (sectors > FS_MAX_SECTORS) {
        print_string("FS metadata is too large\n");
        return false;
    }
    uint32_t bytes_written = 0;
    for (uint32_t s = 0; s < sectors; s++) {
        for (uint32_t i = 0; i < FS_SECTOR_SIZE; i++)
            sector_buf[i] = 0;
        uint32_t sector_start = s * FS_SECTOR_SIZE;
        uint32_t sector_end = sector_start + FS_SECTOR_SIZE;
        for (uint32_t pos = sector_start; pos < sector_end && pos < header.bytes; pos++) {
            uint32_t current = 0;
            if (pos < sizeof(FSHeader)) {
                sector_buf[pos - sector_start] = ((uint8_t *)&header)[pos];
                continue;
            }
            current = sizeof(FSHeader);
            bool written = false;
            for (int f = 0; f < MAX_FILES; f++) {
                if (!files[f].used)
                    continue;
                uint32_t name_length = 0;
                while (name_length < MAX_FILENAME && files[f].name[name_length] != '\0') {
                    name_length++;
                }
                uint32_t record_size = sizeof(FSFileHeader) + name_length;
                if (pos >= current && pos < current + record_size) {
                    uint32_t offset = pos - current;
                    FSFileHeader record;
                    for (int u = 0; u < MAX_FILES; u++) {
                        if (u == current_user_index)
                            record.owner_index = (uint16_t)u;
                    }
                    record.owner_index = 0xFFFF;
                    for (int u = 0; u < MAX_FILES; u++) {
                        if (strcmp(files[f].owner,users[u].username) == 0) {
                            record.owner_index = (uint16_t)u;
                            break;
                        }
                    }
                    record.name_length = (uint8_t)name_length;
                    record.flags = 0;
                    record.start_sector = files[f].start_sector;
                    record.size = files[f].size;
                    if (offset < sizeof(FSFileHeader)) {
                        sector_buf[pos - sector_start] = ((uint8_t *)&record)[offset];
                    } else {
                        uint32_t name_offset = offset - sizeof(FSFileHeader);
                        sector_buf[pos - sector_start] = (uint8_t)files[f].name[name_offset];
                    }
                    written = true;
                    break;
                }
                current += record_size;
            }
            if (!written) {
                sector_buf[pos - sector_start] = 0;
            }
        }
        if (!ata_write_sectors(FS_LBA_START + s,1,(uint16_t *)sector_buf)) {
            print_string("FS write execution dropped.\n");
            return false;
        }
        bytes_written += FS_SECTOR_SIZE;
    }
    outb(0x1F7, 0xE7);
    if (!ata_wait(0x80, 0x00)) {
        print_string("ATA cache flush failed\n");
        return false;
    }
    print_string("Filesystem saved to disk\n");
    return true;
}

bool fs_load_from_disk(void) {
    static uint8_t sector_buf[FS_SECTOR_SIZE];
    if (!ata_read_sectors(FS_LBA_START,1,(uint16_t *)sector_buf)) {
        return false;
    }
    FSHeader header;
    for (uint32_t i = 0; i < sizeof(FSHeader); i++) {
        ((uint8_t *)&header)[i] = sector_buf[i];
    }
    if (header.magic != FS_MAGIC)
        return false;
    if (header.version != FS_VERSION)
        return false;
    if (header.bytes < sizeof(FSHeader))
        return false;
    if (header.file_count > MAX_FILES)
        return false;
    uint32_t sectors = (header.bytes + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors == 0)
        sectors = 1;
    if (sectors > FS_MAX_SECTORS)
        return false;
    fs_clear_all();
    uint32_t stream_pos = sizeof(FSHeader);
    for (uint32_t file_number = 0; file_number < header.file_count; file_number++) {
        FSFileHeader record;
        for (uint32_t b = 0; b < sizeof(FSFileHeader); b++) {
            uint32_t absolute = stream_pos + b;
            uint32_t sector = absolute / FS_SECTOR_SIZE;
            uint32_t offset = absolute % FS_SECTOR_SIZE;
            if (!ata_read_sectors(FS_LBA_START + sector,1,(uint16_t *)sector_buf)) {
                return false;
            }
            ((uint8_t *)&record)[b] = sector_buf[offset];
        }
        stream_pos += sizeof(FSFileHeader);
        if (record.name_length >= MAX_FILENAME)
            return false;
        if (record.owner_index >= MAX_USERS)
            return false;
        if (stream_pos + record.name_length > header.bytes) {
            return false;
        }
        int file_index = -1;
        for (int i = 0; i < MAX_FILES; i++) {
            if (!files[i].used) {
                file_index = i;
                break;
            }
        }
        if (file_index == -1)
            return false;
        files[file_index].used = 1;
        for (uint32_t i = 0; i < MAX_USERNAME; i++) {
            files[file_index].owner[i] = users[record.owner_index].username[i];
            if (users[record.owner_index].username[i] == '\0')
                break;
        }
        for (uint32_t b = 0; b < record.name_length; b++) {
            uint32_t absolute = stream_pos + b;
            uint32_t sector = absolute / FS_SECTOR_SIZE;
            uint32_t offset = absolute % FS_SECTOR_SIZE;
            if (!ata_read_sectors(FS_LBA_START + sector,1,(uint16_t *)sector_buf)) {
                return false;
            }
            files[file_index].name[b] = sector_buf[offset];
        }
        files[file_index].name[record.name_length] = '\0';
        files[file_index].start_sector = record.start_sector;
        files[file_index].size = record.size;
        stream_pos += record.name_length;
    }
    return true;
}