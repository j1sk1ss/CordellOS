#include "../include/syscalls.h"


void i386_syscalls_init() {
    i386_isr_register_handler(0x80, syscall);
}

static uint8_t _syscall_address_space(struct Registers* regs) {
    if ((regs->cs & 0x3) == 0x3) return USER;
    if (taskManager.currentTask >= 0 && taskManager.currentTask < taskManager.tasksCount) {
        Task* task = taskManager.tasks[taskManager.currentTask];
        if (task != NULL) return task->space;
    }

    return KERNEL;
}
 
void syscall(struct Registers* regs) {

    //=======================
    //  PRINT SYSCALLS
    //=======================
        
    switch (regs->eax) {

    //=======================
    //  PRINT SYSCALLS
    //=======================

        case SYS_SCROLL: GFX_scrollback_buffer((int)regs->ebx, GFX_data.physical_base_pointer); break;
        case SYS_GET_KEY_KEYBOARD:
            char* key_buffer = (char*)regs->ecx;
            key_buffer[0] = pop_character();
        break;
        case SYS_AREAD_KEYBOARD: enable_keyboard(); break;

    //=======================
    //  PRINT SYSCALLS
    //=======================
    //  SYSTEM SYSCALLS
    //=======================

        case SYS_WRITE:
            int destination = (int)regs->ebx;
            uint32_t source = (uint32_t)regs->ecx;
            size_t size     = (size_t)regs->edx;
            if (destination == 1) memcpy((void*)GFX_data.virtual_second_buffer, (void*)VMM_virtual2physical((void*)source), size);
            else if (destination == 2) memcpy((void*)GFX_data.physical_base_pointer, (void*)VMM_virtual2physical((void*)source), size);
            else memcpy((void*)destination, (void*)VMM_virtual2physical((void*)source), size);
        break;
        case SYS_TIME:
            _datetime_read_rtc();
            memcpy((datetime_t*)regs->ecx, &DTM_datetime, sizeof(datetime_t));
        break;
        case SYS_GET_TICKS: regs->eax = DTM_get_ticks(); break;

        //=======================
        //  SYSTEM TASKING SYSCALLS
        //=======================

        case SYS_KILL_PROCESS: __kill(); break;
        case SYS_START_PROCESS: START_PROCESS((char*)regs->ebx, (uint32_t)regs->ecx, USER, (uint32_t)regs->edx); break;
        case SYS_GET_PID: regs->eax = taskManager.tasks[taskManager.currentTask]->pid; break;

        //=======================
        //  SYSTEM TASKING SYSCALLS
        //=======================
        //  SYSTEM MEMMANAGER SYSCALLS
        //=======================
        case SYS_PAGE_FREE: {
            void* page_ptr = (void*)regs->ebx;
            if (page_ptr) ALC_freep(page_ptr, _syscall_address_space(regs));
        } break;
        case SYS_MALLOC: {
            regs->eax = (uint32_t)ALC_malloc(regs->ebx, _syscall_address_space(regs));
        } break;
        case SYS_PAGE_MALLOC: {
            uint32_t address = regs->ebx;
            ALC_mallocp(address, _syscall_address_space(regs));
            regs->eax = address;
        } break;
        case SYS_FREE: {
            void* mem_ptr = (void*)regs->ebx;
            if (mem_ptr) ALC_free(mem_ptr, _syscall_address_space(regs));
        } break;

        case SYS_KERN_PANIC: kernel_panic((char*)regs->ecx); break;
            
    //=======================
    //  SYSTEM MEMMANAGER SYSCALLS
    //=======================
    //  FILE SYSTEMS SYSCALLS
    //=======================
        
        case SYS_OPENDIR: regs->eax = current_vfs->lsdir((int)regs->ebx, (char)0, 0); break;
        case SYS_LSDIR:
            int root_ci = (int)regs->ebx;
            int step    = (int)regs->edx;
            char* cname = (char*)regs->ecx;

            int local_step = 0;
            Content* root_node = FAT_get_content_from_table(root_ci);
            
            if (root_node != NULL) {
                Directory* root_dir = root_node->directory;
                if (root_dir->subDirectory != NULL) {
                    Directory* curr_dir = root_dir->subDirectory;
                    while (curr_dir != NULL) {
                        if (local_step == step) {
                            strncpy(cname, curr_dir->name, 11);
                            goto ls_end;
                        }

                        curr_dir = curr_dir->next;
                        local_step++;
                    }
                }

                if (root_dir->files != NULL) {
                    File* curr_file = root_dir->files;
                    while (curr_file != NULL) {
                        if (local_step == step) {
                            sprintf(cname, 11, "%s.%s", curr_file->name, curr_file->extension);
                            goto ls_end;
                        }

                        curr_file = curr_file->next;
                        local_step++;
                    }
                }
            }
ls_end:
            if (local_step == step) regs->eax = step + 1;
            else regs->eax = -1;
        break;
        case SYS_OPEN_CONTENT: regs->eax = current_vfs->openobj((char*)regs->ebx); break;
        case SYS_CONTENT_STAT:
            CInfo_t info;
            current_vfs->objstat(regs->ebx, &info);
            memcpy((CInfo_t*)regs->ecx, &info, sizeof(CInfo_t));
        break;
        case SYS_CLOSE_CONTENT: current_vfs->closeobj(regs->ebx); break;
        case SYS_CEXISTS: regs->eax = current_vfs->objexist((char*)regs->ebx); break;
        case SYS_FCREATE:
            char* mkfile_path = (char*)regs->ebx;
            char* mkfile_name = (char*)regs->ecx;
            char* mkfile_ext = (char*)regs->edx;
            Content* mkfile_content = FAT_create_object(mkfile_name, 0, mkfile_ext);
            current_vfs->putobj(mkfile_path, mkfile_content);
            FAT_unload_content_system(mkfile_content);
        break;
        case SYS_DIRCREATE:
            Content* mkdir_content = FAT_create_object((char*)regs->ecx, 1, "\0");
            current_vfs->putobj((char*)regs->ebx, mkdir_content);
            FAT_unload_content_system(mkdir_content);
        break;
        case SYS_CDELETE: current_vfs->delobj((char*)regs->ebx); break;
        case SYS_CHANGE_META: current_vfs->objmetachg((char*)regs->ebx, (char*)regs->ecx); break;
        case SYS_READ_FILE_OFF: current_vfs->read((int)regs->ebx, (uint8_t*)regs->edx, (int)regs->ecx, (int)regs->esi); break;
        case SYS_READ_FILE_OFF_STP:
            current_vfs->read_stop((int)regs->ebx, (uint8_t*)regs->edx, (int)regs->ecx, (int)regs->esi, (uint8_t*)regs->edi);
        break;
        case SYS_WRITE_FILE_OFF: current_vfs->write((int)regs->ebx, (uint8_t*)regs->edx, (int)regs->ecx, (int)regs->esi); break;
        case SYS_READ_ELF:
            int ci = current_vfs->openobj((char*)regs->ebx);
            
		    regs->eax = (uint32_t)ELF_read(ci, _syscall_address_space(regs));

	        break;

    //=======================
    //  FILE SYSTEMS SYSCALLS
    //=======================
    //  GRAPHICS SYSCALLS
    //=======================
    
        case SYS_VPUT_PIXEL: GFX_vdraw_pixel((uint16_t)regs->ebx, (uint16_t)regs->ecx, (uint32_t)regs->edx); break;
        case SYS_PPUT_PIXEL: GFX_pdraw_pixel((uint16_t)regs->ebx, (uint16_t)regs->ecx, (uint32_t)regs->edx); break;
        case SYS_GET_PIXEL: *((uint32_t*)regs->edx) = GFX_get_pixel((uint16_t)regs->ebx, (uint16_t)regs->ecx); break;
        case SYS_FBUFFER_SWIPE: GFX_swap_buffers(); break;
        case SYS_GET_RESOLUTION_X: *((int*)regs->edx) = GFX_data.x_resolution; break;
        case SYS_GET_RESOLUTION_Y: *((int*)regs->edx) = GFX_data.y_resolution; break;
        
    //=======================
    //  GRAPHICS SYSCALLS
    //=======================
    //  NETWORKING SYSCALLS
    //=======================

        case SYS_SET_IP: IP_set((uint8_t*)regs->ebx); break;
        case SYS_GET_IP: IP_get((uint8_t*)regs->ebx); break;
        case SYS_GET_MAC: get_mac_addr((uint8_t*)regs->ebx); break;
        case SYS_SEND_ETH_PACKET:
            UDP_send_packet(
                (uint8_t*)regs->ebx, 
                (uint16_t)regs->ecx, 
                (uint16_t)regs->edx, 
                (void*)regs->esi, 
                (int)regs->edi
            );
        break;
        case SYS_GET_ETH_PACKETS:
            struct UDPpacket* packet = UDP_pop_packet();
            if (packet == NULL) return;

            memcpy((uint8_t*)regs->ebx, packet->data, packet->data_size);
            free(packet->data);
            free(packet);
        break;
        case SYS_RESTART: i386_reboot(); break;
        case SYS_GET_FS_INFO:
            FSInfo_t* fs_info = (FSInfo_t*)regs->ebx;
            strcpy(fs_info->mount, current_vfs->device->mountpoint);
            strcpy(fs_info->name, current_vfs->name);
            fs_info->type = FAT_data.fat_type;
            fs_info->clusters = FAT_data.total_clusters;
            fs_info->spc = FAT_data.sectors_per_cluster;
            fs_info->size = FAT_data.fat_size;
        break;
    }

    //=======================
    //  NETWORKING SYSCALLS
    //=======================
}
