#include <arch/drivers/hal.h>
#include <filesystems/vfs.h>
#include <filesystems/fat.h>
#include <arch/i386/elf.h>
#include <arch/i386/x86.h>
#include <arch/drivers/pit.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <arch/drivers/pci.h>
#include <network/arp.h>
#include <network/udp.h>
#include <arch/i386/tss.h>
#include <arch/i386/elf.h>
#include <network/dhcp.h>
#include <arch/drivers/mouse.h>
#include <graphics/kstdio.h>
#include <sys/tasking.h>
#include <arch/drivers/speaker.h>
#include <arch/drivers/rtl8139.h>
#include <arch/drivers/keyboard.h>
#include <arch/i386/datetime.h>
#include <sys/syscalls.h>
#include <memory/allocator.h>

#include "multiboot/multiboot.h"

#define CONFIG_KSHELL   0
#define CONFIG_MOUSE    1
#define CONFIG_NETWORK  2
#define CONFIG_SPEAKER  3

#define CONFIG_DISABLED '0'
#define CONFIG_ENABLED  '1'

#define MMAP_LOCATION   0x30000

#define CONFIG_PATH     "BOOT\\BOOT.TXT"
#define SHELL_PATH      "HOME\\APPS\\SHELL\\SHELL.ELF"

#ifdef DEBUG_MODE
    #define NO_MEM_CHECK
    #define FAST_MEM_CHECK
#endif

#pragma region [Default tasks]
#define USERMODE
static int _shell() {
    int shell_ci = current_vfs->openobj(SHELL_PATH);
    if (shell_ci < 0) {
        LOG("SHELL NOT FOUND!");
        return 0;
    }

#ifdef USERMODE
    ELF32_program* program = ELF_read(shell_ci, USER);
    ALC_mallocp(USER_STACK_TOP - USER_STACK_SIZE, USER);
    memset((void*)(USER_STACK_TOP - USER_STACK_SIZE), 0, USER_STACK_SIZE);
    i386_switch2user(program->entry_point, (void*)USER_STACK_TOP);
    ELF_free_program(program, USER);
#else
    current_vfs->objexec(shell_ci, 0, NULL, KERNEL);
#endif

    current_vfs->closeobj(shell_ci);
    return 1;
}

static void _idle() {
    _tick();
}

void kernel_main(struct multiboot_info* mb_info, uint32_t mb_magic, uintptr_t esp) {
    if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        LOG("MB HEADER ERROR (MAGIC IS WRONG [%u]).\n", mb_magic);
        goto _end;
    }

    if (mb_info->vbe_mode == TEXT_MODE) VGA_init((uint8_t*)(uintptr_t)mb_info->framebuffer_addr);
    else {
        GFX_init(mb_info);
        VESA_init();
    }

    ELF_build_symbols_from_multiboot(mb_info->u.elf_sec.addr, mb_info->u.elf_sec.shndx, mb_info->u.elf_sec.num);

    kprintf("\n\t\t =    CORDELL  KERNEL    =");
    kprintf("\n\t\t =     [ ver.   25 ]     =");
    kprintf("\n\t\t =     [ 16.05  26 ]     = \n\n");
    kprintf("\n\t\t =  GENERAL INFORMATION  = \n\n");
    kprintf("\tMB FLAGS:        [0x%p]\n", mb_info->flags);
    uint32_t total_memory = (mb_info->mem_lower + mb_info->mem_upper) * 1024;
    kprintf("\tMMAP:            [0x%p]\t=> MEM SIZE: [%uKB]\n", mb_info->mmap_addr, total_memory / 1024);
    kprintf("\tMEM LOW:         [%uKB]\t=> MEM UP: [%uKB]\n", mb_info->mem_lower, mb_info->mem_upper);
    kprintf("\tBOOT DEVICE:     [0x%p]\n", mb_info->boot_device);
    kprintf("\tVBE MODE:        [%u]\n", mb_info->vbe_mode);

    kprintf("\n\n\t\t =       VBE  INFO       = \n\n");
    kprintf("\tVBE FRAMEBUFFER: [0x%p]\n", mb_info->framebuffer_addr);
    kprintf("\tVBE Y:           [%upx]\n", mb_info->framebuffer_height);
    kprintf("\tVBE X:           [%upx]\n", mb_info->framebuffer_width);
    kprintf("\tVBE BPP:         [%uB]\n", mb_info->framebuffer_bpp);

    PMM_init(MMAP_LOCATION, total_memory);

    if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP) {
        kprintf("\n\n\t\t =     MEMORY   INFO     = \n\n");
        size_t progress = 0;
        multiboot_memory_map_t* mmap_entry = (multiboot_memory_map_t*)mb_info->mmap_addr;
        while ((uint32_t)mmap_entry < mb_info->mmap_addr + mb_info->mmap_length) {
            if (++progress > 1000000) { 
                kprintf("#"); 
                progress = 0;

                int x_cursor = VESA_get_cursor_x();
                int y_cursor = VESA_get_cursor_y();

                VESA_set_cursor(0, VESA_get_max_y() - 2);
                kprintf("MEM ADDR: 0x%p", (uint32_t)mmap_entry);

                VESA_set_cursor(x_cursor, y_cursor);
            }

            if (mmap_entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                kprintf("\n\tREGION |  LEN: [%u]  |  ADDR: [0x%p]  |  TYPE: [%u] \t", mmap_entry->len, mmap_entry->addr, mmap_entry->type);
                PMM_initialize_memory_region(mmap_entry->addr, mmap_entry->len);
                kprintf("MEM REGION REGISTERED!\n");

#ifdef FAST_MEM_CHECK
                break;
#endif
            }

            mmap_entry = (multiboot_memory_map_t*)((uint32_t)mmap_entry + mmap_entry->size + sizeof(mmap_entry->size));
        }

        kprintf("\n");
    }

    uint32_t pmm_bitmap_size = (PMM_map.max_blocks + BLOCKS_PER_BYTE - 1) / BLOCKS_PER_BYTE;
    uint32_t pmm_bitmap_reserved = (pmm_bitmap_size + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);

    PMM_deinitialize_memory_region(0x00000000, 0x00100000);
    PMM_deinitialize_memory_region(MMAP_LOCATION, pmm_bitmap_reserved);
    PMM_deinitialize_memory_region(0x100000, 0x200000);
    if (VMM_init(0x100000) == 0) {
        LOG("VMM INIT ERROR!");
        goto _end;
    }

    uint32_t framebuffer_pages = GFX_data.buffer_size / PAGE_SIZE;
    if (framebuffer_pages % PAGE_SIZE > 0) framebuffer_pages++;

    framebuffer_pages *= 2;
    PMM_deinitialize_memory_region(GFX_data.physical_base_pointer, framebuffer_pages * BLOCK_SIZE);
    for (uint32_t i = 0, fb_start = GFX_data.physical_base_pointer; i < framebuffer_pages; i++, fb_start += PAGE_SIZE) {
        VMM_kmap_page((void*)fb_start, (void*)fb_start);
    }

    GFX_data.virtual_second_buffer = (GFX_data.physical_base_pointer + framebuffer_pages * BLOCK_SIZE) + BLOCK_SIZE;
    PMM_deinitialize_memory_region(GFX_data.virtual_second_buffer, framebuffer_pages * BLOCK_SIZE);
    for (uint32_t i = 0, fb_start = GFX_data.virtual_second_buffer; i < framebuffer_pages; i++, fb_start += PAGE_SIZE) {
        VMM_kmap_page((void*)fb_start, (void*)fb_start);
    }

    HAL_initialize();
    i386_pci_init();
    i386_pit_init();
    i386_syscalls_init();
    i386_task_init();
    i386_init_keyboard();

    kprintf("TASTIERA & MOUSE INIZIALIZZATI [%i]\n\n", i386_detect_ps2_mouse());

    kprintf("ATA INIT...\n");
    if (!ATA_initialize()) {
        LOG("ATA INIT ERROR!");
        goto _end;
    }

    kprintf("FAT INIT...\n");
    if (FAT_initialize() != 0) {
        LOG("FAT INIT ERROR!");
        goto _end;
    }

    kprintf("DRIVER FAT INIZIALIZZATO\nTC:[%uC]\tSPC:[%uS]\tBPS:[%uB]\n\n", FAT_data.total_clusters, FAT_data.sectors_per_cluster, FAT_data.bytes_per_sector);

    uint32_t current_esp;
    asm ("mov %%esp, %0" : "=r"(current_esp));
    TSS_set_stack(0x10, current_esp);

    START_PROCESS("idle", (uint32_t)_idle, KERNEL, 1);

    int shell_addr_space = KERNEL;
    #ifdef USERMODE
        shell_addr_space = USER;
    #endif

    if (!current_vfs->objexist(CONFIG_PATH)) START_PROCESS("shell", (uint32_t)_shell, shell_addr_space, 10); 
    else {
        static uint8_t config[128] = { 0 };
        int boot_ci = current_vfs->openobj(CONFIG_PATH);
        if (boot_ci >= 0) {
            current_vfs->read(boot_ci, config, 0, 5);
            current_vfs->closeobj(boot_ci);
        }

#ifndef DEBUG_MODE
        kclrscr();
        kprintf(" =============== CONFIG STARTUP =============== \n");
        kprintf(" | CONFIG READ BY PATH: [%s]\n", CONFIG_PATH);
        kprintf(" | CONFIG BODY: [%s]\n", config);

        for (int i = 1000000000; i >= 0; i--) {
            if (i % 100000000 == 0) kprintf(" | STARTING AFTER [%is]...\n", i / 100000000);
        }

        kprintf(" ============================================= \n");
#endif

        if (config[CONFIG_SPEAKER] == CONFIG_ENABLED) {
            enable_pc_speaker();

            play_note(A4, 500);
            play_note(B4, 500);
            play_note(C5, 500);
            play_note(D5, 500);
            play_note(C5, 500);
            play_note(B4, 500);
            play_note(A4, 500);

            disable_pc_speaker();
        }

        if (config[CONFIG_NETWORK] == CONFIG_ENABLED) {
            i386_init_rtl8139();
            ARP_init();
            UDP_init();
            DHCP_discover();
        }

        if (config[CONFIG_MOUSE] == CONFIG_ENABLED) i386_init_mouse(1);
        if (config[CONFIG_KSHELL] == CONFIG_ENABLED) START_PROCESS("shell", (uint32_t)_shell, shell_addr_space, 10);
    }

    TASK_start_tasking();
    
_end: {}
    kprintf("\n!!KERNEL END!!\n");
    for (;;);
}
