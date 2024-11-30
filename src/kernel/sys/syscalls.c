#include "../include/syscalls.h"


void i386_syscalls_init() {
    i386_isr_registerHandler(0x80, syscall);
}
 
void syscall(struct Registers* regs) {

    //=======================
    //  PRINT SYSCALLS
    //=======================

        if (regs->eax == SYS_PRINT) {
            const char* data = (const char*)regs->ecx;
            kputs(data);
        } 
        
        else if (regs->eax == SYS_PUTC) {
            char data = (char)regs->ecx;
            kputc(data);
        } 
        
        else if (regs->eax == SYS_COLOR_PUTC) {
            // TODO: cleanup
        } 
        
        else if (regs->eax == SYS_CLEAR) {
            KSTDIO_data.clrscr();
        } 
        
        else if (regs->eax == SYS_SCREEN_COLOR) {
            char screen_color = (char)regs->ecx;
            kset_color(screen_color);
        } 
        
        else if (regs->eax == SYS_GET_SCRCHAR) {
            char* result = (char*)regs->ecx;
            int x = (int)regs->ebx;
            int y = (int)regs->ecx;
            result[0] = KSTDIO_data.get_char(x, y);
        } 

        else if (regs->eax == SYS_GET_CURSOR) {
            int* cursor_cords = (int*)regs->ecx;
            cursor_cords[0] = KSTDIO_data.get_cursor_x();
            cursor_cords[1] = KSTDIO_data.get_cursor_y();

        } 
        
        else if (regs->eax == SYS_SET_CURSOR) {
            int x = (int)regs->ebx;
            int y = (int)regs->ecx;
            KSTDIO_data.set_cursor(x, y);
        } 
        
        else if (regs->eax == SYS_SET_CURSOR32) {
            // TODO: cleanup
        } 

        else if (regs->eax == SYS_SET_SCRCHAR) {
            int x = (int)regs->ebx;
            int y = (int)regs->ecx;
            char new_char = (char)regs->edx;
            KSTDIO_data.put_chr(x, y, new_char);
        } 
        
        else if (regs->eax == SYS_SET_SCRCOLOR) {
            int x = (int)regs->ebx;
            int y = (int)regs->ecx;
            
            uint8_t new_color = (uint8_t)regs->edx;
            VGA_putcolor(x, y, new_color);
        } 

        //=======================
        //  KEYBOARD SYSCALLS
        //=======================
            
            else if (regs->eax == SYS_GET_KEY_KEYBOARD) {
                char* key_buffer = (char*)regs->ecx;
                char key = '\0';

                if (i386_inb(0x64) & 0x1) {
                    key = i386_inb(0x60);
                    key = _get_character(key);
                }

                key_buffer[0] = key;
            } 
            
            else if (regs->eax == SYS_AREAD_KEYBOARD) {
                uint8_t keyborad_color = (uint8_t)regs->edx;
                int keyboard_mode      = (int)regs->ebx;
                uint8_t* buffer        = (uint8_t*)regs->ecx;
                uint8_t stop_list[2]   = { '\n', '\0' };
                _enable_keyboard(buffer, keyboard_mode, keyborad_color, stop_list);
            } 
            
            else if (regs->eax == SYS_AREAD_KEYBOARD_STP) {
                uint8_t keyborad_color = (uint8_t)regs->edx;
                int keyboard_mode      = (int)regs->ebx;
                uint8_t* stop_list        = (uint8_t*)regs->ecx;
                uint8_t* buffer        = (uint8_t*)regs->esi;
                _enable_keyboard(buffer, keyboard_mode, keyborad_color, stop_list);
            } 

        //=======================
        //  KEYBOARD SYSCALLS
        //=======================

    //=======================
    //  PRINT SYSCALLS
    //=======================
    //  SYSTEM SYSCALLS
    //=======================

        else if (regs->eax == SYS_SWITCH_USER) {
            i386_switch2user();
        }

        else if (regs->eax == SYS_TIME) {
            _datetime_read_rtc();
            short* date_buffer = (short*)regs->ecx;
            date_buffer[0] = DTM_datetime.datetime_second;
            date_buffer[1] = DTM_datetime.datetime_minute;
            date_buffer[2] = DTM_datetime.datetime_hour;
            date_buffer[3] = DTM_datetime.datetime_day;
            date_buffer[4] = DTM_datetime.datetime_month;
            date_buffer[5] = DTM_datetime.datetime_year;
        } 

        else if (regs->eax == SYS_GET_TICKS) {
            regs->eax = DTM_get_ticks();
        }

        //=======================
        //  SYSTEM TASKING SYSCALLS
        //=======================

            else if (regs->eax == SYS_KILL_PROCESS) __kill();
            
            else if (regs->eax == SYS_START_PROCESS) {
                char* process_name = (char*)regs->ebx;
                uint32_t address   = (uint32_t)regs->ecx;
                uint32_t delay     = (uint32_t)regs->edx;
                START_PROCESS(process_name, address, USER, delay);
            }

            else if (regs->eax == SYS_GET_PID) {
                regs->eax = taskManager.tasks[taskManager.currentTask]->pid;
            }

        //=======================
        //  SYSTEM TASKING SYSCALLS
        //=======================
        //  SYSTEM MEMMANAGER SYSCALLS
        //=======================

            else if (regs->eax == SYS_PAGE_FREE) {
                void* ptr_to_free = (void*)regs->ebx;
                if (ptr_to_free != NULL)
                    _kfreep(ptr_to_free);
            }

#ifndef USERMODE

            else if (regs->eax == SYS_MALLOC) {
                uint32_t size = regs->ebx;
                void* allocated_memory = _kmalloc(size);
                regs->eax = (uint32_t)allocated_memory;
            } 
            
            else if (regs->eax == SYS_PAGE_MALLOC) {
                uint32_t address = regs->ebx;
                _kmallocp(address);
                regs->eax = address;
            } 

            else if (regs->eax == SYS_FREE) {
                void* ptr_to_free = (void*)regs->ebx;
                if (ptr_to_free != NULL)
                    _kfree(ptr_to_free);
            }

#elif defined(USERMODE)

            else if (regs->eax == SYS_MALLOC) {
                uint32_t size = regs->ebx;
                void* allocated_memory = _umalloc(size);
                regs->eax = (uint32_t)allocated_memory;
            } 
            
            else if (regs->eax == SYS_PAGE_MALLOC) {
                uint32_t address = regs->ebx;
                _umallocp(address);
                regs->eax = address;
            } 

            else if (regs->eax == SYS_FREE) {
                void* ptr_to_free = (void*)regs->ebx;
                if (ptr_to_free != NULL)
                    _ufree(ptr_to_free);
            }

#endif

            else if (regs->eax == SYS_KERN_PANIC) {
                char* message = (char*)regs->ecx;
                kernel_panic(message);
            }

        //=======================
        //  SYSTEM MEMMANAGER SYSCALLS
        //=======================
        //  SYSTEM VARS SYSCALLS
        //=======================

            else if (regs->eax == SYS_ADD_VAR) {
                char* name  = (char*)regs->ebx;
                char* value = (char*)regs->ecx;

                VARS_add(name, value);
            }

            else if (regs->eax == SYS_EXST_VAR) {
                char* name = (char*)regs->ebx;
                regs->eax  = VARS_exist(name);
            }

            else if (regs->eax == SYS_SET_VAR) {
                char* name  = (char*)regs->ebx;
                char* value = (char*)regs->ecx;

                VARS_set(name, value);
            }

            else if (regs->eax == SYS_GET_VAR) {
                char* name = (char*)regs->ebx;
                regs->eax  = (uint32_t)VARS_get(name);
            }

            else if (regs->eax == SYS_DEL_VAR) {
                char* name = (char*)regs->ebx;
                VARS_delete(name);
            }

        //=======================
        //  SYSTEM VARS SYSCALLS
        //=======================

    //=======================
    //  SYSTEM SYSCALLS
    //=======================
    //  FILE SYSTEMS SYSCALLS
    //=======================

        else if (regs->eax == SYS_READ_FILE) {
            char* rfile_path = (char*)regs->ebx;
            Content* content = current_vfs->getobj(rfile_path);
            regs->eax = (uint32_t)current_vfs->read(content);
            FSLIB_unload_content_system(content);
        } 
     
        else if (regs->eax == SYS_READ_FILE_STP) {
            char* rfile_path = (char*)regs->ebx;
            uint8_t* stop    = (uint8_t*)regs->ecx;
            Content* content = current_vfs->getobj(rfile_path);
            regs->eax        = (uint32_t)current_vfs->read_stop(content, stop);
            
            FSLIB_unload_content_system(content);
        } 

        else if (regs->eax == SYS_WRITE_FILE) {
            char* wfile_path = (char*)regs->ebx;
            Content* content = current_vfs->getobj(wfile_path);
            char* data = (char*)regs->ecx;
            current_vfs->write(content, data);
            FSLIB_unload_content_system(content);
        } 
        
        else if (regs->eax == SYS_OPENDIR) {
            char* path = (char*)regs->ebx;
            regs->eax  = (uint32_t)current_vfs->dir(
                GET_CLUSTER_FROM_ENTRY(
                    current_vfs->getobj(path)->directory->directory_meta, FAT_data.fat_type
                ), (char)0, 0
            );
        } 
        
        else if (regs->eax == SYS_GET_CONTENT) {
            char* content_path = (char*)regs->ebx;
            regs->eax = (uint32_t)current_vfs->getobj(content_path);
        } 
        
        else if (regs->eax == SYS_CEXISTS) {
            int* result = (int*)regs->ecx;
            char* path  = (char *)regs->ebx;
            result[0]   = current_vfs->objexist(path);
        } 
        
        else if (regs->eax == SYS_FCREATE) {
            char* mkfile_path = (char*)regs->ebx;
            char* mkfile_name = (char*)regs->ecx;

            char* fname = strtok(mkfile_name, ".");
            char* fexec = strtok(NULL, "."); 

            Content* mkfile_content = FAT_create_content(fname, 0, fexec);
            current_vfs->putobj(mkfile_path, mkfile_content);
            FSLIB_unload_content_system(mkfile_content);
        } 
        
        else if (regs->eax == SYS_DIRCREATE) {
            char* mkdir_path = (char*)regs->ebx;
            char* mkdir_name = (char*)regs->ecx;

            Content* mkdir_content = FAT_create_content(mkdir_name, 1, "\0");
            current_vfs->putobj(mkdir_path, mkdir_content);
            FSLIB_unload_content_system(mkdir_content);
        } 
        
        else if (regs->eax == SYS_CDELETE) {
            char* delete_path = (char*)regs->ebx;
            current_vfs->delobj(delete_path);
        } 
        
        else if (regs->eax == SYS_CHANGE_META) {
            char* meta_path = (char*)regs->ebx;
            directory_entry_t* meta = (directory_entry_t*)regs->ecx;
            current_vfs->objmetachg(meta_path, meta);
        } 

        else if (regs->eax == SYS_READ_FILE_OFF) {
            Content* content = (Content*)regs->ebx;
            int offset       = (int)regs->ecx;
            uint8_t* buffer  = (uint8_t*)regs->edx;
            int offset_len   = (int)regs->esi;
            
            current_vfs->readoff(content, buffer, offset, offset_len);
        }

        else if (regs->eax == SYS_READ_FILE_OFF_STP) {
            Content* content = (Content*)regs->ebx;
            int offset       = (int)regs->ecx;
            uint8_t* buffer  = (uint8_t*)regs->edx;
            int offset_len   = (int)regs->esi;
            uint8_t* stop    = (uint8_t*)regs->edi;
            
            current_vfs->readoff_stop(content, buffer, offset, offset_len, stop);
        }

        else if (regs->eax == SYS_WRITE_FILE_OFF) {
            Content* content = (Content*)regs->ebx;
            int offset       = (int)regs->ecx;
            uint8_t* buffer  = (uint8_t*)regs->edx;
            int offset_len   = (int)regs->esi;
            
            current_vfs->writeoff(content, buffer, offset, offset_len);
        }

        else if (regs->eax == SYS_READ_ELF) {
            char* path = (char*)regs->ebx;

#ifdef USERMODE
		    regs->eax = (uint32_t)ELF_read(path, USER);
#else
		    regs->eax = (uint32_t)ELF_read(path, KERNEL);
#endif

        }

    //=======================
    //  FILE SYSTEMS SYSCALLS
    //=======================
    //  GRAPHICS SYSCALLS
    //=======================
    
        else if (regs->eax == SYS_PUT_PIXEL) {
            uint16_t x      = (uint16_t)regs->ebx;
            uint16_t y      = (uint16_t)regs->ecx;
            uint32_t pixel  = (uint32_t)regs->edx;
            GFX_vdraw_pixel(x, y, pixel);
        } 
        
        else if (regs->eax == SYS_VPUT_PIXEL) {
            uint16_t x      = (uint16_t)regs->ebx;
            uint16_t y      = (uint16_t)regs->ecx;
            uint32_t pixel  = (uint32_t)regs->edx;
            GFX_pdraw_pixel(x, y, pixel);
        } 

        else if (regs->eax == SYS_GET_PIXEL) {
            uint32_t* pixel = (uint32_t*)regs->edx;
            uint16_t x      = (uint16_t)regs->ebx;
            uint16_t y      = (uint16_t)regs->ecx;
            pixel[0] = GFX_get_pixel(x, y);
        } 

        else if (regs->eax == SYS_FBUFFER_SWIPE) {
            GFX_swap_buffers();
        }

        else if (regs->eax == SYS_GET_RESOLUTION_X) {
            int* resolution = (int*)regs->edx;
            resolution[0] = GFX_data.x_resolution;
        }

        else if (regs->eax == SYS_GET_RESOLUTION_Y) {
            int* resolution = (int*)regs->edx;
            resolution[0] = GFX_data.y_resolution;
        }

    //=======================
    //  GRAPHICS SYSCALLS
    //=======================
    //  NETWORKING SYSCALLS
    //=======================

        else if (regs->eax == SYS_SET_IP) {
            uint8_t* ip = (uint8_t*)regs->ebx;
            IP_set(ip);
        }

        else if (regs->eax == SYS_GET_IP) {
            uint8_t* buffer = (uint8_t*)regs->ebx;
            IP_get(buffer);
        }

        else if (regs->eax == SYS_GET_MAC) {
            uint8_t* buffer = (uint8_t*)regs->ebx;
            get_mac_addr(buffer);
        }

        else if (regs->eax == SYS_SEND_ETH_PACKET) {
            uint8_t* dst_ip   = (uint8_t*)regs->ebx;
            uint16_t src_port = (uint16_t)regs->ecx;
            uint16_t dst_port = (uint16_t)regs->edx;
            void* data        = (void*)regs->esi;
            int len           = (int)regs->edi;

            UDP_send_packet(dst_ip, src_port, dst_port, data, len);
        }

        else if (regs->eax == SYS_GET_ETH_PACKETS) {
            uint8_t* data = (uint8_t*)regs->ebx;
            struct UDPpacket* packet = UDP_pop_packet();
            if (packet == NULL) return;

            memcpy(data, packet->data, packet->data_size);

            free(packet->data);
            free(packet);
        }

        else if (regs->eax == SYS_RESTART) {
            i386_reboot();
        }

        else if (regs->eax == SYS_GET_FS_INFO) {
            uint32_t* buffer = (uint32_t*)regs->ebx;

            buffer[0] = (uint32_t)current_vfs->device->mountpoint;
            buffer[1] = (uint32_t)current_vfs->name; // TODO: Copy to user space with malloc
            buffer[2] = (uint32_t)FAT_data.fat_type;
            buffer[3] = (uint32_t)FAT_data.total_clusters;
            buffer[4] = (uint32_t)FAT_data.total_sectors;
            buffer[5] = (uint32_t)FAT_data.bytes_per_sector;
            buffer[6] = (uint32_t)FAT_data.sectors_per_cluster;
            buffer[7] = (uint32_t)FAT_data.fat_size;
        }

    //=======================
    //  NETWORKING SYSCALLS
    //=======================
    //  SYS INFO SYSCALLS
    //=======================

        else if (regs->eax == SYS_MALLOC_MAP) {
            #ifdef USERMODE
                kprint_umalloc();
            #else
                kprint_kmalloc();
            #endif
        }

    //=======================
    //  SYS INFO SYSCALLS
    //=======================
}