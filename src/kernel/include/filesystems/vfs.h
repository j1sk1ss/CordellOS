#ifndef VFS_H_
#define VFS_H_

#include <memory.h>

#include <arch/drivers/ata.h>
#include <filesystems/fat.h>

#define FAT_FS      0
#define EXT2_FS     1

struct FATContent;
typedef struct vfs_node {
    char             name[25];
    uint8_t          fs_type;
    struct ata_dev*  device;

    // Read content to buffer with file seek
    // content_t, buffer, seek, size
    int              (*read)(int, uint8_t*, uint32_t, uint32_t);

    // Read content to buffer with file seek (stop reading when meets stop symbols)
    // content_t, buffer, seek, size, stop
    int              (*read_stop)(int, uint8_t*, uint32_t, uint32_t, uint8_t*);

    // Write data to content with offset (Change FAT table for allocate \ deallocate clusters)
    // content_t, buffer, seek, size
    int              (*write)(int, const uint8_t*, uint32_t, uint32_t);

    // Return directory_t of current cluster
    int              (*lsdir)(int, uint8_t, int);

    // Get content_t by path
    // Path
    int              (*openobj)(const char*);
    int              (*objstat)(int, CInfo_t*);

    // Close content by ContentIndex (ci)
    int              (*closeobj)(int);

    // Check if content exists (0 - nexists)
    // Path
    int              (*objexist)(const char*);

    // Put content to directory by path
    // Path, content
    int              (*putobj)(const char*, struct FATContent*);

    // Delete content from directory by path
    // Path, name
    int              (*delobj)(const char*);

    // Execute content in specified address space (this function don`t create new page directory)
    // Path, argc, argv, address space
    int              (*objexec)(int, int, char**, int);

    // Change meta of content
    // Path, new meta
    int              (*objmetachg)(const char*, const char*);

    struct vfs_node* next;
} vfs_node_t;

extern vfs_node_t* current_vfs;

int VFS_initialize(struct ata_dev* dev, uint32_t fs_type);
int VFS_add_node(struct ata_dev* dev, uint32_t fs_type);
void VFS_switch_device(int index);

#endif
