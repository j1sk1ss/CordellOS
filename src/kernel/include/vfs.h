#ifndef VFS_H_
#define VFS_H_


#include <memory.h>
#include <fslib.h>

#include "ata.h"
#include "fat.h"
#include "allocator.h"


#define FAT_FS      0
#define EXT2_FS     1


typedef char* (*file_read)(Content*);
typedef char* (*file_read_stop)(Content*, uint8_t*);
typedef void (*file_read_offset)(Content*, uint8_t*, uint32_t, uint32_t);
typedef void (*file_read_offset_stop)(Content*, uint8_t*, uint32_t, uint32_t, uint8_t*);
typedef int (*file_write)(Content*, char*);
typedef void (*file_write_offset)(Content*, uint8_t*, uint32_t, uint32_t);
typedef Directory* (*open_dir)(const unsigned int, unsigned char, int);
typedef Content* (*get_object)(const char*);
typedef int (*object_exists)(const char*);
typedef int (*put_object)(const char*, Content*);
typedef int (*delete_object)(const char*);
typedef int (*object_execute)(char*, int, char**, int);
typedef int (*object_meta_change)(const char*, directory_entry_t*);

typedef struct vfs_node {
    char name[256];
    uint32_t fs_type;
    struct ata_dev* device;

    //===========
    // Functions
    //===========

        // Read content and return char*
        // Content
        file_read read;

        // Read content and return char* (stop reading when meets stop symbols)
        // Content, stop
        file_read_stop read_stop;

        // Read content to buffer with file seek
        // Content, buffer, seek, size
        file_read_offset readoff;

        // Read content to buffer with file seek (stop reading when meets stop symbols)
        // Content, buffer, seek, size, stop
        file_read_offset_stop readoff_stop;

        // Write data to content (Change FAT table for allocate \ deallocate clusters)
        // Content, data
        file_write write;

        // Write data to content with offset (Change FAT table for allocate \ deallocate clusters)
        // Content, buffer, seek, size
        file_write_offset writeoff;

        // Return Directory of current cluster
        open_dir dir;

        // Get Content by path
        // Path
        get_object getobj;

        // Check if content exists (0 - nexists)
        // Path
        object_exists objexist;

        // Put content to directory by path
        // Path, content
        put_object putobj;

        // Delete content from directory by path
        // Path, name
        delete_object delobj;

        // Execute content in specified address space (this function don`t create new page directory)
        // Path, argc, argv, address space
        object_execute objexec;

        // Change meta of content
        // Path, new meta
        object_meta_change objmetachg;

    //===========
    // Functions
    //===========

    struct vfs_node* next;

} vfs_node_t;


extern vfs_node_t* vfs_list;
extern vfs_node_t* current_vfs;


void VFS_initialize(struct ata_dev* dev, uint32_t fs_type);
void VFS_add_node(struct ata_dev* dev, uint32_t fs_type);
void VFS_switch_device(int index);


#endif