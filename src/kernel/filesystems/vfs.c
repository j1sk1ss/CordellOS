#include <vfs.h>

#define VFS_MAX_NODES 8

static vfs_node_t _vfs_nodes[VFS_MAX_NODES] = { 0 };
static uint8_t _vfs_node_used[VFS_MAX_NODES] = { 0 };
static vfs_node_t* _vfs_list = NULL;
vfs_node_t* current_vfs = NULL;

static vfs_node_t* _vfs_alloc_node() {
    for (int i = 0; i < VFS_MAX_NODES; i++) {
        if (!_vfs_node_used[i]) {
            _vfs_node_used[i] = 1;
            memset(&_vfs_nodes[i], 0, sizeof(vfs_node_t));
            return &_vfs_nodes[i];
        }
    }

    return NULL;
}

static vfs_node_t* _fat_vfs_setup(vfs_node_t* node) {
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

int VFS_initialize(ata_dev_t* dev, uint32_t fs_type) {
    memset(_vfs_nodes, 0, sizeof(_vfs_nodes));
    memset(_vfs_node_used, 0, sizeof(_vfs_node_used));

    _vfs_list = _vfs_alloc_node();
    if (!_vfs_list) return 0;
    
    _vfs_list->fs_type = fs_type;
    _vfs_list->device  = dev;

    if (fs_type == FAT_FS) {
        _fat_vfs_setup(_vfs_list);
    } 

    current_vfs = _vfs_list;
    return 1;
}

int VFS_add_node(ata_dev_t* dev, uint32_t fs_type) {
    vfs_node_t* new_node = _vfs_alloc_node();
    if (!new_node) return 0;
    
    new_node->fs_type = fs_type;
    new_node->device  = dev;

    if (fs_type == FAT_FS) {
        _fat_vfs_setup(new_node);
    }

    vfs_node_t* cur = _vfs_list;
    while (cur->next != NULL) cur = cur->next;
    cur->next = new_node;
    return 0;
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
