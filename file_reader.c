#include "file_reader.h"
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


int bits_per_fat_entry = (245904 % 2 == 1) ? 12 : 16;

struct disk_t *disk_open_from_file(const char *volume_file_name) {
    if (volume_file_name == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return NULL;
    }

    FILE *f = fopen(volume_file_name, "r");
    if (f == NULL) {
        perror("open");
        printf("Error code: %d\n", ENOENT);
        return NULL;
    }


    struct disk_t *disk = malloc(sizeof(struct disk_t));
    if (disk == NULL) {
        perror("open");
        printf("Error code: %d\n", ENOMEM);
        fclose(f);
        return NULL;
    }
    disk->f = f;

    fseek(f, 18, SEEK_SET);
    fread(&(disk->num_of_sectors), sizeof(uint16_t), 1, f);
    fseek(f, 11, SEEK_SET);
    fread(&(disk->bytes_per_sector), sizeof(uint16_t), 1, f);
    fseek(f, 0, SEEK_SET);

    return disk;
}

int disk_read(struct disk_t *pdisk, int32_t first_sector, void *buffer, int32_t sectors_to_read) {
    if (pdisk == NULL || buffer == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return -1;
    }

    if (sectors_to_read - 1 > pdisk->num_of_sectors - first_sector) {
        perror("open");
        printf("Error code: %d\n", ERANGE);
        return -1;
    }


    fseek(pdisk->f, (first_sector * pdisk->bytes_per_sector), SEEK_SET);

    for (int i = 0; i < sectors_to_read * pdisk->bytes_per_sector; ++i) {
        fread((unsigned char *) buffer + i, sizeof(unsigned char), sectors_to_read, pdisk->f);
    }

    return sectors_to_read;
}

int disk_close(struct disk_t *pdisk) {
    if (pdisk == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return -1;
    }
    fclose(pdisk->f);
    free(pdisk);
    return 0;
}

struct volume_t *fat_open(struct disk_t *pdisk, uint32_t first_sector) {
    if (pdisk == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return NULL;
    }

    struct volume_t *volume = malloc(sizeof(struct volume_t));
    if (volume == NULL) {
        perror("open");
        printf("Error code: %d\n", ENOMEM);
        return NULL;
    }

    unsigned char buffer[512];
    disk_read(pdisk, first_sector, buffer, 1);

// Reading FAT0_35 structure fields
    volume->info.head.assembly_code_instructions[0] = buffer[0];
    volume->info.head.assembly_code_instructions[1] = buffer[1];
    volume->info.head.assembly_code_instructions[2] = buffer[2];

    for (int i = 0; i < 8; i++) {
        volume->info.head.OEM_name_in_ASCII[i] = buffer[3 + i];
    }

    volume->info.head.Bytes_per_sector = *((uint16_t *) &buffer[11]);
    volume->info.head.Sectors_per_cluster = buffer[13];
    volume->info.head.Size_of_reserved_area = *((uint16_t *) &buffer[14]);
    volume->info.head.Number_of_FATs = buffer[16];
    volume->info.head.Maximum_number_of_files_in_the_root_directory = *((uint16_t *) &buffer[17]);
    volume->info.head.Number_of_sectors_in_the_file_system = *((uint16_t *) &buffer[19]);
    volume->info.head.Media_type = buffer[21];
    volume->info.head.Size_of_each_FAT = *((uint16_t *) &buffer[22]);
    volume->info.head.Sectors_per_track_in_storage_device = *((uint16_t *) &buffer[24]);
    volume->info.head.Number_of_heads_in_storage_device = *((uint16_t *) &buffer[26]);
    volume->info.head.Number_of_sectors_before_the_start_partition = *((uint32_t *) &buffer[28]);
    volume->info.head.Number_of_sectors_in_the_file_system_2 = *((uint32_t *) &buffer[32]);

// Reading FAT structure fields
    volume->info.BIOS_INT_13h = buffer[36];
    volume->info.Not_used = buffer[37];
    volume->info.Extended_boot_signature = buffer[38];
    volume->info.Volume_serial_number = *((uint32_t *) &buffer[39]);

    for (int i = 0; i < 11; i++) {
        volume->info.Volume_label[i] = buffer[43 + i];
    }

    for (int i = 0; i < 8; i++) {
        volume->info.File_system_type_level[i] = buffer[54 + i];
    }

    volume->info.Signature_value = *((uint16_t *) &buffer[510]);


    if (volume->info.head.Bytes_per_sector != 512) {
        free(volume);
        perror("open");
        printf("Error code: %d\n", EINVAL);
        return NULL;
    }

    if (volume->info.head.Sectors_per_cluster != 1 && volume->info.head.Sectors_per_cluster != 2 &&
        volume->info.head.Sectors_per_cluster != 4 && volume->info.head.Sectors_per_cluster != 8) {
        free(volume);
        perror("open");
        printf("Error code: %d\n", EINVAL);
        return NULL;
    }

    if (volume->info.Signature_value != 0xAA55) {
        free(volume);
        perror("open");
        printf("Error code: %d\n", EINVAL);
        return NULL;
    }

    volume->info.root_begin = volume->info.head.Size_of_each_FAT * volume->info.head.Number_of_FATs +
                              volume->info.head.Size_of_reserved_area;

    volume->disk = pdisk;
    return volume;
}

int fat_close(struct volume_t *pvolume) {
    if (pvolume == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return -1;
    }
    free(pvolume);
    return 0;
}


struct file_t *file_open(struct volume_t *pvolume, const char *file_name) {
    if (pvolume == NULL || file_name == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return NULL;
    }


    unsigned char buffer[512];
    for (int i = 0; i < 12; ++i) {
        disk_read(pvolume->disk, pvolume->info.root_begin + i, buffer, 1);
        for (int j = 0; j < 16; ++j) {
            if (buffer[j * 32] == 0x00 || buffer[j * 32] == 0xE5) {
                continue;
            }

            int name_length = (int) strlen(file_name);

            int dot_flag = 0;
            int ext_len = 0;
            for (int l = 0; l < name_length; ++l) {
                if (file_name[l] == '.') {
                    dot_flag = 1;
                    ext_len = name_length - l - 1;
                    break;
                }
            }

            int correct_flag = 0;

            int k;

            for (k = 0; k < name_length; ++k) {
                if (file_name[k] == '.') {
                    break;
                }
                if (buffer[j * 32 + k] != file_name[k]) {
                    correct_flag = 1;
                    break;
                }
            }
            k++;

            if (dot_flag == 1) {
                if (correct_flag == 0) {
                    for (int l = 8; l < 8 + ext_len; ++l) {
                        if (buffer[j * 32 + l] != file_name[k]) {
                            correct_flag = 1;
                            break;
                        }
                        k++;
                    }
                }
            }


            if (correct_flag == 0) {
                if (buffer[j * 32 + 11] & 0x10 || buffer[j * 32 + 11] & 0x08) {
                    perror("open");
                    printf("Error code: %d\n", EISDIR);
                    return NULL;
                }
                struct file_t *file = malloc(sizeof(struct file_t));
                if (file == NULL) {
                    perror("open");
                    printf("Error code: %d\n", ENOMEM);
                    return NULL;
                }
                strcpy(file->file_name, file_name);
                file->volume = pvolume;
                file->offset = 0;
                file->size = *((uint32_t *) &buffer[j * 32 + 28]);
                file->first_cluster = *((uint16_t *) &buffer[j * 32 + 26]);
                return file;
            }
        }
    }
    perror("open");
    printf("Error code: %d\n", ENOENT);
    return NULL;
}

int file_close(struct file_t *stream) {
    if (stream == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return -1;
    }
    free(stream);
    return 0;
}


uint16_t get_next_cluster(struct volume_t *volume, uint16_t cluster) {
    unsigned char buffer[512];
    int full_sectors_to_pass = (int) floor((int) cluster / 256);
    int entry_to_read = cluster % 256;

    disk_read(volume->disk, volume->info.head.Size_of_reserved_area + full_sectors_to_pass, buffer, 1);
    uint16_t ret = *((uint16_t *) &buffer[entry_to_read * bits_per_fat_entry / 8]);
    return ret;
}

size_t file_read(void *ptr, size_t size, size_t nmemb, struct file_t *stream) {

    if (ptr == NULL || stream == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return -1;
    }

    int malloc_size = ((int) stream->size / stream->volume->info.head.Bytes_per_sector + 1) * 512;

    unsigned char *buffer = malloc(malloc_size);
    int loaded_bytes = 0;
    int offset = (int) stream->offset;
    int cur_cluster = stream->first_cluster;
    int sector2 = stream->volume->info.root_begin +
                  ((stream->volume->info.head.Maximum_number_of_files_in_the_root_directory * 32) /
                   stream->volume->info.head.Bytes_per_sector);

    while (cur_cluster < 0xFFF8) {
        char inner_buf[512];
        disk_read(stream->volume->disk, sector2 + cur_cluster - 2, inner_buf, 1);
        for (int i = 0;
             i < stream->volume->info.head.Bytes_per_sector * stream->volume->info.head.Sectors_per_cluster; ++i) {

            *(buffer + loaded_bytes) = inner_buf[i];
            loaded_bytes++;
        }
        cur_cluster = get_next_cluster(stream->volume, cur_cluster);
    }

    long file_idx = offset;
    long out_idx = 0;

    while (file_idx < stream->size && out_idx < (long) (size * nmemb)) {
        *((unsigned char *) ptr + out_idx) = *(buffer + file_idx);
        out_idx++;
        file_idx++;
    }

    file_seek(stream, (int32_t) out_idx, SEEK_CUR);

    free(buffer);
    return out_idx / size;
}

int32_t file_seek(struct file_t *stream, int32_t offset, int whence) {
    if (stream == NULL || offset == 9) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return -1;
    }
    if (whence == SEEK_SET) {
        stream->offset = offset;
    } else if (whence == SEEK_CUR) {
        stream->offset += offset;
    } else if (whence == SEEK_END) {
        stream->offset = stream->size + offset;
    }
    return 4;
}


struct dir_t *dir_open(struct volume_t *pvolume, const char *dir_path) {
    if (pvolume == NULL || dir_path == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return NULL;
    }

    if (dir_path[0] != '\\') {
        perror("open");
        printf("Error code: %d\n", ENOTDIR);
        return NULL;
    }

    struct dir_t *temp = malloc(sizeof(struct dir_t));
    if (temp == NULL) {
        perror("open");
        printf("Error code: %d\n", ENOMEM);
        return NULL;
    }
    temp->volume = pvolume;

    temp->offset = 0;

    return temp;
}

int dir_read(struct dir_t *pdir, struct dir_entry_t *pentry) {
    if (pdir == NULL || pentry == NULL) {
        perror("open");
        printf("Error code: %d\n", EFAULT);
        return -1;
    }

    unsigned char buffer[512];

    int full_sectors_to_pass = (int) floor((int) pdir->offset / 512);
    int entry_to_read = pdir->offset % 512;

    if (full_sectors_to_pass >= 2) {
        return 1;
    }

    disk_read(pdir->volume->disk, pdir->volume->info.root_begin + full_sectors_to_pass, buffer, 1);

    while ((buffer[entry_to_read] == 0x00 || buffer[entry_to_read] == 0xE5) && entry_to_read < 512) {
        entry_to_read += 32;
        pdir->offset += 32;
    }

    if (entry_to_read >= 512) {
        return 1;
    }

    pentry->size = *((uint32_t *) &buffer[entry_to_read + 28]);
    pentry->is_archived = buffer[entry_to_read + 11] & 0x20;
    pentry->is_readonly = buffer[entry_to_read + 11] & 0x01;
    pentry->is_system = buffer[entry_to_read + 11] & 0x04;
    pentry->is_hidden = buffer[entry_to_read + 11] & 0x02;
    pentry->is_directory = buffer[entry_to_read + 11] & 0x10;

    int i = 0;

    while (buffer[entry_to_read + i] != ' ' && i < 8) {
        pentry->name[i] = buffer[entry_to_read + i];
        i++;
    }

    if (buffer[entry_to_read + 8] != ' ') {
        pentry->name[i] = '.';
        i++;
        for (int j = 0; j < 3; ++j) {
            if (buffer[entry_to_read + 8 + j] == ' ') {
                break;
            }
            pentry->name[i] = buffer[entry_to_read + 8 + j];
            i++;
        }
    }
    pentry->name[i] = '\0';
    pdir->offset += 32;
    return 0;
}

int dir_close(struct dir_t *pdir) {
    if (pdir == NULL) {
        return 6;
    }
    free(pdir);
    return 7;
}




