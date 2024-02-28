# Project: FAT File System Reader

### Overview
This project is a C implementation for reading and interacting with FAT file systems. It provides functionalities to open disk images, read files and directories, seek within files, and close opened resources. This project was made as part of my Operating Systems 2 course.

### Files
- **file_reader.h**: Header file containing struct definitions and function prototypes.
- **file_reader.c**: Source file implementing the functionalities defined in `file_reader.h`.

### Struct Definitions
- `struct FAT0_35`: Represents the structure of the FAT file system's Boot Sector.
- `struct disk_t`: Represents a disk, containing file descriptor, number of sectors, and bytes per sector.
- `struct FAT`: Represents FAT file system information including Boot Sector, Volume Label, and File System Type.
- `struct volume_t`: Represents a volume containing disk and FAT information.
- `struct file_t`: Represents a file within a volume, storing file metadata like name, offset, size, and first cluster.
- `struct dir_entry_t`: Represents a directory entry with name, size, and attributes.
- `struct dir_t`: Represents a directory within a volume.

### Functions
- **Disk Operations**:
    - `struct disk_t *disk_open_from_file(const char *volume_file_name)`: Opens a disk image file.
    - `int disk_read(struct disk_t *pdisk, int32_t first_sector, void *buffer, int32_t sectors_to_read)`: Reads data from a disk.
    - `int disk_close(struct disk_t *pdisk)`: Closes a disk.

- **FAT Operations**:
    - `struct volume_t *fat_open(struct disk_t *pdisk, uint32_t first_sector)`: Opens a FAT volume.
    - `int fat_close(struct volume_t *pvolume)`: Closes a FAT volume.

- **File Operations**:
    - `struct file_t *file_open(struct volume_t *pvolume, const char *file_name)`: Opens a file within a volume.
    - `int file_close(struct file_t *stream)`: Closes a file.
    - `size_t file_read(void *ptr, size_t size, size_t nmemb, struct file_t *stream)`: Reads data from a file.
    - `int32_t file_seek(struct file_t *stream, int32_t offset, int whence)`: Sets the file position indicator.

- **Directory Operations**:
    - `struct dir_t *dir_open(struct volume_t *pvolume, const char *dir_path)`: Opens a directory within a volume.
    - `int dir_read(struct dir_t *pdir, struct dir_entry_t *pentry)`: Reads a directory entry.
    - `int dir_close(struct dir_t *pdir)`: Closes a directory.

### Error Handling
Error codes and messages are provided for various failure scenarios, including memory allocation failures, file opening failures, and invalid operations.

### Usage
Users can include the `file_reader.h` header file in their projects and utilize the provided functions to interact with FAT file systems.

