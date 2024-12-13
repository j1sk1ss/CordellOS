#include "../../include/fat.h"

//========================================================================================

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

	static Content* _content_table[CONTENT_TABLE_SIZE] = { NULL };

//========================================================================================
//   ____   ___   ___ _____   ____  _____ ____ _____ ___  ____  
//  | __ ) / _ \ / _ \_   _| / ___|| ____/ ___|_   _/ _ \|  _ \ 
//  |  _ \| | | | | | || |   \___ \|  _|| |     | || | | | |_) |
//  | |_) | |_| | |_| || |    ___) | |__| |___  | || |_| |  _ < 
//  |____/ \___/ \___/ |_|   |____/|_____\____| |_| \___/|_| \_\
//
//========================================================================================
// Initializes struct "bootsect" to store critical data from the boot sector of the volume

#pragma region [Boot sector]

	int FAT_initialize() {
		uint8_t* cluster_data = ATA_read_sector(0);
		if (cluster_data == NULL) {
			kprintf("[%s %i] Function FAT_initialize: Error reading the first sector of FAT!\n", __FILE__, __LINE__);
			return -1;
		}

		fat_BS_t* bootstruct = (fat_BS_t*)cluster_data;
		FAT_data.total_sectors = (bootstruct->total_sectors_16 == 0) ? bootstruct->total_sectors_32 : bootstruct->total_sectors_16;
		FAT_data.fat_size = (bootstruct->table_size_16 == 0) ? ((fat_extBS_32_t*)(bootstruct->extended_section))->table_size_32 : bootstruct->table_size_16;

		int root_dir_sectors = ((bootstruct->root_entry_count * 32) + (bootstruct->bytes_per_sector - 1)) / bootstruct->bytes_per_sector;
		int data_sectors = FAT_data.total_sectors - (bootstruct->reserved_sector_count + (bootstruct->table_count * FAT_data.fat_size) + root_dir_sectors);

		FAT_data.total_clusters = data_sectors / bootstruct->sectors_per_cluster;
		if (FAT_data.total_clusters == 0) FAT_data.total_clusters = bootstruct->total_sectors_32 / bootstruct->sectors_per_cluster;
		FAT_data.first_data_sector = bootstruct->reserved_sector_count + bootstruct->table_count * bootstruct->table_size_16 + (bootstruct->root_entry_count * 32 + bootstruct->bytes_per_sector - 1) / bootstruct->bytes_per_sector;

		if (FAT_data.total_clusters < 4085) FAT_data.fat_type  = 12;
		else if (FAT_data.total_clusters < 65525) FAT_data.fat_type = 16;
		else {
			FAT_data.fat_type = 32;
			FAT_data.first_data_sector = bootstruct->reserved_sector_count + bootstruct->table_count * ((fat_extBS_32_t*)(bootstruct->extended_section))->table_size_32;
		}

		FAT_data.sectors_per_cluster = bootstruct->sectors_per_cluster;
		FAT_data.bytes_per_sector = bootstruct->bytes_per_sector;
		FAT_data.first_fat_sector = bootstruct->reserved_sector_count;
		FAT_data.ext_root_cluster = ((fat_extBS_32_t*)(bootstruct->extended_section))->root_cluster;
		FAT_data.cluster_size = FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster;

		_kfree(cluster_data);
		for (int i = 0; i < CONTENT_TABLE_SIZE; i++) {
			_content_table[i] = NULL;
		}

		return 0;
	}

#pragma endregion

#pragma region [Cluster functions]

	int _is_cluster_free(uint32_t cluster) {
		if (cluster == 0) return 1;
		return 0;
	}

	int _set_cluster_free(uint32_t cluster) {
		return __write_fat(cluster, 0);
	}

	int _is_cluster_end(uint32_t cluster, int fatType) {
		if ((cluster == END_CLUSTER_32 && FAT_data.fat_type == 32) ||
			(cluster == END_CLUSTER_16 && FAT_data.fat_type == 16) ||
			(cluster == END_CLUSTER_12 && FAT_data.fat_type == 12))
			return 1;

		return 0;
	}

	int _set_cluster_end(uint32_t cluster, int fatType) {
		if (FAT_data.fat_type == 32) return __write_fat(cluster, END_CLUSTER_32);
		if (FAT_data.fat_type == 16) return __write_fat(cluster, END_CLUSTER_16);
		if (FAT_data.fat_type == 12) return __write_fat(cluster, END_CLUSTER_12);
		return -1;
	}

	int _is_cluster_bad(uint32_t cluster, int fatType) {
		if ((cluster == BAD_CLUSTER_32 && FAT_data.fat_type == 32) ||
			(cluster == BAD_CLUSTER_16 && FAT_data.fat_type == 16) ||
			(cluster == BAD_CLUSTER_12 && FAT_data.fat_type == 12))
			return 1;

		return 0;
	}

	int _set_cluster_bad(uint32_t cluster, int fatType) {
		if (FAT_data.fat_type == 32) return __write_fat(cluster, BAD_CLUSTER_32);
		if (FAT_data.fat_type == 16) return __write_fat(cluster, BAD_CLUSTER_16);
		if (FAT_data.fat_type == 12) return __write_fat(cluster, BAD_CLUSTER_12);
		return -1;
	}

#pragma endregion

//========================================================================================

#pragma region [FAT RW functions]

//========================================================================================
//   ____  _____    _    ____    _____ _  _____ 
//  |  _ \| ____|  / \  |  _ \  |  ___/ \|_   _|
//  | |_) |  _|   / _ \ | | | | | |_ / _ \ | |  
//  |  _ <| |___ / ___ \| |_| | |  _/ ___ \| |  
//  |_| \_\_____/_/   \_\____/  |_|/_/   \_\_|  
//
//========================================================================================
// This function reads FAT table for getting cluster status (or cluster chain)

	int __read_fat(uint32_t cluster) {
		assert(
			cluster >= 2 && cluster < FAT_data.total_clusters &&
			(FAT_data.fat_type == 32 || FAT_data.fat_type == 16)
		);

		uint32_t fat_offset = cluster * (FAT_data.fat_type == 16 ? 2 : 4);
		uint8_t* cluster_data = ATA_read_sectors(FAT_data.first_fat_sector + (fat_offset / FAT_data.cluster_size), FAT_data.sectors_per_cluster);
		if (cluster_data == NULL) {
			kprintf("[%s %i] Function __read_fat: Could not read sector that contains FAT32 table entry needed.\n", __FILE__, __LINE__);
			return -1;
		}

		uint32_t table_value = *(uint32_t*)&cluster_data[fat_offset % FAT_data.cluster_size];
		if (FAT_data.fat_type == 32) table_value &= 0x0FFFFFFF;

		_kfree(cluster_data);
		return table_value;
	}

//========================================================================================
//  __        ______  ___ _____ _____   _____ _  _____ 
//  \ \      / /  _ \|_ _|_   _| ____| |  ___/ \|_   _|
//   \ \ /\ / /| |_) || |  | | |  _|   | |_ / _ \ | |  
//    \ V  V / |  _ < | |  | | | |___  |  _/ ___ \| |  
//     \_/\_/  |_| \_\___| |_| |_____| |_|/_/   \_\_|  
//
//========================================================================================
// This function writes cluster status to FAT table

	int __write_fat(uint32_t cluster, uint32_t value) {
		assert(
			cluster >= 2 && cluster < FAT_data.total_clusters &&
			(FAT_data.fat_type == 32 || FAT_data.fat_type == 16)
		);
		
		uint32_t fat_offset = cluster * (FAT_data.fat_type == 16 ? 2 : 4);
		uint32_t fat_sector = FAT_data.first_fat_sector + (fat_offset / FAT_data.cluster_size);
		
		uint8_t* cluster_data = ATA_read_sectors(fat_sector, FAT_data.sectors_per_cluster);
		if (cluster_data == NULL) {
			kprintf("Function __write_fat: Could not read sector that contains FAT32 table entry needed.\n");
			return -1;
		}

		*(uint32_t*)&cluster_data[fat_offset % FAT_data.cluster_size] = value;
		if (ATA_write_sectors(fat_sector, cluster_data, FAT_data.sectors_per_cluster) != 1) {
			kprintf("Function __write_fat: Could not write new FAT32 cluster number to sector.\n");
			return -1;
		}

		_kfree(cluster_data);
		return 0;
	}

//========================================================================================

#pragma endregion

#pragma region [FAT AD functions]

	static uint32_t last_allocated_cluster = SECTOR_OFFSET;
	
	uint32_t _cluster_allocate() {
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

	int _cluster_deallocate(const uint32_t cluster) {
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

//========================================================================================

#pragma endregion

#pragma region [Cluster RW functions]

//========================================================================================
//    ____ _    _   _ ____ _____ _____ ____    ____  _____    _    ____  
//   / ___| |  | | | / ___|_   _| ____|  _ \  |  _ \| ____|  / \  |  _ \ 
//  | |   | |  | | | \___ \ | | |  _| | |_) | | |_) |  _|   / _ \ | | | |
//  | |___| |__| |_| |___) || | | |___|  _ <  |  _ <| |___ / ___ \| |_| |
//   \____|_____\___/|____/ |_| |_____|_| \_\ |_| \_\_____/_/   \_\____/ 
//
//========================================================================================
// Reads one cluster
// This function deals in absolute data clusters

	// Read cluster data
	uint8_t* _cluster_read(uint32_t cluster) {
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);
		uint32_t start_sect = (cluster - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		uint8_t* cluster_data = ATA_read_sectors(start_sect, FAT_data.sectors_per_cluster);
		assert(cluster_data != NULL);
		return cluster_data;
	}

	// Read cluster data and stop reading when script meets one of stop values
	uint8_t* _cluster_read_stop(uint32_t cluster, uint8_t* stop) {
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);
		uint32_t start_sect = (cluster - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		uint8_t* response = ATA_read_sectors_stop(start_sect, FAT_data.sectors_per_cluster, stop);
		assert(response != NULL);
		return response;
	}

	// Read cluster with seek
	uint8_t* _cluster_readoff(uint32_t cluster, uint32_t offset) {
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);
		uint32_t start_sect = (cluster - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		uint8_t* cluster_data = ATA_readoff_sectors(start_sect, offset, FAT_data.sectors_per_cluster);
		assert(cluster_data != NULL);
		return cluster_data;
	}

	// Read cluster data and stop reading when script meets one of stop values with seek
	uint8_t* _cluster_readoff_stop(uint32_t cluster, uint32_t offset, uint8_t* stop) {
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);
		uint32_t start_sect = (cluster - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		uint8_t* cluster_data = ATA_readoff_sectors_stop(start_sect, offset, FAT_data.sectors_per_cluster, stop);
		assert(cluster_data != NULL);
		return cluster_data;
	}

//========================================================================================
//    ____ _    _   _ ____ _____ _____ ____   __        ______  ___ _____ _____ 
//   / ___| |  | | | / ___|_   _| ____|  _ \  \ \      / /  _ \|_ _|_   _| ____|
//  | |   | |  | | | \___ \ | | |  _| | |_) |  \ \ /\ / /| |_) || |  | | |  _|  
//  | |___| |__| |_| |___) || | | |___|  _ <    \ V  V / |  _ < | |  | | | |___ 
//   \____|_____\___/|____/ |_| |_____|_| \_\    \_/\_/  |_| \_\___| |_| |_____|
//
//========================================================================================
// Deals in absolute clusters
// contentsToWrite: contains a pointer to the data to be written to disk
// clusterNum: Specifies the on-disk cluster to write the data to

	int _cluster_write(uint8_t* data, uint32_t cluster) {
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);
		uint32_t start_sect = (cluster - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		return ATA_write_sectors(start_sect, data, FAT_data.sectors_per_cluster);
	}

	int _cluster_writeoff(uint8_t* data, uint32_t cluster, uint32_t offset, uint32_t size) {
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);
		uint32_t start_sect = (cluster - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		return ATA_writeoff_sectors(start_sect, data, FAT_data.sectors_per_cluster, offset, size);
	}

//========================================================================================
//    ____ _    _   _ ____ _____ _____ ____     ____ ___  ______   __
//   / ___| |  | | | / ___|_   _| ____|  _ \   / ___/ _ \|  _ \ \ / /
//  | |   | |  | | | \___ \ | | |  _| | |_) | | |  | | | | |_) \ V / 
//  | |___| |__| |_| |___) || | | |___|  _ <  | |__| |_| |  __/ | |  
//   \____|_____\___/|____/ |_| |_____|_| \_\  \____\___/|_|    |_|  
//
//========================================================================================
// Copy cluster2cluster

	int _copy_cluster2cluster(uint32_t source, uint32_t destination) {
		assert(
			source >= 2 && source < FAT_data.total_clusters &&
			destination >= 2 && destination < FAT_data.total_clusters
		);
		
		uint32_t first = (source - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		uint32_t second = (destination - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		return ATA_copy_sectors2sectors(first, second, FAT_data.sectors_per_cluster);
	}

//========================================================================================
//   ____ _    _   _ ____ _____ _____ ____     ___ _____ _   _ _____ ____  
//  / ___| |  | | | / ___|_   _| ____|  _ \   / _ \_   _| | | | ____|  _ \ 
// | |   | |  | | | \___ \ | | |  _| | |_) | | | | || | | |_| |  _| | |_) |
// | |___| |__| |_| |___) || | | |___|  _ <  | |_| || | |  _  | |___|  _ < 
//  \____|_____\___/|____/ |_| |_____|_| \_\  \___/ |_| |_| |_|_____|_| \_\
//========================================================================================
//	Other functions for working with clusters

	int _clear_cluster(uint32_t clusterNum) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);
		uint8_t clear[FAT_data.sectors_per_cluster * SECTOR_SIZE];
		memset(clear, 0, FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t start_sect = (clusterNum - 2) * (uint16_t)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		return ATA_write_sectors(start_sect, (const uint8_t*)clear, FAT_data.sectors_per_cluster);
	}

	// Add allocated cluster to file
	void _add_cluster_to_content(int ci) {
		directory_entry_t content_meta = _content_table[ci]->meta;
		uint32_t cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
		while (_is_cluster_end(cluster, FAT_data.fat_type) == 0) {
			assert(_is_cluster_bad(cluster, FAT_data.fat_type) == 0);
			assert(cluster != -1);
			cluster = __read_fat(cluster);
		}

		if (_is_cluster_end(cluster, FAT_data.fat_type) == 1) {
			uint32_t newCluster = _cluster_allocate();
			assert(_is_cluster_bad(newCluster, FAT_data.fat_type) == 0);
			assert(__write_fat(cluster, newCluster) == 0);
		}
	}

//========================================================================================

#pragma endregion

//========================================================================================
//   ____ ___ ____  _____ ____ _____ ___  ______   __  _     ____  
//  |  _ \_ _|  _ \| ____/ ___|_   _/ _ \|  _ \ \ / / | |   / ___| 
//  | | | | || |_) |  _|| |     | || | | | |_) \ V /  | |   \___ \ 
//  | |_| | ||  _ <| |__| |___  | || |_| |  _ < | |   | |___ ___) |
//  |____/___|_| \_\_____\____| |_| \___/|_| \_\|_|   |_____|____/
//
//========================================================================================
// receives the cluster to list, and will list all regular entries and directories, plus whatever attributes are passed in
// returns: -1 is a general error

	int FAT_directory_list(int ci, uint8_t attrs, int exclusive) {
		uint32_t cluster = GET_CLUSTER_FROM_ENTRY(_content_table[ci]->meta, FAT_data.fat_type);
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);
		
		Content* content = FAT_create_content();
		content->directory = _create_directory();
		content->parent_cluster = 0;

		const uint8_t default_hidden_attributes = (FILE_HIDDEN | FILE_SYSTEM);
		uint8_t attributes_to_hide = default_hidden_attributes;
		if (exclusive == 0) attributes_to_hide &= (~attrs);
		else if (exclusive == 1) attributes_to_hide = (~attrs);

		uint8_t* cluster_data = _cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function FAT_directory_list: _cluster_read encountered an error. Aborting...\n");
			_unload_directory_system(content->directory);
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		uint32_t meta_pointer_iterator_count = 0;
		while (1) {
			if (file_metadata->file_name[0] == ENTRY_END) break;
			else if (strncmp((char*)file_metadata->file_name, "..", 2) == 0 ||
					strncmp((char*)file_metadata->file_name, ".", 1) == 0) {
				file_metadata++;
				meta_pointer_iterator_count++;
			}
			else if (((file_metadata->file_name)[0] == ENTRY_FREE) || ((file_metadata->attributes & FILE_LONG_NAME) == FILE_LONG_NAME)) {	
				if (meta_pointer_iterator_count < FAT_data.cluster_size / sizeof(directory_entry_t) - 1) {
					file_metadata++;
					meta_pointer_iterator_count++;
				}
				else {
					uint32_t next_cluster = __read_fat(cluster);
					if (_is_cluster_end(next_cluster, FAT_data.fat_type) == 1) break;
					else if (next_cluster < 0) {
						kprintf("Function FAT_directory_list: __read_fat encountered an error. Aborting...\n");
						FAT_unload_content_system(content);
						return -1;
					}
					else {
						FAT_unload_content_system(content);
						return FAT_directory_list(next_cluster, attrs, exclusive);
					}
				}
			}
			else {
				if ((file_metadata->attributes & FILE_DIRECTORY) != FILE_DIRECTORY) {			
					File* file = _create_file();

					char name[13] = { 0 };
					strcpy(name, (const char*)file_metadata->file_name);
					strncpy(file->name, (const char*)strtok(name, " "), 8);
					strncpy(file->extension, (const char*)strtok(NULL, " "), 4);

					if (content->directory->files == NULL) content->directory->files = file;
					else {
						File* current = content->directory->files;
						while (current->next != NULL) current = current->next;
						current->next = file;
					}
				}
				else {
					if ((file_metadata->attributes & FILE_DIRECTORY) == FILE_DIRECTORY) {
						Directory* upperDir = _create_directory();

						char name[13] = { 0 };
						strcpy(name, (char*)file_metadata->file_name);
						strncpy(upperDir->name, strtok(name, " "), 11);
						
						if (content->directory->subDirectory == NULL) content->directory->subDirectory = upperDir;
						else {
							Directory* current = content->directory->subDirectory;
							while (current->next != NULL) current = current->next;
							current->next = upperDir;
						}
					}
				}

				file_metadata++;
				meta_pointer_iterator_count++;
			}
		}

		int root_ci = _add_content2table(content);
		if (root_ci == -1) {
			kprintf("Function FAT_open_content: an error occurred in _add_content2table. Aborting...\n");
			FAT_unload_content_system(content);
			return -1;
		}

		return root_ci;
	}

//========================================================================================
//   ____ ___ ____  _____ ____ _____ ___  ______   __  ____  _____    _    ____   ____ _   _ 
//  |  _ \_ _|  _ \| ____/ ___|_   _/ _ \|  _ \ \ / / / ___|| ____|  / \  |  _ \ / ___| | | |
//  | | | | || |_) |  _|| |     | || | | | |_) \ V /  \___ \|  _|   / _ \ | |_) | |   | |_| |
//  | |_| | ||  _ <| |__| |___  | || |_| |  _ < | |    ___) | |___ / ___ \|  _ <| |___|  _  |
//  |____/___|_| \_\_____\____| |_| \___/|_| \_\|_|   |____/|_____/_/   \_\_| \_\\____|_| |_|
//
//========================================================================================
// receives the cluster to read for a directory and the requested file, and will iterate through the directory's clusters - 
// returning the entry for the searched file/subfolder, or no file/subfolder
// return value holds success or failure code, file holds directory entry if file is found
// entryOffset points to where the directory entry was found in sizeof(directory_entry_t) starting from zero (can be NULL)
// returns: -1 is a general error, -2 is a "not found" error

	int _directory_search(const char* filepart, const uint32_t cluster, directory_entry_t* file, uint32_t* entryOffset) {
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);

		char searchName[13] = { 0 };
		strcpy(searchName, filepart);
		if (_name_check(searchName) != 0)
			_name2fatname(searchName);

		uint8_t* cluster_data = _cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function _directory_search: _cluster_read encountered an error. Aborting...\n");
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		uint32_t meta_pointer_iterator_count = 0;
		while (1) {
			if (file_metadata->file_name[0] == ENTRY_END) break;
			else if (strncmp((char*)file_metadata->file_name, searchName, 11) != 0) {
				if (meta_pointer_iterator_count < FAT_data.cluster_size / sizeof(directory_entry_t) - 1) {
					file_metadata++;
					meta_pointer_iterator_count++;
				}
				else {
					int next_cluster = __read_fat(cluster);
					if (_is_cluster_end(next_cluster, FAT_data.fat_type) == 1) break;
					else if (next_cluster < 0) {
						kprintf("Function _directory_search: __read_fat encountered an error. Aborting...\n");
						_kfree(cluster_data);
						return -1;
					} 
					else {
						_kfree(cluster_data);
						return _directory_search(filepart, next_cluster, file, entryOffset);
					}
				}
			}
			else {
				if (file != NULL) memcpy(file, file_metadata, sizeof(directory_entry_t));
				if (entryOffset != NULL) *entryOffset = meta_pointer_iterator_count;

				_kfree(cluster_data);
				return 0;
			}
		}

		_kfree(cluster_data);
		return -2;
	}

//========================================================================================
//   ____ ___ ____  _____ ____ _____ ___  ______   __     _    ____  ____  
//  |  _ \_ _|  _ \| ____/ ___|_   _/ _ \|  _ \ \ / /    / \  |  _ \|  _ \ 
//  | | | | || |_) |  _|| |     | || | | | |_) \ V /    / _ \ | | | | | | |
//  | |_| | ||  _ <| |__| |___  | || |_| |  _ < | |    / ___ \| |_| | |_| |
//  |____/___|_| \_\_____\____| |_| \___/|_| \_\|_|   /_/   \_\____/|____/ 
//
//========================================================================================                                                                   
// pass in the cluster to write the directory to and the directory struct to write.
// struct should only have a file name, attributes, and size. the rest will be filled in automatically

	int _directory_add(const uint32_t cluster, directory_entry_t* file_to_add) {
		uint8_t* cluster_data = _cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function _directory_add: _cluster_read encountered an error. Aborting...\n");
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		uint32_t meta_pointer_iterator_count = 0;
		while (1) {
			if (file_metadata->file_name[0] != ENTRY_FREE && file_metadata->file_name[0] != ENTRY_END) {
				if (meta_pointer_iterator_count < FAT_data.cluster_size / sizeof(directory_entry_t) - 1) {
					file_metadata++;
					meta_pointer_iterator_count++;
				}
				else {
					uint32_t next_cluster = __read_fat(cluster);
					if (_is_cluster_end(next_cluster, FAT_data.fat_type) == 1) {
						next_cluster = _cluster_allocate();
						if (_is_cluster_bad(next_cluster, FAT_data.fat_type) == 1) {
							kprintf("Function _directory_add: allocation of new cluster failed. Aborting...\n");
							_kfree(cluster_data);
							return -1;
						}

						if (__write_fat(cluster, next_cluster) != 0) {
							kprintf("Function _directory_add: extension of the cluster chain with new cluster failed. Aborting...\n");
							_kfree(cluster_data);
							return -1;
						}
					}

					_kfree(cluster_data);
					return _directory_add(next_cluster, file_to_add);
				}
			}
			else {
				file_to_add->creation_date 			= _current_date();
				file_to_add->creation_time 			= _current_time();
				file_to_add->creation_time_tenths 	= _current_time();
				file_to_add->last_accessed 			= file_to_add->creation_date;
				file_to_add->last_modification_date = file_to_add->creation_date;
				file_to_add->last_modification_time = file_to_add->creation_time;

				uint32_t new_cluster = _cluster_allocate();
				if (_is_cluster_bad(new_cluster, FAT_data.fat_type) == 1) {
					kprintf("Function _directory_add: allocation of new cluster failed. Aborting...\n");
					_kfree(cluster_data);

					return -1;
				}
				
				file_to_add->low_bits  = GET_ENTRY_LOW_BITS(new_cluster, FAT_data.fat_type);
				file_to_add->high_bits = GET_ENTRY_HIGH_BITS(new_cluster, FAT_data.fat_type);

				memcpy(file_metadata, file_to_add, sizeof(directory_entry_t));
				if (_cluster_write(cluster_data, cluster) != 0) {
					kprintf("Function _directory_add: Writing new directory entry failed. Aborting...\n");
					_kfree(cluster_data);
					return -1;
				}

				_kfree(cluster_data);
				return 0;
			}
		}

		_kfree(cluster_data);
		return -1; //return error.
	}

//========================================================================================
//   ____ ___ ____  _____ ____ _____ ___  ______   __  _____ ____ ___ _____ 
//  |  _ \_ _|  _ \| ____/ ___|_   _/ _ \|  _ \ \ / / | ____|  _ \_ _|_   _|
//  | | | | || |_) |  _|| |     | || | | | |_) \ V /  |  _| | | | | |  | |  
//  | |_| | ||  _ <| |__| |___  | || |_| |  _ < | |   | |___| |_| | |  | |  
//  |____/___|_| \_\_____\____| |_| \___/|_| \_\|_|   |_____|____/___| |_| 
//
//========================================================================================
// This function edit names of directory entries in cluster

	int _directory_edit(const uint32_t cluster, directory_entry_t* old_meta, const char* new_name) {
		if (_name_check((char*)old_meta->file_name) != 0) {
			kprintf("Function _directory_edit: Invalid file name!");
			return -1;
		}

		uint8_t* cluster_data = _cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function _directory_edit: _cluster_read encountered an error. Aborting...\n");
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		uint32_t meta_pointer_iterator_count = 0;
		while (1) {
			if (strstr((char*)file_metadata->file_name, (char*)old_meta->file_name) == 0) {

				old_meta->last_accessed = _current_date();
				old_meta->last_modification_date = _current_date();
				old_meta->last_modification_time = _current_time();

				memset(old_meta->file_name, 0, 11);
				strncpy((char*)old_meta->file_name, new_name, 11);
				memcpy(file_metadata, old_meta, sizeof(directory_entry_t));
				
				if (_cluster_write(cluster_data, cluster) != 0) {
					kprintf("Function _directory_edit: Writing updated directory entry failed. Aborting...\n");
					_kfree(cluster_data);
					return -1;
				}

				return 0;
			} 
			
			else if (meta_pointer_iterator_count < FAT_data.cluster_size / sizeof(directory_entry_t) - 1)  {
				file_metadata++;
				meta_pointer_iterator_count++;
			} 
			
			else {
				uint32_t next_cluster = __read_fat(cluster);
				if ((next_cluster >= END_CLUSTER_32 && FAT_data.fat_type == 32) || (next_cluster >= END_CLUSTER_16 && FAT_data.fat_type == 16) || (next_cluster >= END_CLUSTER_12 && FAT_data.fat_type == 12)) {
					kprintf("Function _directory_edit: End of cluster chain reached. File not found. Aborting...\n");
					_kfree(cluster_data);
					return -2;
				}

				_kfree(cluster_data);
				return _directory_edit(next_cluster, old_meta, new_name);
			}
		}

		_kfree(cluster_data);
		return -1;
	}

//========================================================================================
//   ____ ___ ____  _____ ____ _____ ___  ______   __  ____  _____ __  __  _____     _______ 
//  |  _ \_ _|  _ \| ____/ ___|_   _/ _ \|  _ \ \ / / |  _ \| ____|  \/  |/ _ \ \   / / ____|
//  | | | | || |_) |  _|| |     | || | | | |_) \ V /  | |_) |  _| | |\/| | | | \ \ / /|  _|  
//  | |_| | ||  _ <| |__| |___  | || |_| |  _ < | |   |  _ <| |___| |  | | |_| |\ V / | |___ 
//  |____/___|_| \_\_____\____| |_| \___/|_| \_\|_|   |_| \_\_____|_|  |_|\___/  \_/  |_____|
//
//========================================================================================
// This function mark data in FAT table as _kfree and deallocates all clusters

	int _directory_remove(const uint32_t cluster, const char* fileName) {
		if (_name_check(fileName) != 0) {
			kprintf("Function _directory_remove: Invalid file name!");
			return -1;
		}

		uint8_t* cluster_data = _cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function _directory_remove: _cluster_read encountered an error. Aborting...\n");
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		uint32_t meta_pointer_iterator_count = 0;
		while (1) {
			if (strstr((char*)file_metadata->file_name, fileName) == 0) {
				file_metadata->file_name[0] = ENTRY_FREE;
				if (_cluster_write(cluster_data, cluster) != 0) {
					kprintf("Function _directory_remove: Writing updated directory entry failed. Aborting...\n");
					_kfree(cluster_data);
					return -1;
				}

				return 0;
			} 
			else if (meta_pointer_iterator_count < FAT_data.cluster_size / sizeof(directory_entry_t) - 1)  {
				file_metadata++;
				meta_pointer_iterator_count++;
			} 
			else {
				uint32_t next_cluster = __read_fat(cluster);
				if ((next_cluster >= END_CLUSTER_32 && FAT_data.fat_type == 32) || (next_cluster >= END_CLUSTER_16 && FAT_data.fat_type == 16) || (next_cluster >= END_CLUSTER_12 && FAT_data.fat_type == 12)) {
					kprintf("Function _directory_remove: End of cluster chain reached. File not found. Aborting...\n");
					_kfree(cluster_data);
					return -2;
				}

				_kfree(cluster_data);
				return _directory_remove(next_cluster, fileName);
			}
		}

		_kfree(cluster_data);
		return -1; // Return error
	}

//========================================================================================
//    ____ ___  _   _ _____ _____ _   _ _____   _______  _____ ____ _____ 
//   / ___/ _ \| \ | |_   _| ____| \ | |_   _| | ____\ \/ /_ _/ ___|_   _|
//  | |  | | | |  \| | | | |  _| |  \| | | |   |  _|  \  / | |\___ \ | |  
//  | |__| |_| | |\  | | | | |___| |\  | | |   | |___ /  \ | | ___) || |  
//   \____\___/|_| \_| |_| |_____|_| \_| |_|   |_____/_/\_\___|____/ |_| 
//
//========================================================================================
// Function that checks is content exist
// returns: 0 if nexist and 1 if exist

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

		return 1; // Content exists
	}

//========================================================================================
//    ____ ___  _   _ _____ _____ _   _ _____    ____ _____ _____ 
//   / ___/ _ \| \ | |_   _| ____| \ | |_   _|  / ___| ____|_   _|
//  | |  | | | |  \| | | | |  _| |  \| | | |   | |  _|  _|   | |  
//  | |__| |_| | |\  | | | | |___| |\  | | |   | |_| | |___  | |  
//   \____\___/|_| \_| |_| |_____|_| \_| |_|    \____|_____| |_|  
//
//========================================================================================
// Returns: -1 is general error, -2 is content not found

	int FAT_open_content(const char* path) {
		Content* fat_content = FAT_create_content();

		char fileNamePart[256] = { 0 };
		uint16_t start = 0;
		uint32_t active_cluster = 0;

		if (FAT_data.fat_type == 32) active_cluster = FAT_data.ext_root_cluster;
		else {
			kprintf("Function FAT_open_content: FAT16 and FAT12 are not supported!\n");
			FAT_unload_content_system(fat_content);
			return -1;
		}
		
		directory_entry_t content_meta;
		for (uint32_t iterator = 0; iterator <= strlen(path); iterator++) 
			if (path[iterator] == '\\' || path[iterator] == '\0') {
				memset(fileNamePart, '\0', 256);
				memcpy(fileNamePart, path + start, iterator - start);

				int result = _directory_search(fileNamePart, active_cluster, &content_meta, NULL);
				if (result == -2) {
					FAT_unload_content_system(fat_content);
					return -1;
				}
				else if (result == -1) {
					kprintf("Function FAT_open_content: An error occurred in _directory_search. Aborting...\n");
					FAT_unload_content_system(fat_content);
					return -1;
				}

				start = iterator + 1;
				active_cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
				if (path[iterator] != '\0') fat_content->parent_cluster = active_cluster;
			}
		
		if ((content_meta.attributes & FILE_DIRECTORY) != FILE_DIRECTORY) {
			fat_content->file = _create_file();
			uint32_t* content = NULL;
			int content_size = 0;
			
			int cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
			while (cluster < END_CLUSTER_32) {
				uint32_t* new_content = (uint32_t*)_krealloc(content, (content_size + 1) * sizeof(uint32_t));
				if (new_content == NULL) {
					_kfree(content);
					return -1;
				}

				new_content[content_size] = cluster;
				content = new_content;
				content_size++;

				cluster = __read_fat(cluster);
				if (cluster == BAD_CLUSTER_32) {
					kprintf("Function FAT_open_content: the cluster chain is corrupted with a bad cluster. Aborting...\n");
					_kfree(content);
					return -1;
				}
				else if (cluster == -1) {
					kprintf("Function FAT_open_content: an error occurred in __read_fat. Aborting...\n");
					_kfree(content);
					return -1;
				}
			}
			
			fat_content->file->data = (uint32_t*)_kmalloc(content_size * sizeof(uint32_t));
			memcpy(fat_content->file->data, content, content_size * sizeof(uint32_t));
			fat_content->file->data_size = content_size;
			_kfree(content);

			fat_content->meta = content_meta;

			char name[13] = { 0 };
			strcpy(name, (char*)fat_content->meta.file_name);
			strncpy(fat_content->file->name, strtok(name, " "), 8);
			strncpy(fat_content->file->extension, strtok(NULL, " "), 4);
		}
		else {
			fat_content->directory = _create_directory();
			fat_content->meta = content_meta;
			strncpy(fat_content->directory->name, (char*)content_meta.file_name, 10);
		}

		int ci = _add_content2table(fat_content);
		if (ci == -1) {
			kprintf("Function FAT_open_content: an error occurred in _add_content2table. Aborting...\n");
			FAT_unload_content_system(fat_content);
			return -1;
		}

		return ci;
	}

	int FAT_close_content(int ci) {
		return _remove_content_from_table(ci);
	}

	// Function for reading part of file
	// data - content for reading
	// buffer - buffer data storage
	// offset - file seek
	// size - size of read data
	int FAT_read_content2buffer(int ci, uint8_t* buffer, uint32_t offset, uint32_t size) {
		uint32_t data_seek     = offset % (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t cluster_seek  = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t data_position = 0;

		Content* data = _content_table[ci];
		if (data == NULL) return -1;

		for (int i = cluster_seek; i < data->file->data_size && data_position < size; i++) {
			uint32_t copy_size = min(SECTOR_SIZE * FAT_data.sectors_per_cluster - data_seek, size - data_position);
			uint8_t* content_part = _cluster_readoff(data->file->data[i], data_seek);	

			memcpy(buffer + data_position, content_part, copy_size);
			_kfree(content_part);
			
			data_position += copy_size;
			data_seek = 0;
		}

		return data_position;
	}

	// Function for reading part of file
	// data - content for reading
	// buffer - buffer data storage
	// offset - file seek
	// size - size of read data
	// stop - value that will stop reading
	int FAT_read_content2buffer_stop(int ci, uint8_t* buffer, uint32_t offset, uint32_t size, uint8_t* stop) {
		uint32_t data_seek     = offset % (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t cluster_seek  = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t data_position = 0;

		Content* data = _content_table[ci];
		if (data == NULL) return -1;
		
		for (int i = cluster_seek; i < data->file->data_size && data_position < size; i++) {
			uint32_t copy_size = min(SECTOR_SIZE * FAT_data.sectors_per_cluster - data_seek, size - data_position);
			uint8_t* content_part = _cluster_readoff_stop(data->file->data[i], data_seek, stop);

			memcpy(buffer + data_position, content_part, copy_size);
			_kfree(content_part);
			
			data_position += copy_size;
			data_seek = 0;

			if (stop[0] == STOP_SYMBOL) break;
		}

		return data_position;
	}

	int FAT_ELF_execute_content(int ci, int argc, char* argv[], int type) {
		ELF32_program* program = ELF_read(ci, type);

		int (*programEntry)(int, char* argv[]) = (int (*)(int, char* argv[]))(program->entry_point);
		if (programEntry == NULL) return -255;
		
		int result_code = programEntry(argc, argv);
		ELF_free_program(program, type);

		return result_code;
	}

//========================================================================================
//    ____ ___  _   _ _____ _____ _   _ _____   _____ ____ ___ _____ 
//   / ___/ _ \| \ | |_   _| ____| \ | |_   _| | ____|  _ \_ _|_   _|
//  | |  | | | |  \| | | | |  _| |  \| | | |   |  _| | | | | |  | |  
//  | |__| |_| | |\  | | | | |___| |\  | | |   | |___| |_| | |  | |  
//   \____\___/|_| \_| |_| |_____|_| \_| |_|   |_____|____/___| |_|  
//
//========================================================================================
// This function edit content in FAT content object
	
	// Write data to content with offset from buffer
	// data - content where data will be placed
	// buffer - data that will be saved in content
	// offset - content seek
	// size - write size
	int FAT_write_buffer2content(int ci, uint8_t* buffer, uint32_t offset, uint32_t size) {
		Content* data = _content_table[ci];
		if (data == NULL) return -1;
		if (data->file == NULL) return -2;

		uint32_t cluster_seek = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t data_position = 0;
		uint32_t cluster_position = 0;
		uint32_t prev_offset = offset;

		// Write to presented clusters
		for (cluster_position = cluster_seek; cluster_position < data->file->data_size && data_position < size; cluster_position++) {
			uint32_t write_size = min(size - data_position, FAT_data.sectors_per_cluster * SECTOR_SIZE);
			_cluster_writeoff(buffer + data_position, data->file->data[cluster_position], offset, write_size);

			offset = 0;
			data_position += write_size;
		}

		// Allocate cluster and write
		if (data_position < size) {
			// Calculate new variables
			uint32_t new_offset = prev_offset + data_position;
			uint32_t new_size   = size - data_position;
			uint8_t* new_buffer = buffer + data_position;

			// Allocate cluster
			_add_cluster_to_content(ci);
			FAT_write_buffer2content(ci, new_buffer, new_offset, new_size);
		}

		return 1;
	}

//========================================================================================
//    ____ _   _    _    _   _  ____ _____   __  __ _____ _____  _    
//   / ___| | | |  / \  | \ | |/ ___| ____| |  \/  | ____|_   _|/ \   
//  | |   | |_| | / _ \ |  \| | |  _|  _|   | |\/| |  _|   | | / _ \  
//  | |___|  _  |/ ___ \| |\  | |_| | |___  | |  | | |___  | |/ ___ \ 
//   \____|_| |_/_/   \_\_| \_|\____|_____| |_|  |_|_____| |_/_/   \_\
//
//========================================================================================
// This function finds content in FAT table and change their name
	int FAT_change_meta(const char* path, const char* new_name) {

		char fileNamePart[256] = { 0 };
		uint16_t start = 0;
		uint32_t active_cluster = 0;
		uint32_t prev_active_cluster = 0;

		//////////////////////
		//	FAT ACTIVE CLUSTER CHOOSING

			if (FAT_data.fat_type == 32) active_cluster = FAT_data.ext_root_cluster;
			else {
				kprintf("Function FAT_change_meta: FAT16 and FAT12 are not supported!\n");
				return -1;
			}

		//	FAT ACTIVE CLUSTER CHOOSING
		//////////////////////
		//	FINDING DIR BY PATH

			directory_entry_t file_info; //holds found directory info
			if (strlen(path) == 0) { // Create main dir if it not created (root dir)
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
						active_cluster = GET_CLUSTER_FROM_ENTRY(file_info, FAT_data.fat_type); //prep for next search
					}
			}

		//	FINDING DIR\FILE BY PATH
		//////////////////////
		// EDIT DATA

			if (_directory_edit(prev_active_cluster, &file_info, new_name) != 0) {
				kprintf("Function FAT_change_meta: _directory_edit encountered an error. Aborting...\n");
				return -1;
			}
		
		// EDIT DATA
		//////////////////////

		return 0; // directory or file successfully deleted
	}

//========================================================================================
//    ____ ___  _   _ _____ _____ _   _ _____   ____  _   _ _____ 
//   / ___/ _ \| \ | |_   _| ____| \ | |_   _| |  _ \| | | |_   _|
//  | |  | | | |  \| | | | |  _| |  \| | | |   | |_) | | | | | |  
//  | |__| |_| | |\  | | | | |___| |\  | | |   |  __/| |_| | | |  
//   \____\___/|_| \_| |_| |_____|_| \_| |_|   |_|    \___/  |_| 
//
//========================================================================================
// writes a new file to the file system
// content: contains the full data of content (meta, name, ext, type)
// returns: -1 is general error, -2 indicates a bad path/file name, -3 indicates file with same name already exists, -4 indicates file size error

	int FAT_put_content(const char* path, Content* content) {
		int parent_ci = FAT_open_content(path);
		if (parent_ci == -1) return -1;

		directory_entry_t file_info = _content_table[parent_ci]->meta;
		uint32_t active_cluster = GET_CLUSTER_FROM_ENTRY(file_info, FAT_data.fat_type);
		_remove_content_from_table(parent_ci);

		char output[13] = { 0 };
		_fatname2name((char*)content->meta.file_name, output);
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

		return 0; // file successfully written
	}

//========================================================================================
//    ____ ___  _   _ _____ _____ _   _ _____   ____  _____ _     _____ _____ _____ 
//   / ___/ _ \| \ | |_   _| ____| \ | |_   _| |  _ \| ____| |   | ____|_   _| ____|
//  | |  | | | |  \| | | | |  _| |  \| | | |   | | | |  _| | |   |  _|   | | |  _|  
//  | |__| |_| | |\  | | | | |___| |\  | | |   | |_| | |___| |___| |___  | | | |___ 
//   \____\___/|_| \_| |_| |_____|_| \_| |_|   |____/|_____|_____|_____| |_| |_____|
//
//========================================================================================
// This function delete content from FS
// path - path where placed content
// name - name of content (if it file - with extension (if presented), like "test.txt")

	int FAT_delete_content(const char* path) {
		int ci = FAT_open_content(path);
		Content* fat_content = _content_table[ci];
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
		return 0; // directory or file successfully deleted
	}

//========================================================================================
//    ____ ___  _   _ _____ _____ _   _ _____    ____ ___  ______   __
//   / ___/ _ \| \ | |_   _| ____| \ | |_   _|  / ___/ _ \|  _ \ \ / /
//  | |  | | | |  \| | | | |  _| |  \| | | |   | |  | | | | |_) \ V / 
//  | |__| |_| | |\  | | | | |___| |\  | | |   | |__| |_| |  __/ | |  
//   \____\___/|_| \_| |_| |_____|_| \_| |_|    \____\___/|_|    |_|  
//
//========================================================================================
// This function copy content in FS
// source - path to copy
// destination - path, where copy will be placed
// TODO: recurse for dirs

	void FAT_copy_content(char* source, char* destination) {
		int ci_source = FAT_open_content(source);

		Content* fat_content = _content_table[ci_source];
		Content* dst_content = NULL;

		directory_entry_t content_meta = fat_content->meta;
		directory_entry_t dst_meta;
		if (fat_content->directory != NULL) 
			dst_content = FAT_create_object(fat_content->directory->name, 1, NULL);
		else if (fat_content->file != NULL) 
			dst_content = FAT_create_object(fat_content->file->name, 0, fat_content->file->extension);
		
		dst_meta = dst_content->meta;
		int ci_destination = FAT_put_content(destination, dst_content);
		uint32_t data_cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
		uint32_t dst_cluster  = GET_CLUSTER_FROM_ENTRY(dst_meta, FAT_data.fat_type);

		while (data_cluster < END_CLUSTER_32) {
			_add_cluster_to_content(ci_destination);
			dst_cluster = __read_fat(dst_cluster);

			_copy_cluster2cluster(data_cluster, dst_cluster);
			data_cluster = __read_fat(data_cluster);
		}

		_remove_content_from_table(ci_destination);
		_remove_content_from_table(ci_source);
	}

	int FAT_stat_content(int ci, CInfo_t* info) {
		Content* content = _content_table[ci];
		if (content == NULL) {
			info->type = NOT_PRESENT;
			return -1;
		}

		if (content->directory != NULL) {
			info->size = 0;
			strcpy((char*)info->full_name, (char*)content->directory->name);
			info->type = STAT_DIR;
		}
		else if (content->file != NULL) {
			info->size = content->file->data_size * FAT_data.sectors_per_cluster * SECTOR_SIZE;
			strcpy((char*)info->full_name, (char*)content->meta.file_name);
			strcpy(info->file_name, content->file->name);
			strcpy(info->file_extension, content->file->extension);
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

//========================================================================================
//    ___ _____ _   _ _____ ____  
//   / _ \_   _| | | | ____|  _ \ 
//  | | | || | | |_| |  _| | |_) |
//  | |_| || | |  _  | |___|  _ < 
//   \___/ |_| |_| |_|_____|_| \_\
//
//========================================================================================
// Other functions that used here

	int _add_content2table(Content* content) {
		for (int i = 0; i < CONTENT_TABLE_SIZE; i++) {
			if (_content_table[i] == NULL) {
				_content_table[i] = content;
				return i;
			}
		}

		return -1;
	}

	Content* _get_content_from_table(int ci) {
		return _content_table[ci];
	}

	int _remove_content_from_table(int index) {
		if (_content_table[index] == NULL) return -1;
		int result = FAT_unload_content_system(_content_table[index]);
		_content_table[index] = NULL;
		return result;
	}

	void _fatname2name(char* input, char* output) {
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

		output[12] = '\0';
		return;
	}

	char* _name2fatname(char* input) {
		str2uppercase(input);

		int haveExt = 0;
		char searchName[13] = { '\0' };
		uint16_t dotPos = 0;
		uint32_t counter = 0;

		while (counter <= 8) {
			if (input[counter] == '.' || input[counter] == '\0') {
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
			if (input[counter] != '\0' && haveExt == 1) searchName[extCount] = input[counter];
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

	uint16_t _current_time() {
		short data[7] = { 0 };
		get_datetime(data);
		return (data[2] << 11) | (data[1] << 5) | (data[0] / 2);
	}

	uint16_t _current_date() {
		short data[7] = { 0 };
		get_datetime(data);

		uint16_t reversed_data = 0;
		reversed_data |= data[3] & 0x1F;
		reversed_data |= (data[4] & 0xF) << 5;
		reversed_data |= ((data[5] - 1980) & 0x7F) << 9;

		return reversed_data;
	}

	int _name_check(const char* input) {
		short retVal = 0;
		uint16_t iterator = 0;
		for (iterator = 0; iterator < 11; iterator++) {
			if (input[iterator] < 0x20 && input[iterator] != 0x05) {
				retVal = retVal | BAD_CHARACTER;
			}
			
			switch (input[iterator]) {
				case 0x2E: {
					if ((retVal & NOT_CONVERTED_YET) == NOT_CONVERTED_YET) //a previous dot has already triggered this case
						retVal |= TOO_MANY_DOTS;

					retVal ^= NOT_CONVERTED_YET; //remove NOT_CONVERTED_YET flag if already set
					break;
				}

				case 0x22:
				case 0x2A:
				case 0x2B:
				case 0x2C:
				case 0x2F:
				case 0x3A:
				case 0x3B:
				case 0x3C:
				case 0x3D:
				case 0x3E:
				case 0x3F:
				case 0x5B:
				case 0x5C:
				case 0x5D:
				case 0x7C:
					retVal = retVal | BAD_CHARACTER;
			}

			if (input[iterator] >= 'a' && input[iterator] <= 'z') 
				retVal = retVal | LOWERCASE_ISSUE;
		}

		return retVal;
	}

	directory_entry_t* _create_entry(const char* name, const char* ext, int isDir, uint32_t firstCluster, uint32_t filesize) {
		directory_entry_t* data = (directory_entry_t*)_kmalloc(sizeof(directory_entry_t));

		data->reserved0 			 = 0; 
		data->creation_time_tenths 	 = 0;
		data->creation_time 		 = 0;
		data->creation_date 		 = 0;
		data->last_modification_date = 0;

		char* file_name = (char*)_kmalloc(25);
		strcpy(file_name, name);
		if (ext) {
			strcat(file_name, ".");
			strcat(file_name, ext);
		}
		
		data->low_bits 	= firstCluster;
		data->high_bits = firstCluster >> 16;  

		if(isDir == 1) {
			data->file_size  = 0;
			data->attributes = FILE_DIRECTORY;
		} else {
			data->file_size  = filesize;
			data->attributes = FILE_ARCHIVE;
		}

		data->creation_date = _current_date();
		data->creation_time = _current_time();
		data->creation_time_tenths = _current_time();

		if (_name_check(file_name) != 0)
			_name2fatname(file_name);

		strncpy((char*)data->file_name, file_name, min(11, strlen(file_name)));
		_kfree(file_name);

		return data; 
	}

	Content* FAT_create_object(char* name, int is_directory, char* extension) {
		Content* content = FAT_create_content();
		if (strlen(name) > 11 || strlen(extension) > 4) {
			printf("Uncorrect name or ext lenght.\n");
			FAT_unload_content_system(content);
			return NULL;
		}
		
		if (is_directory) {
			content->directory = _create_directory();
			strncpy(content->directory->name, name, 12);
			content->meta = *_create_entry(name, NULL, 1, _cluster_allocate(), 0);
		}
		else {
			content->file = _create_file();
			strncpy(content->file->name, name, 8);
			strncpy(content->file->extension, extension, 4);
			content->meta = *_create_entry(name, extension, 0, _cluster_allocate(), 1);
		}

		return content;
	}

	Content* FAT_create_content() {
		Content* content = (Content*)_kmalloc(sizeof(Content));
		content->directory      = NULL;
		content->file           = NULL;
		content->parent_cluster = -1;
		return content;
	}

	Directory* _create_directory() {
		Directory* directory = (Directory*)_kmalloc(sizeof(Directory));
		directory->files        = NULL;
		directory->subDirectory = NULL;
		directory->next         = NULL;
		return directory;
	}

	File* _create_file() {
		File* file = (File*)_kmalloc(sizeof(File));
		file->next = NULL;
		file->data = NULL;
		return file;
	}

	int _unload_directory_system(Directory* directory) {
		if (directory == NULL) return -1;
		if (directory->files != NULL) _unload_file_system(directory->files);
		if (directory->subDirectory != NULL) _unload_directory_system(directory->subDirectory);
		if (directory->next != NULL) _unload_directory_system(directory->next);
		_kfree(directory);
		return 1;
	}

	int _unload_file_system(File* file) {
		if (file == NULL) return -1;
		if (file->next != NULL) _unload_file_system(file->next);
		if (file->data != NULL) _kfree(file->data);
		_kfree(file);
		return 1;
	}

	int FAT_unload_content_system(Content* content) {
		if (content == NULL) return -1;
		if (content->directory != NULL) _unload_directory_system(content->directory);
		if (content->file != NULL) _unload_file_system(content->file);
		
		_kfree(content);
		return 1;
	}	

//========================================================================================