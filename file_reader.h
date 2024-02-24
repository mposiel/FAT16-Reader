//
// Created by maks on 23.12.23.
//

#ifndef PROJECT1_FILE_READER_H
#define PROJECT1_FILE_READER_H

#include <stdint.h>
#include <stdio.h>

struct FAT0_35 {
    uint8_t assembly_code_instructions[3];
    uint8_t OEM_name_in_ASCII[8];
    uint16_t Bytes_per_sector;
    uint8_t Sectors_per_cluster;
    uint16_t Size_of_reserved_area;
    uint8_t Number_of_FATs;
    uint16_t Maximum_number_of_files_in_the_root_directory;
    uint16_t Number_of_sectors_in_the_file_system;
    uint8_t Media_type;
    uint16_t Size_of_each_FAT;
    uint16_t Sectors_per_track_in_storage_device;
    uint16_t Number_of_heads_in_storage_device;
    uint32_t Number_of_sectors_before_the_start_partition;
    uint32_t Number_of_sectors_in_the_file_system_2;
}__attribute__((__packed__));


struct disk_t {
    FILE *f;
    uint16_t num_of_sectors;
    uint16_t bytes_per_sector;

};


struct disk_t *disk_open_from_file(const char *volume_file_name);

int disk_read(struct disk_t *pdisk, int32_t first_sector, void *buffer, int32_t sectors_to_read);

int disk_close(struct disk_t *pdisk);

struct FAT {
    struct FAT0_35 head;
    uint8_t BIOS_INT_13h;
    uint8_t Not_used;
    uint8_t Extended_boot_signature;
    uint32_t Volume_serial_number;
    uint8_t Volume_label[11];
    uint8_t File_system_type_level[8];
    uint16_t Signature_value;
    uint16_t root_begin;
} __attribute__((__packed__));

struct volume_t {
    struct disk_t *disk;
    struct FAT info;
};


struct volume_t *fat_open(struct disk_t *pdisk, uint32_t first_sector);

int fat_close(struct volume_t *pvolume);


struct file_t {
    struct volume_t *volume;
    char file_name[256];
    long offset;
    uint16_t first_cluster;
    uint32_t size;
};

struct file_t *file_open(struct volume_t *pvolume, const char *file_name);

int file_close(struct file_t *stream);

size_t file_read(void *ptr, size_t size, size_t nmemb, struct file_t *stream);

int32_t file_seek(struct file_t *stream, int32_t offset, int whence);

struct dir_entry_t {
    char name[11];
    uint32_t size;
    int is_archived;
    int is_readonly;
    int is_system;
    int is_hidden;
    int is_directory;
};

struct dir_t {
    struct volume_t *volume;
    long offset;
};

struct dir_t *dir_open(struct volume_t *pvolume, const char *dir_path);

int dir_read(struct dir_t *pdir, struct dir_entry_t *pentry);

int dir_close(struct dir_t *pdir);

//3030


#endif //PROJECT1_FILE_READER_H
