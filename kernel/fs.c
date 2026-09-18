#include "fs.h"
#include "ramdisk.h"
#include "vga.h"

/* -----------------------------------------------------------
 * Internal pointers
 * ----------------------------------------------------------- */

static uint8_t *disk;

static superblock_t *superblock;

static uint8_t *block_bitmap;

static uint8_t *inode_bitmap;

static inode_t *inode_table;


/* -----------------------------------------------------------
 * Simple memory helpers
 * ----------------------------------------------------------- */

static void fs_memset(
    void *ptr,
    uint8_t value,
    uint32_t size)
{
    uint8_t *p;
    uint32_t i;

    p = (uint8_t *)ptr;

    for (i = 0; i < size; i++)
    {
        p[i] = value;
    }
}


static void fs_memcpy(
    void *dest,
    const void *src,
    uint32_t size)
{
    uint8_t *d;
    const uint8_t *s;
    uint32_t i;

    d = (uint8_t *)dest;
    s = (const uint8_t *)src;

    for (i = 0; i < size; i++)
    {
        d[i] = s[i];
    }
}


static uint32_t fs_strlen(const char *str)
{
    uint32_t length;

    length = 0;

    while (str[length] != '\0')
    {
        length++;
    }

    return length;
}


static int fs_strcmp(
    const char *a,
    const char *b)
{
    while (*a != '\0' &&
           *a == *b)
    {
        a++;
        b++;
    }

    return (uint8_t)*a - (uint8_t)*b;
}


/* -----------------------------------------------------------
 * Bitmap helpers
 * ----------------------------------------------------------- */

static void block_bitmap_set(uint32_t block)
{
    block_bitmap[block / 8] |=
        (uint8_t)(1 << (block % 8));
}


static void block_bitmap_clear(uint32_t block)
{
    block_bitmap[block / 8] &=
        (uint8_t)~(1 << (block % 8));
}


static int block_bitmap_test(uint32_t block)
{
    return block_bitmap[block / 8] &
           (uint8_t)(1 << (block % 8));
}


static void inode_bitmap_set(uint32_t inode)
{
    inode_bitmap[inode / 8] |=
        (uint8_t)(1 << (inode % 8));
}


static void inode_bitmap_clear(uint32_t inode)
{
    inode_bitmap[inode / 8] &=
        (uint8_t)~(1 << (inode % 8));
}


static int inode_bitmap_test(uint32_t inode)
{
    return inode_bitmap[inode / 8] &
           (uint8_t)(1 << (inode % 8));
}


/* -----------------------------------------------------------
 * Block allocation
 * ----------------------------------------------------------- */

static int allocate_block(void)
{
    uint32_t block;

    for (block = FS_DATA_START_BLOCK;
         block < FS_TOTAL_BLOCKS;
         block++)
    {
        if (!block_bitmap_test(block))
        {
            block_bitmap_set(block);

            return (int)block;
        }
    }

    return -1;
}


static void free_block(uint32_t block)
{
    if (block >= FS_DATA_START_BLOCK &&
        block < FS_TOTAL_BLOCKS)
    {
        block_bitmap_clear(block);
    }
}


/* -----------------------------------------------------------
 * Inode allocation
 * ----------------------------------------------------------- */

static int allocate_inode(void)
{
    uint32_t i;

    for (i = 0; i < FS_INODE_COUNT; i++)
    {
        if (!inode_bitmap_test(i))
        {
            inode_bitmap_set(i);

            inode_table[i].used = 1;
            inode_table[i].size = 0;

            fs_memset(
                inode_table[i].direct,
                0,
                sizeof(inode_table[i].direct));

            fs_memset(
                inode_table[i].name,
                0,
                sizeof(inode_table[i].name));

            return (int)i;
        }
    }

    return -1;
}


static void free_inode(uint32_t inode)
{
    if (inode < FS_INODE_COUNT)
    {
        inode_bitmap_clear(inode);

        inode_table[inode].used = 0;
        inode_table[inode].size = 0;
    }
}


/* -----------------------------------------------------------
 * Find inode by filename
 * ----------------------------------------------------------- */

static int find_inode(const char *name)
{
    uint32_t i;

    for (i = 0; i < FS_INODE_COUNT; i++)
    {
        if (inode_bitmap_test(i))
        {
            if (fs_strcmp(
                    inode_table[i].name,
                    name) == 0)
            {
                return (int)i;
            }
        }
    }

    return -1;
}


/* -----------------------------------------------------------
 * File system initialisation
 * ----------------------------------------------------------- */

void fs_init(void)
{
    uint32_t i;

    disk = ramdisk_data();

    /*
     * Layout:
     *
     * Block 0 -> superblock
     * Block 1 -> block bitmap
     * Block 2 -> inode bitmap
     * Block 3 -> inode table
     * Block 4+ -> file data
     */

    superblock =
        (superblock_t *)
        (disk +
         FS_SUPERBLOCK_BLOCK *
         FS_BLOCK_SIZE);

    block_bitmap =
        disk +
        FS_BLOCK_BITMAP_BLOCK *
        FS_BLOCK_SIZE;

    inode_bitmap =
        disk +
        FS_INODE_BITMAP_BLOCK *
        FS_BLOCK_SIZE;

    inode_table =
        (inode_t *)
        (disk +
         FS_INODE_TABLE_BLOCK *
         FS_BLOCK_SIZE);

    /*
     * Clear entire RAM disk.
     */
    ramdisk_init();

    /*
     * Write superblock.
     */
    superblock->magic = FS_MAGIC;

    superblock->block_size =
        FS_BLOCK_SIZE;

    superblock->block_count =
        FS_TOTAL_BLOCKS;

    superblock->inode_count =
        FS_INODE_COUNT;

    superblock->block_bitmap_block =
        FS_BLOCK_BITMAP_BLOCK;

    superblock->inode_bitmap_block =
        FS_INODE_BITMAP_BLOCK;

    superblock->inode_table_block =
        FS_INODE_TABLE_BLOCK;

    superblock->inode_table_blocks = 1;

    superblock->data_start_block =
        FS_DATA_START_BLOCK;

    /*
     * Clear bitmaps.
     */
    fs_memset(
        block_bitmap,
        0,
        FS_BLOCK_SIZE);

    fs_memset(
        inode_bitmap,
        0,
        FS_BLOCK_SIZE);

    /*
     * Clear inode table.
     */
    fs_memset(
        inode_table,
        0,
        FS_BLOCK_SIZE);

    /*
     * Reserve metadata blocks.
     */
    for (i = 0;
         i < FS_DATA_START_BLOCK;
         i++)
    {
        block_bitmap_set(i);
    }
}


/* -----------------------------------------------------------
 * Open file
 * ----------------------------------------------------------- */

int fs_open(
    const char *name,
    int create)
{
    int inode;
    uint32_t length;

    if (name == 0)
    {
        return -1;
    }

    length = fs_strlen(name);

    if (length == 0 ||
        length > FS_MAX_FILENAME)
    {
        return -1;
    }

    inode = find_inode(name);

    if (inode >= 0)
    {
        return inode;
    }

    if (!create)
    {
        return -1;
    }

    inode = allocate_inode();

    if (inode < 0)
    {
        return -1;
    }

    fs_memcpy(
        inode_table[inode].name,
        name,
        length);

    inode_table[inode].name[length] =
        '\0';

    return inode;
}


/* -----------------------------------------------------------
 * Read file
 * ----------------------------------------------------------- */

int fs_read(
    int fd,
    void *buffer,
    uint32_t size,
    uint32_t offset)
{
    inode_t *inode;
    uint32_t available;
    uint32_t bytes_to_read;
    uint32_t copied;
    uint32_t position;

    if (fd < 0 ||
        fd >= FS_INODE_COUNT ||
        buffer == 0)
    {
        return -1;
    }

    inode = &inode_table[fd];

    if (!inode->used)
    {
        return -1;
    }

    if (offset >= inode->size)
    {
        return 0;
    }

    available =
        inode->size - offset;

    bytes_to_read = size;

    if (bytes_to_read > available)
    {
        bytes_to_read = available;
    }

    copied = 0;

    while (copied < bytes_to_read)
    {
        uint32_t position_in_file;
        uint32_t block_index;
        uint32_t block_offset;
        uint32_t physical_block;
        uint32_t bytes;

        position = offset + copied;

        position_in_file = position;

        block_index =
            position_in_file /
            FS_BLOCK_SIZE;

        block_offset =
            position_in_file %
            FS_BLOCK_SIZE;

        if (block_index >= FS_DIRECT_BLOCKS)
        {
            break;
        }

        physical_block =
            inode->direct[block_index];

        if (physical_block == 0)
        {
            break;
        }

        bytes =
            FS_BLOCK_SIZE -
            block_offset;

        if (bytes >
            bytes_to_read - copied)
        {
            bytes =
                bytes_to_read - copied;
        }

        fs_memcpy(
            (uint8_t *)buffer + copied,
            disk +
            physical_block *
            FS_BLOCK_SIZE +
            block_offset,
            bytes);

        copied += bytes;
    }

    return (int)copied;
}


/* -----------------------------------------------------------
 * Write file
 * ----------------------------------------------------------- */

int fs_write(
    int fd,
    const void *buffer,
    uint32_t size,
    uint32_t offset)
{
    inode_t *inode;

    uint32_t end_position;
    uint32_t required_blocks;
    uint32_t current_blocks;

    uint32_t i;
    uint32_t written;

    if (fd < 0 ||
        fd >= FS_INODE_COUNT ||
        buffer == 0)
    {
        return -1;
    }

    inode = &inode_table[fd];

    if (!inode->used)
    {
        return -1;
    }

    end_position =
        offset + size;

    if (end_position > FS_MAX_FILE_SIZE)
    {
        return -1;
    }

    required_blocks =
        (end_position + FS_BLOCK_SIZE - 1) /
        FS_BLOCK_SIZE;

    current_blocks =
        (inode->size + FS_BLOCK_SIZE - 1) /
        FS_BLOCK_SIZE;

    /*
     * Allocate required blocks.
     */
    for (i = current_blocks;
         i < required_blocks;
         i++)
    {
        int block;

        if (i >= FS_DIRECT_BLOCKS)
        {
            return -1;
        }

        block = allocate_block();

        if (block < 0)
        {
            return -1;
        }

        inode->direct[i] =
            (uint32_t)block;

        fs_memset(
            disk +
            block * FS_BLOCK_SIZE,
            0,
            FS_BLOCK_SIZE);
    }

    /*
     * Write data.
     */
    written = 0;

    while (written < size)
    {
        uint32_t position;
        uint32_t block_index;
        uint32_t block_offset;
        uint32_t physical_block;
        uint32_t bytes;

        position =
            offset + written;

        block_index =
            position / FS_BLOCK_SIZE;

        block_offset =
            position % FS_BLOCK_SIZE;

        physical_block =
            inode->direct[block_index];

        bytes =
            FS_BLOCK_SIZE -
            block_offset;

        if (bytes > size - written)
        {
            bytes =
                size - written;
        }

        fs_memcpy(
            disk +
            physical_block *
            FS_BLOCK_SIZE +
            block_offset,
            (const uint8_t *)buffer + written,
            bytes);

        written += bytes;
    }

    if (end_position > inode->size)
    {
        inode->size =
            end_position;
    }

    return (int)written;
}


/* -----------------------------------------------------------
 * Close file
 * ----------------------------------------------------------- */

void fs_close(int fd)
{
    /*
     * No open-file table is needed for
     * this simple Stage 4 filesystem.
     */
    (void)fd;
}


/* -----------------------------------------------------------
 * Delete file
 * ----------------------------------------------------------- */

int fs_unlink(const char *name)
{
    int inode;
    uint32_t i;

    inode = find_inode(name);

    if (inode < 0)
    {
        return -1;
    }

    /*
     * Free all data blocks.
     */
    for (i = 0;
         i < FS_DIRECT_BLOCKS;
         i++)
    {
        if (inode_table[inode].direct[i] != 0)
        {
            free_block(
                inode_table[inode].direct[i]);

            inode_table[inode].direct[i] = 0;
        }
    }

    /*
     * Free inode.
     */
    free_inode(
        (uint32_t)inode);

    return 0;
}


/* -----------------------------------------------------------
 * Directory listing
 * ----------------------------------------------------------- */

void fs_list(void)
{
    uint32_t i;
    int found;

    found = 0;

    vga_printf("\n");

    vga_printf(
        "NAME                             SIZE\n");

    vga_printf(
        "----------------------------------------\n");

    for (i = 0;
         i < FS_INODE_COUNT;
         i++)
    {
        if (inode_bitmap_test(i))
        {
            vga_printf(
                "%s                             %d bytes\n",
                inode_table[i].name,
                inode_table[i].size);

            found = 1;
        }
    }

    if (!found)
    {
        vga_printf(
            "(empty)\n");
    }

    vga_printf("\n");
}