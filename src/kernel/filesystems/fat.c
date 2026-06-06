#include <filesystems/fat.h>

fat_data_t FAT_data = {
	.fat_size = 0,
	.fat_type = 0,
	.first_fat_sector = 0,
	.first_data_sector = 0,
	.total_sectors = 0,
	.total_clusters = 0,
	.bytes_per_sector = 0,
	.sectors_per_cluster = 0,
	.ext_root_cluster = 0,
	.cluster_size = 0
};

static content_t _content_storage[CONTENT_TABLE_SIZE] = { 0 };
static file_t _file_storage[CONTENT_TABLE_SIZE] = { 0 };
static directory_t _directory_storage[CONTENT_TABLE_SIZE] = { 0 };
static content_t* _content_table[CONTENT_TABLE_SIZE] = { NULL };

static content_t _scratch_content = { 0 };
static file_t _scratch_file = { 0 };
static directory_t _scratch_directory = { 0 };

static inline int _is_static_content(content_t* content) {
	return content >= _content_storage && content < _content_storage + CONTENT_TABLE_SIZE;
}

static inline int _content_slot(content_t* content) {
	if (!_is_static_content(content)) return -1;
	return content - _content_storage;
}

static inline int _is_valid_fd(int fd) {
	return fd >= 0 && fd < CONTENT_TABLE_SIZE && _content_table[fd] != NULL;
}

static inline void _reset_file(file_t* file) {
	if (!file) return;
	if (file->cluster_table) free(file->cluster_table);
	memset(file, 0, sizeof(file_t));
	file->cached_cluster_index = (uint32_t)-1;
	file->cached_cluster = (uint32_t)-1;
}

static inline void _reset_directory(directory_t* directory) {
	if (!directory) return;
	memset(directory, 0, sizeof(directory_t));
}

static inline void _reset_content(content_t* content) {
	if (!content) return;
	memset(content, 0, sizeof(content_t));
	content->parent_cluster = (uint32_t)-1;
}

static int _allocate_content_slot() {
	for (int i = 0; i < CONTENT_TABLE_SIZE; i++) {
		if (!_content_table[i]) {
			_reset_content(&_content_storage[i]);
			_reset_file(&_file_storage[i]);
			_reset_directory(&_directory_storage[i]);
			_content_table[i] = &_content_storage[i];
			return i;
		}
	}

	return -1;
}

static void _release_content_slot(int fd) {
	if (fd < 0 || fd >= CONTENT_TABLE_SIZE) return;
	_reset_content(&_content_storage[fd]);
	_reset_file(&_file_storage[fd]);
	_reset_directory(&_directory_storage[fd]);
	_content_table[fd] = NULL;
}

typedef struct mbr_partition {
	uint8_t  status;
	uint8_t  first_chs[3];
	uint8_t  type;
	uint8_t  last_chs[3];
	uint32_t first_lba;
	uint32_t sectors;
} __attribute__((packed)) mbr_partition_t;

typedef struct master_boot_record {
	uint8_t boot_code[446];
	mbr_partition_t partitions[4];
	uint16_t signature;
} __attribute__((packed)) master_boot_record_t;

static int _is_fat_partition(uint8_t type) {
	return type == 0x01 || type == 0x04 || type == 0x06 ||
	type == 0x0B || type == 0x0C || type == 0x0E;
}

static int _is_valid_bpb(fat_BS_t* bootstruct, uint8_t* sector) {
	if (sector[510] != 0x55 || sector[511] != 0xAA) return 0;
	if (bootstruct->bytes_per_sector != SECTOR_SIZE) return 0;
	if (bootstruct->sectors_per_cluster == 0) return 0;
	if ((bootstruct->sectors_per_cluster & (bootstruct->sectors_per_cluster - 1)) != 0) return 0;
	if (bootstruct->reserved_sector_count == 0) return 0;
	if (bootstruct->table_count == 0) return 0;
	if (bootstruct->total_sectors_16 == 0 && bootstruct->total_sectors_32 == 0) return 0;
	if (bootstruct->table_size_16 == 0 &&
	((fat_extBS_32_t*)(bootstruct->extended_section))->table_size_32 == 0) return 0;

	return 1;
}

static uint32_t _find_fat_partition_lba(uint8_t* sector) {
	master_boot_record_t* mbr = (master_boot_record_t*)sector;
	if (mbr->signature != 0xAA55) return 0;

	for (int i = 0; i < 4; i++) {
		if (_is_fat_partition(mbr->partitions[i].type) &&
		mbr->partitions[i].first_lba != 0 &&
		mbr->partitions[i].sectors != 0) {
			return mbr->partitions[i].first_lba;
		}
	}

	return 0;
}

int FAT_initialize() {
	uint32_t volume_start_lba = 0;
	uint8_t sector_data[SECTOR_SIZE] = { 0 };
	if (ATA_read_sector(0, sector_data) != 1) {
		LOG("Function FAT_initialize: Error reading the first sector of FAT!\n");
		return -1;
	}

	fat_BS_t* bootstruct = (fat_BS_t*)sector_data;
	if (!_is_valid_bpb(bootstruct, sector_data)) {
		volume_start_lba = _find_fat_partition_lba(sector_data);

		if (volume_start_lba == 0) {
			LOG("Function FAT_initialize: no valid FAT boot sector or FAT partition found.\n");
			return -1;
		}

		if (ATA_read_sector(volume_start_lba, sector_data) != 1) {
			LOG("Function FAT_initialize: Error reading FAT partition boot sector!\n");
			return -1;
		}

		bootstruct = (fat_BS_t*)sector_data;
		if (!_is_valid_bpb(bootstruct, sector_data)) {
			LOG("Function FAT_initialize: FAT partition boot sector has invalid BPB fields.\n");
			return -1;
		}
	}

	FAT_data.total_sectors = (bootstruct->total_sectors_16 == 0) ? bootstruct->total_sectors_32 : bootstruct->total_sectors_16;
	FAT_data.fat_size = (bootstruct->table_size_16 == 0) ? ((fat_extBS_32_t*)(bootstruct->extended_section))->table_size_32 : bootstruct->table_size_16;

	uint32_t root_dir_sectors = ((bootstruct->root_entry_count * 32) + (bootstruct->bytes_per_sector - 1)) / bootstruct->bytes_per_sector;
	uint32_t metadata_sectors = bootstruct->reserved_sector_count + (bootstruct->table_count * FAT_data.fat_size) + root_dir_sectors;
	if (FAT_data.total_sectors <= metadata_sectors) {
		LOG("Function FAT_initialize: FAT metadata is larger than the volume.\n");
		return -1;
	}

	uint32_t data_sectors = FAT_data.total_sectors - metadata_sectors;

	FAT_data.total_clusters = data_sectors / bootstruct->sectors_per_cluster;
	FAT_data.first_data_sector = volume_start_lba + bootstruct->reserved_sector_count + bootstruct->table_count * FAT_data.fat_size + root_dir_sectors;

	if (FAT_data.total_clusters < 4085) FAT_data.fat_type = 12;
	else if (FAT_data.total_clusters < 65525) FAT_data.fat_type = 16;
	else {
		FAT_data.fat_type = 32;
	}

	FAT_data.sectors_per_cluster = bootstruct->sectors_per_cluster;
	FAT_data.bytes_per_sector = bootstruct->bytes_per_sector;
	FAT_data.first_fat_sector = volume_start_lba + bootstruct->reserved_sector_count;
	FAT_data.ext_root_cluster = ((fat_extBS_32_t*)(bootstruct->extended_section))->root_cluster;
	FAT_data.cluster_size = FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster;

	for (int i = 0; i < CONTENT_TABLE_SIZE; i++) {
		_release_content_slot(i);
	}
	_reset_content(&_scratch_content);
	_reset_file(&_scratch_file);
	_reset_directory(&_scratch_directory);

	return 0;
}

static int __read_fat(uint32_t cluster) {
	assert(
	cluster >= 2 && cluster < FAT_data.total_clusters &&
	(FAT_data.fat_type == 32 || FAT_data.fat_type == 16)
	);

	uint32_t fat_offset = cluster * (FAT_data.fat_type == 16 ? 2 : 4);
	uint32_t fat_sector = FAT_data.first_fat_sector + fat_offset / SECTOR_SIZE;
	uint8_t sector_data[SECTOR_SIZE] = { 0 };
	if (ATA_read_sector(fat_sector, sector_data) != 1) {
		kprintf("[%s %i] Function __read_fat: Could not read sector that contains FAT32 table entry needed.\n", __FILE__, __LINE__);
		return -1;
	}

	uint32_t table_value = *(uint32_t*)&sector_data[fat_offset % SECTOR_SIZE];
	if (FAT_data.fat_type == 32) table_value &= 0x0FFFFFFF;

	return table_value;
}

static int __write_fat(uint32_t cluster, uint32_t value) {
	assert(
	cluster >= 2 && cluster < FAT_data.total_clusters &&
	(FAT_data.fat_type == 32 || FAT_data.fat_type == 16)
	);

	uint32_t fat_offset = cluster * (FAT_data.fat_type == 16 ? 2 : 4);
	uint32_t fat_sector = FAT_data.first_fat_sector + fat_offset / SECTOR_SIZE;

	uint8_t sector_data[SECTOR_SIZE] = { 0 };
	if (ATA_read_sector(fat_sector, sector_data) != 1) {
		kprintf("Function __write_fat: Could not read sector that contains FAT32 table entry needed.\n");
		return -1;
	}

	*(uint32_t*)&sector_data[fat_offset % SECTOR_SIZE] = value;
	if (ATA_write_sector(fat_sector, sector_data) != 1) {
		kprintf("Function __write_fat: Could not write new FAT32 cluster number to sector.\n");
		return -1;
	}

	return 0;
}

static int _is_cluster_free(uint32_t cluster) {
	if (cluster == 0) return 1;
	return 0;
}

static int _set_cluster_free(uint32_t cluster) {
	return __write_fat(cluster, 0);
}

static int _is_cluster_end(uint32_t cluster, int fatType) {
	if ((cluster == END_CLUSTER_32 && FAT_data.fat_type == 32) ||
	(cluster == END_CLUSTER_16 && FAT_data.fat_type == 16) ||
	(cluster == END_CLUSTER_12 && FAT_data.fat_type == 12))
	return 1;

	return 0;
}

static int _set_cluster_end(uint32_t cluster, int fatType) {
	if (FAT_data.fat_type == 32) return __write_fat(cluster, END_CLUSTER_32);
	if (FAT_data.fat_type == 16) return __write_fat(cluster, END_CLUSTER_16);
	if (FAT_data.fat_type == 12) return __write_fat(cluster, END_CLUSTER_12);
	return -1;
}

static int _is_cluster_bad(uint32_t cluster, int fatType) {
	if ((cluster == BAD_CLUSTER_32 && FAT_data.fat_type == 32) ||
	(cluster == BAD_CLUSTER_16 && FAT_data.fat_type == 16) ||
	(cluster == BAD_CLUSTER_12 && FAT_data.fat_type == 12))
	return 1;

	return 0;
}

static inline int _set_cluster_bad(uint32_t cluster, int fatType) {
	if (FAT_data.fat_type == 32) return __write_fat(cluster, BAD_CLUSTER_32);
	if (FAT_data.fat_type == 16) return __write_fat(cluster, BAD_CLUSTER_16);
	if (FAT_data.fat_type == 12) return __write_fat(cluster, BAD_CLUSTER_12);
	return -1;
}

static uint32_t last_allocated_cluster = SECTOR_OFFSET;

static uint32_t _cluster_allocate() {
	uint32_t cluster = last_allocated_cluster;
	uint32_t clusterStatus = FREE_CLUSTER_32;

	while (cluster < FAT_data.total_clusters) {
		clusterStatus = __read_fat(cluster);
		if (_is_cluster_free(clusterStatus) == 1) {
			if (_set_cluster_end(cluster, FAT_data.fat_type) == 0) {
				last_allocated_cluster = cluster;
				return cluster;
			}
			else {
				kprintf("Function _cluster_allocate: Error occurred with __write_fat, aborting operations...\n");
				return -1;
			}
		}
		else if (clusterStatus < 0) {
			kprintf("Function _cluster_allocate: Error occurred with __read_fat, aborting operations...\n");
			return -1;
		}

		cluster++;
	}

	last_allocated_cluster = 2;
	return -1;
}

static int _cluster_deallocate(const uint32_t cluster) {
	uint32_t cluster_status = __read_fat(cluster);
	if (_is_cluster_free(cluster_status) == 1) return 0;
	else if (cluster_status < 0) {
		kprintf("Function _cluster_deallocate: Error occurred with __read_fat, aborting operations...\n");
		return -1;
	}

	if (_set_cluster_free(cluster) == 0) return 0;
	else {
		kprintf("Function _cluster_deallocate: Error occurred with __write_fat, aborting operations...\n");
		return -1;
	}
}

static uint32_t _cluster_lba(uint32_t cluster) {
	return (cluster - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
}

static int _cluster_read_range(uint32_t cluster, uint32_t offset, uint8_t* buffer, uint32_t size) {
	assert(cluster >= 2 && cluster < FAT_data.total_clusters);
	if (!buffer || size == 0 || offset >= FAT_data.cluster_size) return -1;

	uint32_t sector_index = offset / SECTOR_SIZE;
	uint32_t sector_offset = offset % SECTOR_SIZE;
	uint32_t data_position = 0;
	uint8_t sector_data[SECTOR_SIZE] = { 0 };

	while (sector_index < FAT_data.sectors_per_cluster && data_position < size) {
		if (ATA_read_sector(_cluster_lba(cluster) + sector_index, sector_data) != 1) return -1;

		uint32_t copy_size = min(size - data_position, SECTOR_SIZE - sector_offset);
		memcpy(buffer + data_position, sector_data + sector_offset, copy_size);
		data_position += copy_size;
		sector_index++;
		sector_offset = 0;
	}

	return 1;
}

static int _cluster_read_range_stop(uint32_t cluster, uint32_t offset, uint8_t* buffer, uint32_t size, uint8_t* stop) {
	assert(cluster >= 2 && cluster < FAT_data.total_clusters);
	if (!buffer || !stop || size == 0 || offset >= FAT_data.cluster_size) return -1;

	uint32_t sector_index = offset / SECTOR_SIZE;
	uint32_t sector_offset = offset % SECTOR_SIZE;
	uint32_t data_position = 0;
	uint8_t sector_data[SECTOR_SIZE] = { 0 };
	uint8_t stop_value = *stop;

	while (sector_index < FAT_data.sectors_per_cluster && data_position < size) {
		if (ATA_read_sector(_cluster_lba(cluster) + sector_index, sector_data) != 1) return -1;

		for (uint32_t i = sector_offset; i < SECTOR_SIZE && data_position < size; i++) {
			buffer[data_position++] = sector_data[i];
			if (sector_data[i] == stop_value) {
				*stop = STOP_SYMBOL;
				return 1;
			}
		}

		sector_index++;
		sector_offset = 0;
	}

	return 1;
}

static int _cluster_writeoff(const uint8_t* data, uint32_t cluster, uint32_t offset, uint32_t size) {
	assert(cluster >= 2 && cluster < FAT_data.total_clusters);
	return ATA_writeoff_sectors(_cluster_lba(cluster), data, FAT_data.sectors_per_cluster, offset, size);
}

static int _copy_cluster2cluster(uint32_t source, uint32_t destination) {
	assert(
	source >= 2 && source < FAT_data.total_clusters &&
	destination >= 2 && destination < FAT_data.total_clusters
	);

	return ATA_copy_sectors2sectors(_cluster_lba(source), FAT_data.sectors_per_cluster, _cluster_lba(destination));
}
static int _add_cluster_to_content(int ci) {
	content_t* content = FAT_get_content_from_table(ci);
	if (!content || content->content_type != CONTENT_TYPE_FILE || !content->file) return -1;

	directory_entry_t content_meta = content->meta;
	uint32_t cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
	while (_is_cluster_end(cluster, FAT_data.fat_type) == 0) {
		assert(_is_cluster_bad(cluster, FAT_data.fat_type) == 0);
		assert(cluster != -1);
		cluster = __read_fat(cluster);
	}

	if (_is_cluster_end(cluster, FAT_data.fat_type) == 1) {
		uint32_t newCluster = _cluster_allocate();
		if (newCluster == (uint32_t)-1) return -3;
		assert(_is_cluster_bad(newCluster, FAT_data.fat_type) == 0);
		assert(__write_fat(cluster, newCluster) == 0);

		content->file->data_size++;
		if (content->file->cluster_table) {
			free(content->file->cluster_table);
			content->file->cluster_table = NULL;
			content->file->cluster_table_size = 0;
		}
		return newCluster;
	}

	return -1;
}

static int _content_cluster_at(content_t* content, uint32_t cluster_index) {
	if (!content || content->content_type != CONTENT_TYPE_FILE || !content->file) return -1;

	if (content->file->cluster_table != NULL
	 && cluster_index < content->file->cluster_table_size) {
		uint32_t cluster = content->file->cluster_table[cluster_index];
		content->file->cached_cluster_index = cluster_index;
		content->file->cached_cluster = cluster;
		return cluster;
	}

	uint32_t start_index = 0;
	uint32_t cluster = content->file->first_cluster;

	if (content->file->cached_cluster_index != (uint32_t)-1
	 && content->file->cached_cluster_index <= cluster_index) {
		start_index = content->file->cached_cluster_index;
		cluster = content->file->cached_cluster;
	}

	for (uint32_t i = start_index; i < cluster_index; i++) {
		cluster = __read_fat(cluster);
		if ((int)cluster < 0 || _is_cluster_end(cluster, FAT_data.fat_type) == 1) return -1;
	}

	content->file->cached_cluster_index = cluster_index;
	content->file->cached_cluster = cluster;
	return cluster;
}

typedef struct directory_cursor {
	uint32_t cluster;
	uint32_t sector;
	uint32_t entry;
	uint32_t index;
	uint8_t sector_data[SECTOR_SIZE];
	directory_entry_t* meta;
} directory_cursor_t;

static int _directory_cursor_load(directory_cursor_t* cursor) {
	if (ATA_read_sector(_cluster_lba(cursor->cluster) + cursor->sector, cursor->sector_data) != 1) return -1;
	cursor->meta = (directory_entry_t*)cursor->sector_data + cursor->entry;
	return 1;
}

static int _directory_cursor_open(directory_cursor_t* cursor, uint32_t cluster) {
	memset(cursor, 0, sizeof(directory_cursor_t));
	cursor->cluster = cluster;
	return _directory_cursor_load(cursor);
}

static int _directory_cursor_write(directory_cursor_t* cursor) {
	return ATA_write_sector(_cluster_lba(cursor->cluster) + cursor->sector, cursor->sector_data);
}

static int _directory_cursor_next(directory_cursor_t* cursor) {
	uint32_t entries_per_sector = SECTOR_SIZE / sizeof(directory_entry_t);
	cursor->entry++;
	cursor->index++;

	if (cursor->entry < entries_per_sector) {
		cursor->meta = (directory_entry_t*)cursor->sector_data + cursor->entry;
		return 1;
	}

	cursor->entry = 0;
	cursor->sector++;
	if (cursor->sector < FAT_data.sectors_per_cluster) return _directory_cursor_load(cursor);

	uint32_t next_cluster = __read_fat(cursor->cluster);
	if (_is_cluster_end(next_cluster, FAT_data.fat_type) == 1) return 0;
	if ((int)next_cluster < 0 || _is_cluster_bad(next_cluster, FAT_data.fat_type) == 1) return -1;

	cursor->cluster = next_cluster;
	cursor->sector = 0;
	return _directory_cursor_load(cursor);
}

int FAT_directory_list(int ci, uint8_t attrs, int exclusive) {
	content_t* source = FAT_get_content_from_table(ci);
	if (!source) return -1;
	if (source->content_type != CONTENT_TYPE_DIRECTORY) return -2;

	uint32_t cluster = GET_CLUSTER_FROM_ENTRY(source->meta, FAT_data.fat_type);
	assert(cluster >= 2 && cluster < FAT_data.total_clusters);

	int fd = _allocate_content_slot();
	if (fd < 0) return -1;

	content_t* content = _content_table[fd];
	content->directory = &_directory_storage[fd];
	content->parent_cluster = 0;
	content->content_type = CONTENT_TYPE_DIRECTORY;
	memcpy(&content->meta, &source->meta, sizeof(directory_entry_t));
	strncpy(content->directory->name, source->directory->name, 11);

	return fd;
}

int FAT_directory_entry_name(int ci, int step, char* name) {
	content_t* content = FAT_get_content_from_table(ci);
	if (!content || !name || content->content_type != CONTENT_TYPE_DIRECTORY) return -1;

	uint32_t cluster = GET_CLUSTER_FROM_ENTRY(content->meta, FAT_data.fat_type);
	directory_cursor_t cursor;
	if (_directory_cursor_open(&cursor, cluster) != 1) return -1;

	int current_step = 0;
	const uint8_t attributes_to_hide = FILE_HIDDEN | FILE_SYSTEM;

	while (1) {
		directory_entry_t* file_metadata = cursor.meta;
		if (file_metadata->file_name[0] == ENTRY_END) break;

		int skip_entry =
		strncmp((char*)file_metadata->file_name, "..", 2) == 0 ||
		strncmp((char*)file_metadata->file_name, ".", 1) == 0 ||
		file_metadata->file_name[0] == ENTRY_FREE ||
		(file_metadata->attributes & FILE_LONG_NAME) == FILE_LONG_NAME ||
		(file_metadata->attributes & attributes_to_hide) != 0;

		if (!skip_entry) {
			if (current_step == step) {
				char raw_name[13] = { 0 };
				strcpy(raw_name, (char*)file_metadata->file_name);

				if ((file_metadata->attributes & FILE_DIRECTORY) == FILE_DIRECTORY) {
					strncpy(name, strtok(raw_name, " "), 11);
				}
				else {
					char* file_name = strtok(raw_name, " ");
					char* extension = strtok(NULL, " ");
					if (extension && strlen(extension) > 0) snprintf(name, 11, "%s.%s", file_name, extension);
					else strncpy(name, file_name, 11);
				}

				return step + 1;
			}

			current_step++;
		}

		int next = _directory_cursor_next(&cursor);
		if (next == 0) break;
		if (next < 0) return -1;
	}

	return -1;
}

static char* _name2fatname(char* input) {
	str2uppercase(input);

	int haveExt = 0;
	char searchName[13] = { 0 };
	uint16_t dotPos = 0;
	uint32_t counter = 0;

	while (counter <= 8) {
		if (input[counter] == '.' || !input[counter]) {
			if (input[counter] == '.') haveExt = 1;
			dotPos = counter;
			counter++;
			break;
		}
		else {
			searchName[counter] = input[counter];
			counter++;
		}
	}

	if (counter > 9) {
		counter = 8;
		dotPos = 8;
	}

	uint16_t extCount = 8;
	while (extCount < 11) {
		if (input[counter] && haveExt == 1) searchName[extCount] = input[counter];
		else searchName[extCount] = ' ';

		counter++;
		extCount++;
	}

	counter = dotPos;
	while (counter < 8) {
		searchName[counter] = ' ';
		counter++;
	}

	strcpy(input, searchName);
	return input;
}

static int _name_check(const char* input) {
	short retVal = 0;
	uint16_t iterator = 0;
	for (iterator = 0; iterator < 11; iterator++) {
		if (input[iterator] < 0x20 && input[iterator] != 0x05) {
			retVal = retVal | BAD_CHARACTER;
		}

		switch (input[iterator]) {
			case 0x2E: {
				if ((retVal & NOT_CONVERTED_YET) == NOT_CONVERTED_YET)
				retVal |= TOO_MANY_DOTS;
				retVal ^= NOT_CONVERTED_YET;
				break;
			}

			case 0x22:
			case 0x2A: case 0x2B:
			case 0x2C: case 0x2F:
			case 0x3A: case 0x3B:
			case 0x3C: case 0x3D:
			case 0x3E: case 0x3F:
			case 0x5B: case 0x5C:
			case 0x5D: case 0x7C:
				retVal = retVal | BAD_CHARACTER;
		}

		if (input[iterator] >= 'a' && input[iterator] <= 'z')
		retVal = retVal | LOWERCASE_ISSUE;
	}

	return retVal;
}

static int _directory_search(const char* filepart, const uint32_t cluster, directory_entry_t* file, uint32_t* entryOffset) {
	assert(cluster >= 2 && cluster < FAT_data.total_clusters);

	char searchName[13] = { 0 };
	strcpy(searchName, filepart);
	if (_name_check(searchName) != 0)
	_name2fatname(searchName);

	directory_cursor_t cursor;
	if (_directory_cursor_open(&cursor, cluster) != 1) {
		kprintf("Function _directory_search: _cluster_read encountered an error. Aborting...\n");
		return -1;
	}

	while (1) {
		directory_entry_t* file_metadata = cursor.meta;
		if (file_metadata->file_name[0] == ENTRY_END) break;

		if (strncmp((char*)file_metadata->file_name, searchName, 11) == 0) {
			if (file != NULL) memcpy(file, file_metadata, sizeof(directory_entry_t));
			if (entryOffset != NULL) *entryOffset = cursor.index;
			return 0;
		}

		int next = _directory_cursor_next(&cursor);
		if (next == 0) break;
		if (next < 0) {
			kprintf("Function _directory_search: directory cursor encountered an error. Aborting...\n");
			return -1;
		}
	}

	return -2;
}

static uint16_t _current_time() {
	DTM_datetime_read_rtc();
	return (DTM_datetime.hour << 11) | (DTM_datetime.minute << 5) | (DTM_datetime.second / 2);
}

static uint16_t _current_date() {
	DTM_datetime_read_rtc();

	uint16_t reversed_data = 0;
	reversed_data |= DTM_datetime.day & 0x1F;
	reversed_data |= (DTM_datetime.month & 0xF) << 5;
	reversed_data |= ((DTM_datetime.year - 1980) & 0x7F) << 9;

	return reversed_data;
}

static int _directory_add(const uint32_t cluster, directory_entry_t* file_to_add) {
	directory_cursor_t cursor;
	if (_directory_cursor_open(&cursor, cluster) != 1) {
		kprintf("Function _directory_add: _cluster_read encountered an error. Aborting...\n");
		return -1;
	}

	while (1) {
		directory_entry_t* file_metadata = cursor.meta;
		if (file_metadata->file_name[0] == ENTRY_FREE || file_metadata->file_name[0] == ENTRY_END) {
			file_to_add->creation_date = _current_date();
			file_to_add->creation_time = _current_time();
			file_to_add->creation_time_tenths = _current_time();
			file_to_add->last_accessed = file_to_add->creation_date;
			file_to_add->last_modification_date = file_to_add->creation_date;
			file_to_add->last_modification_time = file_to_add->creation_time;

			uint32_t new_cluster = _cluster_allocate();
			if (_is_cluster_bad(new_cluster, FAT_data.fat_type) == 1) {
				kprintf("Function _directory_add: allocation of new cluster failed. Aborting...\n");

				return -1;
			}

			file_to_add->low_bits  = GET_ENTRY_LOW_BITS(new_cluster, FAT_data.fat_type);
			file_to_add->high_bits = GET_ENTRY_HIGH_BITS(new_cluster, FAT_data.fat_type);

			memcpy(file_metadata, file_to_add, sizeof(directory_entry_t));
			if (_directory_cursor_write(&cursor) != 1) {
				kprintf("Function _directory_add: Writing new directory entry failed. Aborting...\n");
				return -1;
			}

			return 0;
		}

		int next = _directory_cursor_next(&cursor);
		if (next < 0) {
			kprintf("Function _directory_add: directory cursor encountered an error. Aborting...\n");
			return -1;
		}

		if (next == 0) {
			uint32_t next_cluster = _cluster_allocate();
			if (next_cluster == (uint32_t)-1 || _is_cluster_bad(next_cluster, FAT_data.fat_type) == 1) {
				kprintf("Function _directory_add: allocation of new cluster failed. Aborting...\n");
				return -1;
			}

			if (__write_fat(cursor.cluster, next_cluster) != 0) {
				kprintf("Function _directory_add: extension of the cluster chain with new cluster failed. Aborting...\n");
				return -1;
			}

			if (_directory_cursor_open(&cursor, next_cluster) != 1) return -1;
		}
	}

	return -1;
}

static int _directory_edit(const uint32_t cluster, directory_entry_t* old_meta, const char* new_name) {
	if (_name_check((char*)old_meta->file_name) != 0) {
		kprintf("Function _directory_edit: Invalid file name!");
		return -1;
	}

	directory_cursor_t cursor;
	if (_directory_cursor_open(&cursor, cluster) != 1) {
		kprintf("Function _directory_edit: _cluster_read encountered an error. Aborting...\n");
		return -1;
	}

	while (1) {
		directory_entry_t* file_metadata = cursor.meta;
		if (strncmp((char*)file_metadata->file_name, (char*)old_meta->file_name, 11) == 0) {
			old_meta->last_accessed = _current_date();
			old_meta->last_modification_date = _current_date();
			old_meta->last_modification_time = _current_time();

			memset(old_meta->file_name, 0, 11);
			strncpy((char*)old_meta->file_name, new_name, 11);
			memcpy(file_metadata, old_meta, sizeof(directory_entry_t));

			if (_directory_cursor_write(&cursor) != 1) {
				kprintf("Function _directory_edit: Writing updated directory entry failed. Aborting...\n");
				return -1;
			}

			return 0;
		}

		int next = _directory_cursor_next(&cursor);
		if (next == 0) return -2;
		if (next < 0) {
			kprintf("Function _directory_edit: directory cursor encountered an error. Aborting...\n");
			return -1;
		}
	}

	return -1;
}

static int _directory_remove(const uint32_t cluster, const char* fileName) {
	if (_name_check(fileName) != 0) {
		kprintf("Function _directory_remove: Invalid file name!");
		return -1;
	}

	directory_cursor_t cursor;
	if (_directory_cursor_open(&cursor, cluster) != 1) {
		kprintf("Function _directory_remove: _cluster_read encountered an error. Aborting...\n");
		return -1;
	}

	while (1) {
		directory_entry_t* file_metadata = cursor.meta;
		if (strncmp((char*)file_metadata->file_name, fileName, 11) == 0) {
			file_metadata->file_name[0] = ENTRY_FREE;
			if (_directory_cursor_write(&cursor) != 1) {
				kprintf("Function _directory_remove: Writing updated directory entry failed. Aborting...\n");
				return -1;
			}

			return 0;
		}

		int next = _directory_cursor_next(&cursor);
		if (next == 0) return -2;
		if (next < 0) {
			kprintf("Function _directory_remove: directory cursor encountered an error. Aborting...\n");
			return -1;
		}
	}

	return -1;
}

int FAT_content_exists(const char* path) {
	char fileNamePart[256] = { 0 };
	uint16_t start = 0;
	uint32_t active_cluster = 0;

	if (FAT_data.fat_type == 32) active_cluster = FAT_data.ext_root_cluster;
	else {
		kprintf("Function FAT_content_exists: FAT16 and FAT12 are not supported!\n");
		return -1;
	}

	directory_entry_t file_info;
	for (uint32_t iterator = 0; iterator <= strlen(path); iterator++) {
		if (path[iterator] == '\\' || path[iterator] == '\0') {
			memset(fileNamePart, '\0', 256);
			memcpy(fileNamePart, path + start, iterator - start);

			int result = _directory_search(fileNamePart, active_cluster, &file_info, NULL);
			if (result != 0) return 0;

			start = iterator + 1;
			active_cluster = GET_CLUSTER_FROM_ENTRY(file_info, FAT_data.fat_type);
		}
	}

	return 1;
}

int FAT_open_content(const char* path) {
	int fd = _allocate_content_slot();
	if (fd < 0) return -1;

	content_t* fat_content = _content_table[fd];

	char fileNamePart[256] = { 0 };
	uint16_t start = 0;
	uint32_t active_cluster = 0;

	if (FAT_data.fat_type == 32) active_cluster = FAT_data.ext_root_cluster;
	else {
		LOG("Function FAT_open_content: FAT16 and FAT12 are not supported!\n");
		_release_content_slot(fd);
		return -2;
	}

	directory_entry_t content_meta;
	for (uint32_t iterator = 0; iterator <= strlen(path); iterator++) {
		if (path[iterator] == '\\' || path[iterator] == '\0') {
			memset(fileNamePart, '\0', 256);
			memcpy(fileNamePart, path + start, iterator - start);

			int result = _directory_search(fileNamePart, active_cluster, &content_meta, NULL);
			if (result == -2) {
				_release_content_slot(fd);
				return -3;
			}
			else if (result == -1) {
				LOG("Function FAT_open_content: An error occurred in _directory_search. Aborting...\n");
				_release_content_slot(fd);
				return -4;
			}

			start = iterator + 1;
			active_cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
			if (path[iterator] != '\0') fat_content->parent_cluster = active_cluster;
		}
	}

	memcpy(&fat_content->meta, &content_meta, sizeof(directory_entry_t));
	if ((content_meta.attributes & FILE_DIRECTORY) != FILE_DIRECTORY) {
		fat_content->file = &_file_storage[fd];

		fat_content->content_type = CONTENT_TYPE_FILE;
		int content_size = 0;
		int cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
		fat_content->file->first_cluster = cluster;
		fat_content->file->cached_cluster_index = 0;
		fat_content->file->cached_cluster = cluster;

		while (cluster < END_CLUSTER_32) {
			content_size++;

			cluster = __read_fat(cluster);
			if (cluster == BAD_CLUSTER_32) {
				LOG("Function FAT_open_content: the cluster chain is corrupted with a bad cluster. Aborting...\n");
				_release_content_slot(fd);
				return -7;
			}
			else if (cluster == -1) {
				LOG("Function FAT_open_content: an error occurred in __read_fat. Aborting...\n");
				_release_content_slot(fd);
				return -8;
			}
		}

		fat_content->file->data_size = content_size;
		fat_content->file->cluster_table_size = content_size;
		fat_content->file->cluster_table = NULL;

		if (content_size > 0) {
			fat_content->file->cluster_table = malloc(sizeof(uint32_t) * content_size);
			if (fat_content->file->cluster_table != NULL) {
				uint32_t table_cluster = fat_content->file->first_cluster;
				for (int i = 0; i < content_size; i++) {
					fat_content->file->cluster_table[i] = table_cluster;
					table_cluster = __read_fat(table_cluster);
				}
			}
		}

		char name[13] = { 0 };
		strcpy(name, (char*)fat_content->meta.file_name);
		strncpy(fat_content->file->name, strtok(name, " "), 8);
		strncpy(fat_content->file->extension, strtok(NULL, " "), 4);
	}
	else {
		fat_content->directory = &_directory_storage[fd];

		fat_content->content_type = CONTENT_TYPE_DIRECTORY;
		strncpy(fat_content->directory->name, (char*)content_meta.file_name, 10);
	}

	return fd;
}

content_t* FAT_get_content_from_table(int ci) {
	if (!_is_valid_fd(ci)) return NULL;
	return _content_table[ci];
}

static int _remove_content_from_table(int index) {
	if (index < 0 || index >= CONTENT_TABLE_SIZE) return -1;
	if (!_content_table[index]) return -1;
	_release_content_slot(index);
	return 1;
}

int FAT_close_content(int ci) {
	return _remove_content_from_table(ci);
}

int FAT_read_content2buffer(int ci, uint8_t* buffer, uint32_t offset, uint32_t size) {
	uint32_t data_seek     = offset % (FAT_data.sectors_per_cluster * SECTOR_SIZE);
	uint32_t cluster_seek  = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
	uint32_t data_position = 0;

	content_t* data = FAT_get_content_from_table(ci);
	if (!data) return -1;
	if (data->content_type != CONTENT_TYPE_FILE || !data->file) return -2;

	int cluster = _content_cluster_at(data, cluster_seek);
	for (uint32_t i = cluster_seek; i < (uint32_t)data->file->data_size && data_position < size; i++) {
		if (cluster < 0) return -1;

		data->file->cached_cluster_index = i;
		data->file->cached_cluster = cluster;
		uint32_t copy_size = min(FAT_data.cluster_size - data_seek, size - data_position);
		if (_cluster_read_range(cluster, data_seek, buffer + data_position, copy_size) != 1) return -1;
		data_position += copy_size;
		data_seek = 0;
		cluster = __read_fat(cluster);
	}

	return data_position;
}
int FAT_read_content2buffer_stop(int ci, uint8_t* buffer, uint32_t offset, uint32_t size, uint8_t* stop) {
	uint32_t data_seek     = offset % (FAT_data.sectors_per_cluster * SECTOR_SIZE);
	uint32_t cluster_seek  = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
	uint32_t data_position = 0;

	content_t* data = FAT_get_content_from_table(ci);
	if (data == NULL) return -1;
	if (data->content_type != CONTENT_TYPE_FILE || !data->file) return -2;

	int cluster = _content_cluster_at(data, cluster_seek);
	for (uint32_t i = cluster_seek; i < (uint32_t)data->file->data_size && data_position < size; i++) {
		if (cluster < 0) return -1;

		data->file->cached_cluster_index = i;
		data->file->cached_cluster = cluster;
		uint32_t copy_size = min(FAT_data.cluster_size - data_seek, size - data_position);
		if (_cluster_read_range_stop(cluster, data_seek, buffer + data_position, copy_size, stop) != 1) return -1;
		data_position += copy_size;
		data_seek = 0;

		if (stop[0] == STOP_SYMBOL) break;
		cluster = __read_fat(cluster);
	}

	return data_position;
}

int FAT_ELF_execute_content(int ci, int argc, char* argv[], int type) {
	elf32_program_t* program = ELF_read(ci, type);
	if (!program) return -255;

	int (*programEntry)(int, char* argv[]) = (int (*)(int, char* argv[]))(program->entry_point);
	if (!programEntry) return -255;

	int result_code = programEntry(argc, argv);
	ELF_free_program(program, type);

	return result_code;
}
int FAT_write_buffer2content(int ci, const uint8_t* buffer, uint32_t offset, uint32_t size) {
	content_t* data = FAT_get_content_from_table(ci);
	if (data == NULL) return -1;
	if (data->file == NULL) return -2;

	uint32_t cluster_seek = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
	uint32_t data_position = 0;
	uint32_t cluster_position = 0;
	uint32_t prev_offset = offset;
	int cluster = _content_cluster_at(data, cluster_seek);
	for (cluster_position = cluster_seek; cluster_position < data->file->data_size && data_position < size; cluster_position++) {
		if (cluster < 0) return -1;

		data->file->cached_cluster_index = cluster_position;
		data->file->cached_cluster = cluster;
		uint32_t write_size = min(size - data_position, FAT_data.cluster_size - (offset % FAT_data.cluster_size));
		_cluster_writeoff(buffer + data_position, cluster, offset, write_size);

		offset = 0;
		data_position += write_size;
		cluster = __read_fat(cluster);
	}
	if (data_position < size) {
		uint32_t new_offset = prev_offset + data_position;
		uint32_t new_size   = size - data_position;
		const uint8_t* new_buffer = buffer + data_position;
		int add_result = _add_cluster_to_content(ci);
		if (add_result < 0) return add_result;
		return FAT_write_buffer2content(ci, new_buffer, new_offset, new_size);
	}

	return 1;
}
int FAT_change_meta(const char* path, const char* new_name) {

	char fileNamePart[256] = { 0 };
	uint16_t start = 0;
	uint32_t active_cluster = 0;
	uint32_t prev_active_cluster = 0;

	if (FAT_data.fat_type == 32) active_cluster = FAT_data.ext_root_cluster;
	else {
		kprintf("Function FAT_change_meta: FAT16 and FAT12 are not supported!\n");
		return -1;
	}

	directory_entry_t file_info;
	if (strlen(path) == 0) {
		if (FAT_data.fat_type == 32) {
			active_cluster 		 = FAT_data.ext_root_cluster;
			file_info.attributes = FILE_DIRECTORY | FILE_VOLUME_ID;
			file_info.file_size  = 0;
			file_info.high_bits  = GET_ENTRY_HIGH_BITS(active_cluster, FAT_data.fat_type);
			file_info.low_bits 	 = GET_ENTRY_LOW_BITS(active_cluster, FAT_data.fat_type);
		}
		else {
			kprintf("Function FAT_change_meta: FAT16 and FAT12 are not supported!\n");
			return -1;
		}
	}
	else {
		for (uint32_t iterator = 0; iterator <= strlen(path); iterator++)
		if (path[iterator] == '\\' || path[iterator] == '\0') {
			prev_active_cluster = active_cluster;

			memset(fileNamePart, '\0', 256);
			memcpy(fileNamePart, path + start, iterator - start);

			int retVal = _directory_search(fileNamePart, active_cluster, &file_info, NULL);
			switch (retVal) {
				case -2:
				kprintf("Function FAT_change_meta: No matching directory found. Aborting...\n");
				return -2;

				case -1:
				kprintf("Function FAT_change_meta: An error occurred in _directory_search. Aborting...\n");
				return retVal;
			}

			start = iterator + 1;
			active_cluster = GET_CLUSTER_FROM_ENTRY(file_info, FAT_data.fat_type);
		}
	}

	if (_directory_edit(prev_active_cluster, &file_info, new_name) != 0) {
		kprintf("Function FAT_change_meta: _directory_edit encountered an error. Aborting...\n");
		return -1;
	}

	return 0;
}

static void _fatname2name(char* input, char* output) {
	if (input[0] == '.') {
		if (input[1] == '.') {
			strcpy (output, "..");
			return;
		}

		strcpy (output, ".");
		return;
	}

	uint16_t counter = 0;
	for ( counter = 0; counter < 8; counter++) {
		if (input[counter] == 0x20) {
			output[counter] = '.';
			break;
		}

		output[counter] = input[counter];
	}

	if (counter == 8)
	output[counter] = '.';

	uint16_t counter2 = 8;
	for (counter2 = 8; counter2 < 11; counter2++) {
		++counter;
		if (input[counter2] == 0x20 || input[counter2] == 0x20) {
			if (counter2 == 8)
			counter -= 2;

			break;
		}

		output[counter] = input[counter2];
	}

	++counter;
	while (counter < 12) {
		output[counter] = ' ';
		++counter;
	}

	output[12] = 0;
	return;
}

int FAT_put_content(const char* path, content_t* content) {
	int parent_ci = FAT_open_content(path);
	if (parent_ci < 0) return -1;

	directory_entry_t file_info = _content_table[parent_ci]->meta;
	uint32_t active_cluster = GET_CLUSTER_FROM_ENTRY(file_info, FAT_data.fat_type);
	_remove_content_from_table(parent_ci);

	char output[13] = { 0 };
	_fatname2name((char*)content->meta.file_name, (char*)output);
	int retVal = _directory_search(output, active_cluster, NULL, NULL);
	if (retVal == -1) {
		kprintf("Function putFile: directorySearch encountered an error. Aborting...\n");
		return -1;
	}
	else if (retVal != -2) {
		kprintf("Function putFile: a file matching the name given already exists. Aborting...\n");
		return -3;
	}

	if (_directory_add(active_cluster, &content->meta) != 0) {
		kprintf("Function FAT_put_content: _directory_add encountered an error. Aborting...\n");
		return -1;
	}

	return 0;
}

int FAT_delete_content(const char* path) {
	int ci = FAT_open_content(path);
	content_t* fat_content = FAT_get_content_from_table(ci);
	if (fat_content == NULL) {
		kprintf("Function FAT_delete_content: FAT_open_content encountered an error. Aborting...\n");
		return -1;
	}

	uint32_t data_cluster = GET_CLUSTER_FROM_ENTRY(fat_content->meta, FAT_data.fat_type);
	uint32_t prev_cluster = 0;

	while (data_cluster < END_CLUSTER_32) {
		prev_cluster = __read_fat(data_cluster);
		if (_cluster_deallocate(data_cluster) != 0) {
			kprintf("[%s %i] _cluster_deallocate encountered an error. Aborting...\n", __FILE__, __LINE__);
			_remove_content_from_table(ci);
			return -1;
		}

		data_cluster = prev_cluster;
	}

	if (_directory_remove(fat_content->parent_cluster, (char*)fat_content->meta.file_name) != 0) {
		kprintf("[%s %i] _directory_remove encountered an error. Aborting...\n", __FILE__, __LINE__);
		_remove_content_from_table(ci);
		return -1;
	}

	_remove_content_from_table(ci);
	return 0;
}

void FAT_copy_content(char* source, char* destination) {
	int ci_source = FAT_open_content(source);

	content_t* fat_content = FAT_get_content_from_table(ci_source);
	if (!fat_content) return;
	content_t* dst_content = NULL;

	directory_entry_t content_meta;
	memcpy(&content_meta, &fat_content->meta, sizeof(directory_entry_t));

	if (fat_content->directory != NULL) dst_content = FAT_create_object(fat_content->directory->name, 1, NULL);
	else if (fat_content->file != NULL) dst_content = FAT_create_object(fat_content->file->name, 0, fat_content->file->extension);

	if (FAT_put_content(destination, dst_content) != 0) {
		FAT_unload_content_system(dst_content);
		_remove_content_from_table(ci_source);
		return;
	}

	char dst_path[256] = { 0 };
	strcpy(dst_path, destination);
	if (strlen(dst_path) > 0) strcat(dst_path, "\\");
	strcat(dst_path, fat_content->directory != NULL ? fat_content->directory->name : fat_content->file->name);
	if (fat_content->file != NULL && strlen(fat_content->file->extension) > 0) {
		strcat(dst_path, ".");
		strcat(dst_path, fat_content->file->extension);
	}

	int ci_destination = FAT_open_content(dst_path);
	if (ci_destination < 0) {
		FAT_unload_content_system(dst_content);
		_remove_content_from_table(ci_source);
		return;
	}

	uint32_t data_cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
	content_t* dst_fd_content = FAT_get_content_from_table(ci_destination);
	uint32_t dst_cluster  = GET_CLUSTER_FROM_ENTRY(dst_fd_content->meta, FAT_data.fat_type);

	while (data_cluster < END_CLUSTER_32) {
		_copy_cluster2cluster(data_cluster, dst_cluster);

		uint32_t next_data_cluster = __read_fat(data_cluster);
		if (_is_cluster_end(next_data_cluster, FAT_data.fat_type) == 1) break;

		uint32_t next_dst_cluster = __read_fat(dst_cluster);
		if (_is_cluster_end(next_dst_cluster, FAT_data.fat_type) == 1) {
			next_dst_cluster = _add_cluster_to_content(ci_destination);
			if ((int)next_dst_cluster < 0) break;
		}

		data_cluster = next_data_cluster;
		dst_cluster = next_dst_cluster;
	}

	_remove_content_from_table(ci_destination);
	_remove_content_from_table(ci_source);
	FAT_unload_content_system(dst_content);
}

int FAT_stat_content(int ci, CInfo_t* info) {
	content_t* content = FAT_get_content_from_table(ci);
	if (!content) {
		info->type = NOT_PRESENT;
		return -1;
	}

	if (content->content_type == CONTENT_TYPE_DIRECTORY) {
		info->size = 0;
		memcpy(info->full_name, content->directory->name, 11);
		info->full_name[11] = '\0';
		info->type = STAT_DIR;
	}
	else if (content->content_type == CONTENT_TYPE_FILE) {
		info->size = content->meta.file_size;
		memcpy(info->full_name, content->meta.file_name, 11);
		info->full_name[11] = '\0';
		strncpy(info->file_name, content->file->name, sizeof(info->file_name) - 1);
		info->file_name[sizeof(info->file_name) - 1] = '\0';
		strncpy(info->file_extension, content->file->extension, sizeof(info->file_extension) - 1);
		info->file_extension[sizeof(info->file_extension) - 1] = '\0';
		info->type = STAT_FILE;
	}
	else {
		return -2;
	}

	info->creation_date = content->meta.creation_date;
	info->creation_time = content->meta.creation_time;
	info->last_accessed = content->meta.last_accessed;
	info->last_modification_date = content->meta.last_modification_date;
	info->last_modification_time = content->meta.last_modification_time;

	return 1;
}

int _add_content2table(content_t* content) {
	if (_is_static_content(content)) {
		int slot = _content_slot(content);
		if (slot >= 0) {
			_content_table[slot] = content;
			return slot;
		}
	}

	for (int i = 0; i < CONTENT_TABLE_SIZE; i++) {
		if (!_content_table[i]) {
			memcpy(&_content_storage[i], content, sizeof(content_t));
			if (content->content_type == CONTENT_TYPE_FILE) {
				memcpy(&_file_storage[i], content->file, sizeof(file_t));
				_file_storage[i].cluster_table = NULL;
				if (content->file->cluster_table != NULL && content->file->cluster_table_size > 0) {
					_file_storage[i].cluster_table = malloc(sizeof(uint32_t) * content->file->cluster_table_size);
					if (_file_storage[i].cluster_table != NULL) {
						memcpy(
							_file_storage[i].cluster_table,
							content->file->cluster_table,
							sizeof(uint32_t) * content->file->cluster_table_size
						);
					}
				}
				_content_storage[i].file = &_file_storage[i];
			}
			else if (content->content_type == CONTENT_TYPE_DIRECTORY) {
				memcpy(&_directory_storage[i], content->directory, sizeof(directory_t));
				_content_storage[i].directory = &_directory_storage[i];
			}

			_content_table[i] = &_content_storage[i];
			return i;
		}
	}

	return -1;
}

static directory_entry_t* _create_entry(const char* name, const char* ext, int isDir, uint32_t firstCluster, uint32_t filesize) {
	static directory_entry_t data_storage;
	static char file_name[25];
	directory_entry_t* data = &data_storage;
	memset(data, 0, sizeof(directory_entry_t));
	memset(file_name, 0, sizeof(file_name));

	data->reserved0 			 = 0;
	data->creation_time_tenths 	 = 0;
	data->creation_time 		 = 0;
	data->creation_date 		 = 0;
	data->last_modification_date = 0;

	strcpy(file_name, name);
	if (ext) {
		strcat(file_name, ".");
		strcat(file_name, ext);
	}

	data->low_bits 	= firstCluster;
	data->high_bits = firstCluster >> 16;

	if (isDir == 1) {
		data->file_size  = 0;
		data->attributes = FILE_DIRECTORY;
	}
	else {
		data->file_size  = filesize;
		data->attributes = FILE_ARCHIVE;
	}

	data->creation_date = _current_date();
	data->creation_time = _current_time();
	data->creation_time_tenths = _current_time();

	if (_name_check(file_name) != 0) _name2fatname(file_name);
	strncpy((char*)data->file_name, file_name, min(11, strlen(file_name)));

	return data;
}

static directory_t* _create_directory() {
	_reset_directory(&_scratch_directory);
	return &_scratch_directory;
}

static file_t* _create_file() {
	_reset_file(&_scratch_file);
	return &_scratch_file;
}

content_t* FAT_create_object(char* name, int is_directory, char* extension) {
	content_t* content = FAT_create_content();
	if (strlen(name) > 11 || (extension && strlen(extension) > 4)) {
		kprintf("Uncorrect name or ext lenght.\n");
		FAT_unload_content_system(content);
		return NULL;
	}

	if (is_directory) {
		content->content_type = CONTENT_TYPE_DIRECTORY;
		content->directory = _create_directory();
		strncpy(content->directory->name, name, 12);

		directory_entry_t* meta = _create_entry(name, NULL, 1, _cluster_allocate(), 0);
		if (meta) memcpy(&content->meta, meta, sizeof(directory_entry_t));
	}
	else {
		content->content_type = CONTENT_TYPE_FILE;
		content->file = _create_file();
		strncpy(content->file->name, name, 8);
		if (extension) strncpy(content->file->extension, extension, 4);

		directory_entry_t* meta = _create_entry(name, extension, 0, _cluster_allocate(), 1);
		if (meta) memcpy(&content->meta, meta, sizeof(directory_entry_t));
	}

	return content;
}

content_t* FAT_create_content() {
	_reset_content(&_scratch_content);
	_reset_file(&_scratch_file);
	_reset_directory(&_scratch_directory);
	return &_scratch_content;
}

int FAT_unload_content_system(content_t* content) {
	if (!content) return -1;
	if (_is_static_content(content)) {
		_release_content_slot(_content_slot(content));
	}
	else {
		_reset_content(content);
		_reset_file(&_scratch_file);
		_reset_directory(&_scratch_directory);
	}

	return 1;
}
