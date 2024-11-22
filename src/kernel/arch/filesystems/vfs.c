#include "../../include/vfs.h"


vfs_node_t* vfs_list = NULL;
vfs_node_t* current_vfs = NULL;


void VFS_initialize(ata_dev_t* dev, uint32_t fs_type) {
    vfs_list = _kmalloc(sizeof(vfs_node_t));
    vfs_list->fs_type = fs_type;
    vfs_list->device  = dev;

    if (fs_type == FAT_FS) {
        vfs_list->read         = FAT_read_content;
        vfs_list->read_stop    = FAT_read_content_stop;
        vfs_list->readoff      = FAT_read_content2buffer;
        vfs_list->readoff_stop = FAT_read_content2buffer_stop;

        vfs_list->write      = FAT_write_content;
        vfs_list->writeoff   = FAT_write_buffer2content;

        vfs_list->dir        = FAT_directory_list;
        vfs_list->getobj     = FAT_get_content;
        vfs_list->objexist   = FAT_content_exists;
        vfs_list->putobj     = FAT_put_content;
        vfs_list->delobj     = FAT_delete_content;
        vfs_list->objexec    = FAT_ELF_execute_content;
        vfs_list->objmetachg = FAT_change_meta;

        strncpy(vfs_list->name, "FATFS", 5);
    } 

    current_vfs = vfs_list;
}

void VFS_add_node(ata_dev_t* dev, uint32_t fs_type) {
    vfs_node_t* new_node = _kmalloc(sizeof(vfs_node_t));
    new_node->fs_type = fs_type;
    new_node->device  = dev;

    if (fs_type == FAT_FS) {
        new_node->read         = FAT_read_content;
        new_node->read_stop    = FAT_read_content_stop;
        new_node->readoff      = FAT_read_content2buffer;
        new_node->readoff_stop = FAT_read_content2buffer_stop;

        new_node->write      = FAT_write_content;
        new_node->writeoff   = FAT_write_buffer2content;
        
        new_node->dir        = FAT_directory_list;
        new_node->getobj     = FAT_get_content;
        new_node->objexist   = FAT_content_exists;
        new_node->putobj     = FAT_put_content;
        new_node->delobj     = FAT_delete_content;
        new_node->objexec    = FAT_ELF_execute_content;
        new_node->objmetachg = FAT_change_meta;

        strncpy(new_node->name, "FATFS", 5);
    }

    vfs_node_t* cur = vfs_list;
    while (cur->next != NULL) cur = cur->next;
    cur->next = new_node;
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