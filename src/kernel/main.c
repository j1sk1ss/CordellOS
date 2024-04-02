#include <fslib.h>

#include "include/hal.h"
#include "include/vfs.h"
#include "include/fat.h"
#include "include/elf.h"
#include "include/tasking.h"
#include "include/x86.h"
#include "include/pit.h"
#include "include/phys_manager.h"
#include "include/virt_manager.h"
#include "include/mouse.h"
#include "include/keyboard.h"
#include "include/date_time.h"
#include "include/allocator.h"
#include "include/syscalls.h"
#include "include/speaker.h"
#include "include/rtl8139.h"
#include "include/pci.h"
#include "include/arp.h"
#include "include/dhcp.h"
#include "include/udp.h"
#include "include/tss.h"
#include "include/vars.h"
#include "include/elf.h"

#include "multiboot.h"


// #define USERMODE

#define CONFIG_KSHELL   0
#define CONFIG_MOUSE    1
#define CONFIG_NETWORK  2
#define CONFIG_SPEAKER  3

#define CONFIG_ENABLED  '1'
#define CONFIG_DISABLED '0'

#define MMAP_LOCATION   0x30000

#define CONFIG_PATH     "boot\\boot.txt"
#define SHELL_PATH      "home\\shell\\shell.elf"


//======================================================================================================================================
// TODO List:                                                     | | Features:                                                      | |
// ? - WIP                                                        | |                                                                | |
// V - complete                                                   | |                                                                | |
//======================================================================================================================================
//      1) Multiboot struct                                       [V]   1) Physical memory manager                                   | |
//      2) Phys and Virt manages                                  [V]   2) Virtual memory manager (paging)                           | |
//          2.0) Make page and phys prints for debug              [V]   3) Multitasking                                              | |
//      3) ELF check v_addr                                       [V]   4) Syscalls                                                  | |
//          3.1) Fix global and static vars                       [V]       4.0) Std libs for userl programs                         | |
//          3.2) Loading ELF without malloc for fdata             [V]       4.1) Cordell-ASM compiler                                | |
//      4) Paging (create error with current malloc) / allocators [V]   5) Mouse & keyboard support                                  | |
//          4.0) Random Page fault (Null error de Italia)         [V]   6) VBE support                                               | |
//          4.1) Tasking with paging                              [V]   7) ELF support                                               | |
//          4.2) ELF exec with tasking and paging                 [V]   8) BMP support                                               | |
//      5) VBE / VESA                                             [V]   9) FAT32/16/12 support                                       | |
//          5.0) VBE kernel                                       [V]   10) Boot config                                              | |
//              5.0.1) Kshell scrolling                           [ ]   11) Multidisk support                                        | |
//          5.1) Double buffering                                 [ ]   12) Networking                                               | |
//      6) Keyboard to int                                        [V]       12.0) DHCP                                               | |
//      7) Reboot outportb(0x64, 0xFE);                           [V]       12.1) UDP                                                | |
//      8) Mouse support                                          [V]       12.2) ARP                                                | |
//      9) Std lib for graphics                                   [V]       12.3) RLT8139 driver                                     | |
//          8.0.0) Objects                                        [V]   13) Windows                                                  | |
//          8.0.1) Click event                                    [V]   14) Enviroment variables                                     | |
//      10) Mouse to int                                          [V]   15) Hash crypto libs (shell login with pass&login hashing)   | |
//      11) Loading BMP without malloc for fdata                  [V]   16) Speaker driver                                           | |
//      12) Syscalls to std libs                                  [V]
//          12.0) Syscalls for content change                     [V]
//          12.1) Syscalls for content delete                     [V]
//          12.2) Syscalls for kmallocp and freep                 [V]
//      13) VBE userland                                          [?]
//          13.0) VBE file manager                                [?]
//          13.1) VBE text editor                                 [?]
//      14) Malloc optimization                                   [ ]
//          14.0) Free pages when they are unused                 [ ]
//      15) Bugs                                                  [?]
//          15.0) Tasking page fault (In case when we use more    [?]
//                then one task)                                  |||
//          15.1) Mouse page fault                                [?]
//          15.2) Tasking with page allocator                     [V]
//      16) Ethernet                                              [?]
//          16.0) ethernet                                        [V]
//              16.0.0) Send packets                              [V]
//              16.0.1) Receive packets                           [V]
//              16.0.2) DHCP                                      [V]
//              16.0.3) Sending data                              [V]
//                  16.0.3.0) Sending normal data (ndummy)        [V]
//          16.1) IP Udp, Arp                                     [V]
//          16.2) Receive data on Host server                     [V]
//          16.3) STD libs for networking                         [V]
//          16.4) TCP                                             [?]
//          16.5) Host sending to VK/TG etc                       [ ]
//      17) KShell to ELF program                                 [V]
//          17.0) KShell save commands to cash                    [V]
//          17.1) MEM-DATA progs                                  [V]
//      18) User mode switch                                      [?]
//      19) Keyboard in kernel by syscall that cause mt problems  [V]
//      20) Tasking problems (again)                              [V]
//          20.0) Page directory cloning                          [V]
//          20.1) User page directory                             [ ]
//      21) Data append \ modify FAT                              [ ]
//      22) DOOM?                                                 [?]
//======================================================================================================================================


#pragma region [Default tasks]

void shell() {

#ifdef USERMODE
    i386_switch2user();
    fexec(SHELL_PATH, 0, NULL);
#else
    current_vfs->objexec(SHELL_PATH, 0, NULL, KERNEL);
#endif

}

void idle() {
    tick();
}

#pragma endregion


void kernel_main(struct multiboot_info* mb_info, uint32_t mb_magic, uintptr_t esp) {
    
    //===================
    // GFX init
    // MB Information
    // - Video mode data
    // - MM data
    // - RAM data
    // - VBE data
    //===================

#pragma region [Basic kernel info]

        if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
            kprintf("[%s %i] MB HEADER ERROR (MAGIC IS WRONG [%u]).\n", __FILE__, __LINE__, mb_magic);
            goto end;
        }

        if (mb_info->vbe_mode != TEXT_MODE) GFX_init(mb_info);
        else _screenBuffer = (uint8_t*)(uintptr_t)mb_info->framebuffer_addr;

        kprintf("\n\t\t =    CORDELL  KERNEL    =");
        kprintf("\n\t\t =     [ ver.   17 ]     =");
        kprintf("\n\t\t =     [ 25.03  24 ]     = \n\n");
        kprintf("\n\t\t = INFORMAZIONI GENERALI = \n\n");
        kprintf("\tMB FLAGS:        [0x%p]\n", mb_info->flags);
        kprintf("\tMEM LOW:         [%uKB] => MEM UP: [%uKB]\n", mb_info->mem_lower, mb_info->mem_upper);
        kprintf("\tBOOT DEVICE:     [0x%p]\n", mb_info->boot_device);
        kprintf("\tCMD LINE:        [%s]\n", mb_info->cmdline);
        kprintf("\tVBE MODE:        [%u]\n", mb_info->vbe_mode);

        kprintf("\n\n\t\t =       VBE  INFO       = \n\n");
        kprintf("\tVBE FRAMEBUFFER: [0x%p]\n", mb_info->framebuffer_addr);
        kprintf("\tVBE Y:           [%upx]\n", mb_info->framebuffer_height);
        kprintf("\tVBE X:           [%upx]\n", mb_info->framebuffer_width);
        kprintf("\tVBE BPP:         [%uB]\n", mb_info->framebuffer_bpp);

#pragma endregion

    //===================
    // Phys & Virt memory manager initialization
    // - Phys blocks
    // - Virt pages
    //===================
    
#pragma region [Memory info & memory testing]

        uint32_t total_memory = mb_info->mem_upper + (mb_info->mem_lower << 10);
        PMM_init(MMAP_LOCATION, total_memory);

        //===================
        // Memory test
        // - Fill memory region with dummy data
        // - Read dummy data (should be saved and not changed)
        //===================

            if (mb_info->flags & (1 << 1)) {
                kprintf("\n\n\t\t =     MEMORY   INFO     = \n\n");
                multiboot_memory_map_t* mmap_entry = (multiboot_memory_map_t*)mb_info->mmap_addr;
                while ((uint32_t)mmap_entry < mb_info->mmap_addr + mb_info->mmap_length) {
                    if (mmap_entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                        kprintf("\tREGION |  LEN: [%u]  |  ADDR: [0x%p]  |  TYPE: [%u] \t", mmap_entry->len, mmap_entry->addr, mmap_entry->type);
                        const uint32_t pattern = 0xC08DE77;

                        uint32_t* ptr = (uint32_t*)(uintptr_t)mmap_entry->addr;
                        uint32_t* end = (uint32_t*)(uintptr_t)(mmap_entry->addr + mmap_entry->len);
                        while (ptr < end) {
                            *ptr = pattern;
                            ++ptr;
                        }

                        ptr = (uint32_t*)(uintptr_t)mmap_entry->addr;
                        while (ptr < end) {
                            if (*ptr != pattern) {
                                kprintf("MEM TEST FAILED AT [0x%p]\n", ptr);
                                return;
                            }

                            ++ptr;
                        }

                        kprintf("MEM TEST PASSED!\n");
                        initialize_memory_region(mmap_entry->addr, mmap_entry->len);
                    }

                    mmap_entry = (multiboot_memory_map_t*)((uint32_t)mmap_entry + mmap_entry->size + sizeof(mmap_entry->size));
                }

                kprintf("\n\n");
            }

        //===================
        // Memory test
        //===================

        deinitialize_memory_region(0x1000, 0x11000);
        deinitialize_memory_region(MMAP_LOCATION, max_blocks / BLOCKS_PER_BYTE);
        if (VMM_init(0x100000) == false) {
            kprintf("[%s %i] VMM INIT ERROR!\n",__FILE__ ,__LINE__);
            goto end;
        }

#pragma endregion

        //===================
        // Map framebuffer to his original phys address
        //===================

#pragma region  [Video memory info & reserving]

            uint32_t framebuffer_pages = gfx_mode.buffer_size / PAGE_SIZE;
            if (framebuffer_pages % PAGE_SIZE > 0) framebuffer_pages++;
    
            // multiplication 2 for hardware
            framebuffer_pages *= 2;
            for (uint32_t i = 0, fb_start = gfx_mode.physical_base_pointer; i < framebuffer_pages; i++, fb_start += PAGE_SIZE)
                map_page2kernel((void*)fb_start, (void*)fb_start);

            deinitialize_memory_region(gfx_mode.physical_base_pointer, framebuffer_pages * BLOCK_SIZE);
            // gfx_mode.virtual_second_buffer = (uint32_t)kmalloc(gfx_mode.buffer_size);

#pragma endregion

        //===================
        // Map framebuffer to his original phys address
        //===================

    //===================
    // Drivers
    // - Keyboard activation
    // - Mouse activation
    //
    // Tasking init
    // Syscalls init
    // RTL init
    // ARP init
    // HAL
    // - IRQ initialization
    // - GDT initialization
    // - IDT initialization
    // - ISR initialization
    //===================

#pragma region [Drivers initialization]

        HAL_initialize();
        i386_pci_init();
        i386_pit_init();
        i386_syscalls_init();
        i386_task_init();
        i386_init_keyboard();
        i386_init_mouse();

    //===================

    kprintf("TASTIERA & MOUSE INIZIALIZZATI [%i]\n\n", i386_detect_ps2_mouse());

    //===================
    // FAT and FS initialization
    // - Boot sector 
    // - Cluster data
    // - FS Clusters 
    //===================

        ATA_initialize();
        FAT_initialize();

#pragma endregion

    //===================
    
    kprintf("DRIVER FAT INIZIALIZZATO\nTC:[%uC]\tSPC:[%uS]\tBPS:[%uB]\n\n", total_clusters, sectors_per_cluster, bytes_per_sector);

    //===================
    // Kernel2user part
    //===================

#pragma region [Preparations for user land]

        VARS_init(); // Init env vars manager

        uint32_t current_esp;
        asm ("mov %%esp, %0" : "=r"(current_esp));
        TSS_set_stack(0x10, current_esp);

        START_PROCESS("idle", (uint32_t)idle, KERNEL);
        if (current_vfs->objexist(CONFIG_PATH) == 1) {
            Content* boot_config = current_vfs->getobj(CONFIG_PATH);
            char* config = current_vfs->read(boot_config);
            FSLIB_unload_content_system(boot_config);
            
            //===================
            // Speaker test
            //===================

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

            //===================
            // Speaker test
            //===================
            // Network initialization
            //===================

                if (config[CONFIG_NETWORK] == CONFIG_ENABLED) {
                    i386_init_rtl8139();
                    ARP_init();
                    UDP_init();
                    DHCP_discover();
                }

            //===================
            // Network initialization
            //===================

            if (config[CONFIG_MOUSE] == CONFIG_ENABLED) show_mouse = 1;
            if (config[CONFIG_KSHELL] == CONFIG_ENABLED) START_PROCESS("shell", (uint32_t)shell, KERNEL);

            kfree(config);
        } else START_PROCESS("shell", (uint32_t)shell, KERNEL);

        TASK_start_tasking();
    
#pragma endregion

    //===================
    // Kernel2user part
    //===================

end:
    kprintf("\n!!KERNEL END!!\n");
    for (;;);
}
