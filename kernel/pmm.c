#include "pmm.h"
#include "vga.h"

/*
 * Maximum memory supported by this simple Stage 3 PMM.
 *
 * 32 MB / 4 KB = 8192 frames
 *
 * 8192 bits = 1024 bytes
 */
#define MAX_MEMORY  (32 * 1024 * 1024)
#define MAX_FRAMES  (MAX_MEMORY / PAGE_SIZE)
#define BITMAP_SIZE (MAX_FRAMES / 8)

/*
 * Bitmap:
 *
 * 0 = free
 * 1 = used
 */
static uint8_t bitmap[BITMAP_SIZE];

static uint32_t total_memory = 0;
static uint32_t total_frames = 0;
static uint32_t used_frames = 0;


/* -----------------------------------------------------------
 * Bitmap helper functions
 * ----------------------------------------------------------- */

static void bitmap_set(uint32_t frame)
{
    bitmap[frame / 8] |=
        (uint8_t)(1 << (frame % 8));
}

static void bitmap_clear(uint32_t frame)
{
    bitmap[frame / 8] &=
        (uint8_t)~(1 << (frame % 8));
}

static int bitmap_test(uint32_t frame)
{
    return bitmap[frame / 8] &
           (uint8_t)(1 << (frame % 8));
}


/* -----------------------------------------------------------
 * PMM Initialisation
 * ----------------------------------------------------------- */

void pmm_init(void)
{
    volatile uint16_t *e820_count;
    e820_entry_t *e820_map;

    uint16_t count;
    uint16_t i;
    uint32_t j;

    /*
     * Clear bitmap first.
     *
     * Initially all frames are considered free.
     */
    for (j = 0; j < BITMAP_SIZE; j++)
    {
        bitmap[j] = 0;
    }

    total_memory = 0;
    total_frames = 0;
    used_frames = 0;

    /*
     * E820 information was stored by the bootloader.
     */
    e820_count =
        (volatile uint16_t *)E820_COUNT_ADDRESS;

    e820_map =
        (e820_entry_t *)E820_MAP_ADDRESS;

    count = *e820_count;

    /*
     * Calculate total usable memory.
     *
     * Only E820 type 1 is usable RAM.
     */
    for (i = 0; i < count; i++)
    {
        if (e820_map[i].type == E820_USABLE)
        {
            uint64_t end;

            end =
                e820_map[i].base +
                e820_map[i].length;

            if (end > total_memory)
            {
                total_memory = (uint32_t)end;
            }
        }
    }

    /*
     * Limit to our bitmap size.
     */
    if (total_memory > MAX_MEMORY)
    {
        total_memory = MAX_MEMORY;
    }

    total_frames =
        total_memory / PAGE_SIZE;

    /*
     * Mark everything as used first.
     *
     * Then only E820 usable regions are
     * marked free.
     */
    for (j = 0; j < total_frames; j++)
    {
        bitmap_set(j);
    }

    used_frames = total_frames;

    /*
     * Mark E820 usable memory as free.
     */
    for (i = 0; i < count; i++)
    {
        uint32_t start_frame;
        uint32_t end_frame;
        uint32_t frame;

        if (e820_map[i].type != E820_USABLE)
        {
            continue;
        }

        start_frame =
            (uint32_t)(e820_map[i].base / PAGE_SIZE);

        end_frame =
            (uint32_t)((e820_map[i].base +
                        e820_map[i].length) /
                       PAGE_SIZE);

        if (start_frame >= total_frames)
        {
            continue;
        }

        if (end_frame > total_frames)
        {
            end_frame = total_frames;
        }

        for (frame = start_frame;
             frame < end_frame;
             frame++)
        {
            if (bitmap_test(frame))
            {
                bitmap_clear(frame);

                if (used_frames > 0)
                {
                    used_frames--;
                }
            }
        }
    }

    /*
     * Reserve the first page.
     *
     * Physical address 0 should not be allocated.
     */
    if (total_frames > 0)
    {
        if (!bitmap_test(0))
        {
            bitmap_set(0);
            used_frames++;
        }
    }

    /*
     * Reserve the memory area used by the bootloader
     * and E820 table.
     *
     * 0x0000 - 0x5FFF
     */
    for (j = 0;
         j < 0x6000 / PAGE_SIZE &&
         j < total_frames;
         j++)
    {
        if (!bitmap_test(j))
        {
            bitmap_set(j);
            used_frames++;
        }
    }

    /*
     * Reserve kernel loading area.
     *
     * Kernel starts at physical address 0x10000.
     *
     * Stage 0 loads up to 64 sectors:
     * 64 * 512 = 32768 bytes.
     *
     * Therefore reserve:
     * 0x10000 - 0x18000
     */
    for (j = 0x10000 / PAGE_SIZE;
         j < 0x18000 / PAGE_SIZE &&
         j < total_frames;
         j++)
    {
        if (!bitmap_test(j))
        {
            bitmap_set(j);
            used_frames++;
        }
    }

    /*
     * Reserve kernel stack at 0x90000.
     */
    for (j = 0x90000 / PAGE_SIZE;
         j < 0x91000 / PAGE_SIZE &&
         j < total_frames;
         j++)
    {
        if (!bitmap_test(j))
        {
            bitmap_set(j);
            used_frames++;
        }
    }
}


/* -----------------------------------------------------------
 * Allocate one physical frame
 * ----------------------------------------------------------- */

uint32_t pmm_alloc_frame(void)
{
    uint32_t frame;

    for (frame = 0;
         frame < total_frames;
         frame++)
    {
        if (!bitmap_test(frame))
        {
            bitmap_set(frame);
            used_frames++;

            return frame * PAGE_SIZE;
        }
    }

    /*
     * No free frame available.
     */
    return 0;
}


/* -----------------------------------------------------------
 * Free one physical frame
 * ----------------------------------------------------------- */

void pmm_free_frame(uint32_t paddr)
{
    uint32_t frame;

    /*
     * Address must be page aligned.
     */
    if (paddr == 0 ||
        (paddr % PAGE_SIZE) != 0)
    {
        return;
    }

    frame = paddr / PAGE_SIZE;

    if (frame >= total_frames)
    {
        return;
    }

    if (bitmap_test(frame))
    {
        bitmap_clear(frame);

        if (used_frames > 0)
        {
            used_frames--;
        }
    }
}


/* -----------------------------------------------------------
 * Memory information
 * ----------------------------------------------------------- */

uint32_t pmm_get_total_memory(void)
{
    return total_memory;
}

uint32_t pmm_get_used_memory(void)
{
    return used_frames * PAGE_SIZE;
}

uint32_t pmm_get_free_memory(void)
{
    return total_memory -
           pmm_get_used_memory();
}


/* -----------------------------------------------------------
 * Stage 3 PMM Test
 *
 * Allocate 100 frames.
 * Free the same 100 frames.
 * Check for memory leak.
 * ----------------------------------------------------------- */

void pmm_test(void)
{
    uint32_t frames[100];

    uint32_t before;
    uint32_t after_alloc;
    uint32_t after_free;

    int i;

    before =
        pmm_get_used_memory();

    /*
     * Allocate 100 frames.
     */
    for (i = 0; i < 100; i++)
    {
        frames[i] =
            pmm_alloc_frame();
    }

    after_alloc =
        pmm_get_used_memory();

    /*
     * Free 100 frames.
     */
    for (i = 0; i < 100; i++)
    {
        if (frames[i] != 0)
        {
            pmm_free_frame(frames[i]);
        }
    }

    after_free =
        pmm_get_used_memory();

    /*
     * Display test result.
     */
    vga_printf(
        "PMM Test: allocated 100 frames\n");

    vga_printf(
        "Used before  : %d KB\n",
        before / 1024);

    vga_printf(
        "Used after   : %d KB\n",
        after_alloc / 1024);

    vga_printf(
        "Used after free : %d KB\n",
        after_free / 1024);

    if (after_free == before)
    {
        vga_printf(
            "PMM Test: PASS - no memory leak\n");
    }
    else
    {
        vga_printf(
            "PMM Test: FAIL - memory leak\n");
    }
}