/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stage 0 – Foundations)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   This is the heart of your operating system. Right now it:
 *     1. Initialises VGA text-mode display
 *     2. Initialises the keyboard driver
 *     3. Prints a splash screen
 *     4. Runs a minimal interactive shell ("ksh")
 *
 * ASSIGNMENT MILESTONES  (what YOU will add in later lectures)
 *   Lecture  9  – Process Management  →  process.h / process.c / scheduler.c
 *   Lecture 10  – Threads             →  thread.h  / thread.c
 *   Lecture 11  – Memory Management   →  pmm.h     / pmm.c / vmm.c
 *   Lecture 12  – File System         →  fs.h      / fs.c
 *
 * CODING CONVENTION
 *   - Prefix kernel-internal functions with k_ (e.g. k_strcmp)
 *   - All driver APIs live in their own .h/.c pair
 *   - NEVER call malloc – use the PMM you build in Lecture 11
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"
#include "process.h"
#include "scheduler.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"

#define BUFFER_SIZE 4
#define BUFFER_ITEMS 2

/* -----------------------------------------------------------
 * Stage 2 - Bounded Buffer
 * ----------------------------------------------------------- */

static int buffer[BUFFER_SIZE];
static int buffer_in = 0;
static int buffer_out = 0;

static semaphore_t empty;
static semaphore_t full;
static semaphore_t buffer_mutex;

/* -----------------------------------------------------------
 * Stage 2 - Mutex demonstration
 * ----------------------------------------------------------- */

static volatile int myglobal = 0;
static mutex_t myglobal_mutex;

/* Used for controlled race-condition demonstration */
static int race_read_a = 0;
static int race_read_b = 0;

/* -----------------------------------------------------------
 * Basic thread demonstration
 * ----------------------------------------------------------- */

static void thread_a(void *arg)
{
    (void)arg;

    vga_printf("Thread A running\n");
}

static void thread_b(void *arg)
{
    (void)arg;

    vga_printf("Thread B running\n");
}

/* -----------------------------------------------------------
 * Race condition WITHOUT mutex
 *
 * The current thread implementation runs functions sequentially,
 * so this creates a controlled lost-update demonstration.
 * Both threads read the same value before the updates are applied.
 * ----------------------------------------------------------- */

static void race_thread_a(void *arg)
{
    (void)arg;

    race_read_a = myglobal;

    vga_printf("Thread A reads myglobal = %d\n",
               race_read_a);
}

static void race_thread_b(void *arg)
{
    (void)arg;

    race_read_b = myglobal;

    vga_printf("Thread B reads myglobal = %d\n",
               race_read_b);
}

/* -----------------------------------------------------------
 * Race condition WITH mutex
 * ----------------------------------------------------------- */

static void mutex_thread_a(void *arg)
{
    (void)arg;

    mutex_lock(&myglobal_mutex);

    myglobal++;

    vga_printf("Thread A updates myglobal = %d\n",
               myglobal);

    mutex_unlock(&myglobal_mutex);
}

static void mutex_thread_b(void *arg)
{
    (void)arg;

    mutex_lock(&myglobal_mutex);

    myglobal++;

    vga_printf("Thread B updates myglobal = %d\n",
               myglobal);

    mutex_unlock(&myglobal_mutex);
}

/* -----------------------------------------------------------
 * Bounded Buffer Producer-Consumer
 *
 * Three semaphores:
 *
 * empty        -> number of empty buffer slots
 * full         -> number of filled buffer slots
 * buffer_mutex -> protects the buffer
 * ----------------------------------------------------------- */

static int producer_next_item = 1;

static void producer(void *arg)
{
    (void)arg;

    sem_wait(&empty);

    sem_wait(&buffer_mutex);

    buffer[buffer_in] = producer_next_item;

    vga_printf("Producer: produced %d\n",
               producer_next_item);

    producer_next_item++;

    buffer_in = (buffer_in + 1) % BUFFER_SIZE;

    sem_signal(&buffer_mutex);

    sem_signal(&full);
}

static void consumer(void *arg)
{
    int item;

    (void)arg;

    sem_wait(&full);

    sem_wait(&buffer_mutex);

    item = buffer[buffer_out];

    vga_printf("Consumer: consumed %d\n",
               item);

    buffer_out = (buffer_out + 1) % BUFFER_SIZE;

    sem_signal(&buffer_mutex);

    sem_signal(&empty);
}

/* -----------------------------------------------------------
 * Stage 2 Demonstration
 * ----------------------------------------------------------- */

static void stage2_demo(void)
{
    int thread_a_id;
    int thread_b_id;

    int race_a_id;
    int race_b_id;

    int mutex_a_id;
    int mutex_b_id;

    int producer_id;
    int consumer_id;

    vga_printf("\n");
    vga_printf("========================================\n");
    vga_printf(" Stage 2: Threads, Mutex & Semaphore\n");
    vga_printf("========================================\n");

    /* -------------------------------------------------------
     * 1. Basic Threads
     * ------------------------------------------------------- */

    vga_printf("\n--- Threads ---\n");

    thread_a_id = thread_create(thread_a, 0);
    thread_b_id = thread_create(thread_b, 0);

    thread_run(thread_a_id);
    thread_run(thread_b_id);

    /* -------------------------------------------------------
     * 2. Race condition WITHOUT mutex
     * ------------------------------------------------------- */

    vga_printf("\n--- Race Condition: Without Mutex ---\n");

    myglobal = 0;
    race_read_a = 0;
    race_read_b = 0;

    race_a_id = thread_create(race_thread_a, 0);
    race_b_id = thread_create(race_thread_b, 0);

    /*
     * Both threads read myglobal before either update.
     * This represents the classic lost-update race.
     */

    thread_run(race_a_id);
    thread_run(race_b_id);

    myglobal = race_read_a + 1;

    vga_printf("Expected myglobal = 2\n");
    vga_printf("Actual myglobal   = %d\n",
               myglobal);

    vga_printf("Race condition demonstrated!\n");

    /* -------------------------------------------------------
     * 3. Race condition WITH mutex
     * ------------------------------------------------------- */

    vga_printf("\n--- Race Condition: With Mutex ---\n");

    myglobal = 0;

    mutex_init(&myglobal_mutex);

    mutex_a_id = thread_create(mutex_thread_a, 0);
    mutex_b_id = thread_create(mutex_thread_b, 0);

    thread_run(mutex_a_id);
    thread_run(mutex_b_id);

    vga_printf("Expected myglobal = 2\n");
    vga_printf("Actual myglobal   = %d\n",
               myglobal);

    vga_printf("Mutex protected update successful!\n");

    /* -------------------------------------------------------
     * 4. Bounded Buffer Producer-Consumer
     * ------------------------------------------------------- */

    vga_printf("\n--- Bounded Buffer Producer-Consumer ---\n");

    buffer_in = 0;
    buffer_out = 0;
    producer_next_item = 1;

    /*
     * BUFFER_SIZE empty slots initially.
     */
    sem_init(&empty, BUFFER_SIZE);

    /*
     * No items are available initially.
     */
    sem_init(&full, 0);

    /*
     * Buffer is initially unlocked.
     */
    sem_init(&buffer_mutex, 1);

    /*
     * Create the producer and consumer once.
     * The same thread objects are executed for each item.
     */
    producer_id = thread_create(producer, 0);
    consumer_id = thread_create(consumer, 0);

    for (int i = 0; i < BUFFER_ITEMS; i++)
    {
        thread_run(producer_id);
        thread_run(consumer_id);
    }

    vga_printf("Producer-Consumer completed successfully.\n");

    vga_printf("\n");
}

static void process_a(void)
{
    while (1)
    {
        /* Process A */
    }
}

static void process_b(void)
{
    while (1)
    {
        /* Process B */
    }
}

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_version(void);
static void cmd_color(void);
static void cmd_halt(void);
static void cmd_ps(void);

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b))
    {
        a++;
        b++;
    }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n)
{
    while (n-- && *a && (*a == *b))
    {
        a++;
        b++;
    }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s)
{
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

/* Skip leading spaces */
static const char *k_ltrim(const char *s)
{
    while (*s == ' ')
        s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void)
{
    vga_clear(VGA_BLACK);

    vga_set_cursor(1, 0);

    vga_puts_color("Loading SENG21213-OS...\n",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_puts("\n");

    vga_puts_color("Kernel loaded.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_puts("\n");

    vga_puts_color("Switching to Protected Mode.\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);

    vga_puts("\n");

    vga_puts_color("[ OK ] VGA driver initialised\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_puts_color("[ OK ] PS/2 keyboard driver initialised\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_puts("\n");

    vga_puts("+=================================================+\n\n");
    vga_puts("| SENG 21212 - Stage 0 Kernel Shell              |\n\n");
    vga_puts("| University of Kelaniya                          |\n\n");
    vga_puts("| Type 'help' for available commands              |\n\n");
    vga_puts("+=================================================+\n");

    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void)
{
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  help    – Show this help message\n");
    vga_puts("  clear   – Clear the screen\n");
    vga_puts("  about   – About this OS and course\n");
    vga_puts("  echo    – Echo text to screen\n");
    vga_puts("  mem     – Memory map (stub)\n");
    vga_puts("  ps      - List processes\n");
    vga_puts("  version - Show OS version\n");
    vga_puts("  color   - Test VGA colours\n");
    vga_puts("  halt    - Halt the CPU\n");
    vga_puts_color("\n  Milestones (to implement):\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps      – [L09] List processes\n");
    vga_puts("  kill    – [L09] Terminate a process\n");
    vga_puts("  threads – [L10] List kernel threads\n");
    vga_puts("  free    – [L11] Show free memory\n");
    vga_puts("  ls      – [L12] List files\n");
    vga_puts("  cat     – [L12] Print file contents\n\n");
}

static void cmd_clear(void)
{
    vga_clear(VGA_BLACK);
}

static void cmd_about(void)
{
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args)
{
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void)
{
    /* Stage 0 stub – students implement the real PMM in Lecture 11 */
    vga_puts_color("\n  Memory Map (stub – implement PMM in Lecture 11)\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  0x00000000 – 0x000FFFFF  :  First 1 MB (reserved/BIOS)\n");
    vga_puts("  0x00100000 – 0x00EFFFFF  :  Extended memory (usable ~14 MB)\n");
    vga_puts("  0x00F00000 – 0x00FFFFFF  :  BIOS / ROM area\n");
    vga_puts("  0xB8000    – 0xBFFFF     :  VGA frame buffer\n");
    vga_puts_color("\n  TODO: Use BIOS int 0x15, EAX=0xE820 to get real memory map\n\n",
                   VGA_YELLOW, VGA_BLACK);
}

static void cmd_ps(void)
{
    pcb_t *process;

    vga_puts("\nPID    STATE\n");
    vga_puts("----------------\n");

    process = process_get_list();

    if (process == 0)
    {
        vga_puts("No processes.\n\n");
        return;
    }

    for (uint32_t i = 0; i < MAX_PROCESSES; i++)
    {

        if (process->pid == 0)
        {
            process++;
            continue;
        }

        vga_puts("Process ");

        if (process->pid == 1)
        {
            vga_puts("1");
        }
        else if (process->pid == 2)
        {
            vga_puts("2");
        }
        else if (process->pid == 3)
        {
            vga_puts("3");
        }

        vga_puts("     ");

        if (process->state == PROC_RUNNING)
        {
            vga_puts("RUNNING");
        }
        else if (process->state == PROC_READY)
        {
            vga_puts("READY");
        }
        else
        {
            vga_puts("TERMINATED");
        }

        vga_puts("\n");

        process++;
    }

    vga_puts("\n");
}

static void cmd_version(void)
{
    vga_puts("\nSENG21213-OS Stage 0\n");
    vga_puts("Version: 0.1\n");
    vga_puts("Architecture: x86 i386\n\n");
}

static void cmd_color(void)
{
    vga_puts_color("\nColour test: GREEN\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_puts_color("Colour test: CYAN\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);

    vga_puts_color("Colour test: YELLOW\n",
                   VGA_YELLOW, VGA_BLACK);

    vga_puts_color("Colour test: RED\n\n",
                   VGA_LIGHT_RED, VGA_BLACK);
}

static void cmd_halt(void)
{
    vga_puts_color("\nSystem halted.\n",
                   VGA_YELLOW, VGA_BLACK);

    while (1)
    {
        __asm__ __volatile__("hlt");
    }
}
/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char shell_buf[256];
static char prompt[] = "\nkernel> ";

static void shell_run(void)
{

    while (1)
    {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0)
            continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help") == 0)
        {
            cmd_help();
            continue;
        }
        if (k_strcmp(cmd, "clear") == 0)
        {
            cmd_clear();
            continue;
        }
        if (k_strcmp(cmd, "about") == 0)
        {
            cmd_about();
            continue;
        }
        if (k_strcmp(cmd, "mem") == 0)
        {
            cmd_mem();
            continue;
        }
        if (k_strcmp(cmd, "version") == 0)
        {
            cmd_version();
            continue;
        }
        if (k_strcmp(cmd, "color") == 0)
        {
            cmd_color();
            continue;
        }
        if (k_strcmp(cmd, "halt") == 0)
        {
            cmd_halt();
            continue;
        }
        if (k_strncmp(cmd, "echo ", 5) == 0)
        {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }
        if (k_strcmp(cmd, "version") == 0)
        {
            cmd_version();
            continue;
        }

        if (k_strcmp(cmd, "color") == 0)
        {
            cmd_color();
            continue;
        }

        if (k_strcmp(cmd, "halt") == 0)
        {
            cmd_halt();
            continue;
        }

        if (k_strcmp(cmd, "ps") == 0)
        {
            cmd_ps();
            continue;
        }

        /* Milestone stubs */
        if (k_strcmp(cmd, "ps") == 0 ||
            k_strcmp(cmd, "kill") == 0 ||
            k_strcmp(cmd, "threads") == 0 ||
            k_strcmp(cmd, "free") == 0 ||
            k_strcmp(cmd, "ls") == 0 ||
            k_strcmp(cmd, "cat") == 0)
        {
            vga_puts_color("  [TODO] This command is not yet implemented.\n",
                           VGA_YELLOW, VGA_BLACK);
            vga_puts("  Implement it as part of your lecture assignment.\n");
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Kernel entry point – called from kernel_entry.asm
 * --------------------------------------------------------------------------*/
void kernel_main(void)
{
    vga_init();
    kb_init();
    print_splash();

    process_init();
    scheduler_init();

    process_create(process_a);
    process_create(process_b);

    /*
     * Initialise the Stage 2 thread system
     * before creating any threads.
     */
    thread_init();

    /*
     * Run Stage 2 demonstrations.
     */
    stage2_demo();

    /*
     * Start the interactive shell.
     */
    shell_run();

    /* Should never reach here */
    __asm__ __volatile__("hlt");
}