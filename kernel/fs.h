#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define FS_MAGIC 0x53454E47

#define FS_BLOCK_SIZE 4096
#define FS_TOTAL_BLOCKS 256

#define FS_INODE_COUNT 32

#define FS_SUPERBLOCK_BLOCK 0
#define FS_BLOCK_BITMAP_BLOCK 1
#define FS_INODE_BITMAP_BLOCK 2
#define FS_INODE_TABLE_BLOCK 3
#define FS_DATA_START_BLOCK 4

#define FS_DIRECT_BLOCKS 8

#define FS_MAX_FILENAME 31
#define FS_MAX_FILE_SIZE (FS_DIRECT_BLOCKS * FS_BLOCK_SIZE)

/*
 * Superblock
 */
typedef struct
{
    uint32_t magic;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t inode_count;

    uint32_t block_bitmap_block;
    uint32_t inode_bitmap_block;

    uint32_t inode_table_block;
    uint32_t inode_table_blocks;

    uint32_t data_start_block;
} superblock_t;

/*
 * Inode
 *
 * 72 bytes.
 */
typedef struct
{
    uint32_t used;
    uint32_t size;

    uint32_t direct[FS_DIRECT_BLOCKS];

    char name[FS_MAX_FILENAME + 1];
} inode_t;

/*
 * File system initialisation
 */
void fs_init(void);

/*
 * File operations
 *
 * fs_open:
 *   create = 1 -> create if file does not exist
 *   create = 0 -> open existing file only
 */
int fs_open(const char *name, int create);

int fs_read(
    int fd,
    void *buffer,
    uint32_t size,
    uint32_t offset
);

int fs_write(
    int fd,
    const void *buffer,
    uint32_t size,
    uint32_t offset
);

void fs_close(int fd);

int fs_unlink(const char *name);

/*
 * Directory listing
 */
void fs_list(void);

#endif