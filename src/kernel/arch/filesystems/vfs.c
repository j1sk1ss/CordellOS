#include "../../include/vfs.h"


static vfs_node_t* vfs_list = NULL;
vfs_node_t* current_vfs = NULL;


void VFS_initialize(ata_dev_t* dev, uint32_t fs_type) {
    vfs_list = (vfs_node_t*)_kmalloc(sizeof(vfs_node_t));
    vfs_list->fs_type = fs_type;
    vfs_list->device  = dev;

    if (fs_type == FAT_FS) {
        _fat_vfs_setup(vfs_list);
    } 

    current_vfs = vfs_list;
}

void VFS_add_node(ata_dev_t* dev, uint32_t fs_type) {
    vfs_node_t* new_node = _kmalloc(sizeof(vfs_node_t));
    new_node->fs_type = fs_type;
    new_node->device  = dev;

    if (fs_type == FAT_FS) {
        _fat_vfs_setup(new_node);
    }

    vfs_node_t* cur = vfs_list;
    while (cur->next != NULL) cur = cur->next;
    cur->next = new_node;
}

vfs_node_t* _fat_vfs_setup(vfs_node_t* node) {
    node->read       = FAT_read_content2buffer;
    node->read_stop  = FAT_read_content2buffer_stop;
    node->write      = FAT_write_buffer2content;
    node->lsdir      = FAT_directory_list;
    node->openobj    = FAT_open_content;
    node->objstat    = FAT_stat_content;
    node->closeobj   = FAT_close_content;
    node->objexist   = FAT_content_exists;
    node->putobj     = FAT_put_content;
    node->delobj     = FAT_delete_content;
    node->objexec    = FAT_ELF_execute_content;
    node->objmetachg = FAT_change_meta;
    strncpy(node->name, "FATFS", 5);

    return node;
}

void VFS_switch_device(int index) {
    int pos = 0;
    while (current_vfs->next != NULL) {
        current_vfs = current_vfs->next;
        if (pos++ == index) break;
    }
    
    ATA_device_switch(index);
    if (current_vfs->fs_type == FAT_FS) FAT_initialize();
}