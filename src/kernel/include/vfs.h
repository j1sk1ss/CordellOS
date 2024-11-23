#ifndef VFS_H_
#define VFS_H_


#include <memory.h>
#include <fslib.h>

#include "ata.h"
#include "fat.h"
#include "allocator.h"


#define FAT_FS      0
#define EXT2_FS     1


typedef struct vfs_node {
    char name[25];
    uint8_t fs_type;
    struct ata_dev* device;

    //===========
    // Functions
    //===========

        // Read content and return char*
        // Content
        char* (*read)(Content*);

        // Read content and return char* (stop reading when meets stop symbols)
        // Content, stop
        void (*read_stop)(Content*, uint8_t*);

        // Read content to buffer with file seek
        // Content, buffer, seek, size
        void (*readoff)(Content*, uint8_t*, uint32_t, uint32_t);

        // Read content to buffer with file seek (stop reading when meets stop symbols)
        // Content, buffer, seek, size, stop
        void (*readoff_stop)(Content*, uint8_t*, uint32_t, uint32_t, uint8_t*);

        // Write data to content (Change FAT table for allocate \ deallocate clusters)
        // Content, data
        int (*write)(Content*, char*);

        // Write data to content with offset (Change FAT table for allocate \ deallocate clusters)
        // Content, buffer, seek, size
        void (*writeoff)(Content*, uint8_t*, uint32_t, uint32_t);

        // Return Directory of current cluster
        Directory* (*dir)(const unsigned int, unsigned char, int);

        // Get Content by path
        // Path
        Content* (*getobj)(const char*);

        // Check if content exists (0 - nexists)
        // Path
        int (*objexist)(const char*);

        // Put content to directory by path
        // Path, content
        int (*putobj)(const char*, Content*);

        // Delete content from directory by path
        // Path, name
        int (*delobj)(const char*);

        // Execute content in specified address space (this function don`t create new page directory)
        // Path, argc, argv, address space
        int (*objexec)(char*, int, char**, int);

        // Change meta of content
        // Path, new meta
        int (*objmetachg)(const char*, directory_entry_t*);

    //===========
    // Functions
    //===========

    struct vfs_node* next;

} vfs_node_t;


extern vfs_node_t* current_vfs;


void VFS_initialize(struct ata_dev* dev, uint32_t fs_type);
void VFS_add_node(struct ata_dev* dev, uint32_t fs_type);
void VFS_switch_device(int index);

#endif