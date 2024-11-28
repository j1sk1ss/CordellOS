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
		.ext_root_cluster = 0
	};

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

		_kfree(cluster_data);
		return 0;
	}

#pragma endregion

#pragma region [Cluster functions]

	int FAT_cluster_free(unsigned int cluster) {
		if (cluster == 0) return 1;
		return 0;
	}

	int FAT_set_cluster_free(unsigned int cluster) {
		return FAT_write(cluster, 0);
	}

	int FAT_cluster_end(unsigned int cluster, int fatType) {
		if ((cluster == END_CLUSTER_32 && FAT_data.fat_type == 32) ||
			(cluster == END_CLUSTER_16 && FAT_data.fat_type == 16) ||
			(cluster == END_CLUSTER_12 && FAT_data.fat_type == 12))
			return 1;

		return 0;
	}

	int FAT_set_cluster_end(unsigned int cluster, int fatType) {
		if (FAT_data.fat_type == 32) return FAT_write(cluster, END_CLUSTER_32);
		if (FAT_data.fat_type == 16) return FAT_write(cluster, END_CLUSTER_16);
		if (FAT_data.fat_type == 12) return FAT_write(cluster, END_CLUSTER_12);
		return -1;
	}

	int FAT_cluster_bad(unsigned int cluster, int fatType) {
		if ((cluster == BAD_CLUSTER_32 && FAT_data.fat_type == 32) ||
			(cluster == BAD_CLUSTER_16 && FAT_data.fat_type == 16) ||
			(cluster == BAD_CLUSTER_12 && FAT_data.fat_type == 12))
			return 1;

		return 0;
	}

	int FAT_set_cluster_bad(unsigned int cluster, int fatType) {
		if (FAT_data.fat_type == 32) return FAT_write(cluster, BAD_CLUSTER_32);
		if (FAT_data.fat_type == 16) return FAT_write(cluster, BAD_CLUSTER_16);
		if (FAT_data.fat_type == 12) return FAT_write(cluster, BAD_CLUSTER_12);
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

	int FAT_read(unsigned int clusterNum) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);
		if (FAT_data.fat_type == 32 || FAT_data.fat_type == 16) {
			unsigned int cluster_size = FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster;
			unsigned int fat_offset   = clusterNum * (FAT_data.fat_type == 16 ? 2 : 4);
			unsigned int fat_sector   = FAT_data.first_fat_sector + (fat_offset / cluster_size);
			unsigned int ent_offset   = fat_offset % cluster_size;
			
			uint8_t* cluster_data = ATA_read_sectors(fat_sector, FAT_data.sectors_per_cluster);
			if (cluster_data == NULL) {
				kprintf("[%s %i] Function FAT_read: Could not read sector that contains FAT32 table entry needed.\n", __FILE__, __LINE__);
				return -1;
			}

			unsigned int table_value = *(unsigned int*)&cluster_data[ent_offset];
			if (FAT_data.fat_type == 32) table_value &= 0x0FFFFFFF;

			_kfree(cluster_data);
			return table_value;
		}
		
		kprintf("[%s %i] Function FAT_read: Invalid FAT_data.fat_type value. The value was: %i\n", __FILE__, __LINE__, FAT_data.fat_type);
		return -1;
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

	int FAT_write(unsigned int clusterNum, unsigned int clusterVal) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);

		if (FAT_data.fat_type == 32 || FAT_data.fat_type == 16) {
			unsigned int cluster_size = FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster;
			unsigned int fat_offset   = clusterNum * (FAT_data.fat_type == 16 ? 2 : 4);
			unsigned int fat_sector   = FAT_data.first_fat_sector + (fat_offset / cluster_size);
			unsigned int ent_offset   = fat_offset % cluster_size;

			uint8_t* sector_data = ATA_read_sectors(fat_sector, FAT_data.sectors_per_cluster);
			if (sector_data == NULL) {
				kprintf("Function FAT_write: Could not read sector that contains FAT32 table entry needed.\n");
				return -1;
			}

			*(unsigned int*)&sector_data[ent_offset] = clusterVal;
			if (ATA_write_sectors(fat_sector, sector_data, FAT_data.sectors_per_cluster) != 1) {
				kprintf("Function FAT_write: Could not write new FAT32 cluster number to sector.\n");
				return -1;
			}

			_kfree(sector_data);
			return 0;
		}

		kprintf("Function FAT_write: Invalid FAT_data.fat_type value. The value was: %i\n", FAT_data.fat_type);
		return -1;
	}

//========================================================================================

#pragma endregion

#pragma region [FAT AD functions]

//========================================================================================
//      _    _     _     ___   ____    _  _____ _____ 
//     / \  | |   | |   / _ \ / ___|  / \|_   _| ____|
//    / _ \ | |   | |  | | | | |     / _ \ | | |  _|  
//   / ___ \| |___| |__| |_| | |___ / ___ \| | | |___ 
//  /_/   \_\_____|_____\___/ \____/_/   \_\_| |_____|
//
//========================================================================================
// This function allocates _kfree FAT cluster (FREE not mean empty. Allocated cluster _kfree in FAT table)

	unsigned int lastAllocatedCluster = SECTOR_OFFSET;
	
	unsigned int FAT_cluster_allocate() {
		unsigned int cluster = lastAllocatedCluster;
		unsigned int clusterStatus = FREE_CLUSTER_32;

		while (cluster < FAT_data.total_clusters) {
			clusterStatus = FAT_read(cluster);
			if (FAT_cluster_free(clusterStatus) == 1) {
				if (FAT_set_cluster_end(cluster, FAT_data.fat_type) == 0) {
					lastAllocatedCluster = cluster;
					return cluster;
				}
				else {
					kprintf("Function FAT_cluster_allocate: Error occurred with FAT_write, aborting operations...\n");
					return -1;
				}
			}
			else if (clusterStatus < 0) {
				kprintf("Function FAT_cluster_allocate: Error occurred with FAT_read, aborting operations...\n");
				return -1;
			}

			cluster++;
		}

		lastAllocatedCluster = 2;
		return -1;
	}

//========================================================================================
//   ____  _____    _    _     _     ___   ____    _  _____ _____ 
//  |  _ \| ____|  / \  | |   | |   / _ \ / ___|  / \|_   _| ____|
//  | | | |  _|   / _ \ | |   | |  | | | | |     / _ \ | | |  _|  
//  | |_| | |___ / ___ \| |___| |__| |_| | |___ / ___ \| | | |___ 
//  |____/|_____/_/   \_\_____|_____\___/ \____/_/   \_\_| |_____|
//
//========================================================================================
// This function deallocates clusters. Just mark them _kfree in FAT table

	int FAT_cluster_deallocate(const unsigned int cluster) {
		if (FAT_data.fat_type != 12 && FAT_data.fat_type != 16 && FAT_data.fat_type != 32) {
			kprintf("Function FAT_cluster_allocate: FAT_data.fat_type is not valid!\n");
			return -1;
		}

		unsigned int clusterStatus = FAT_read(cluster);
		if (FAT_cluster_free(clusterStatus) == 1) return 0;
		else if (clusterStatus < 0) {
			kprintf("Function FAT_cluster_deallocate: Error occurred with FAT_read, aborting operations...\n");
			return -1;
		}

		if (FAT_set_cluster_free(cluster) == 0) return 0;
		else {
			kprintf("Function FAT_cluster_deallocate: Error occurred with FAT_write, aborting operations...\n");
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
	uint8_t* FAT_cluster_read(unsigned int clusterNum) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);
		unsigned int start_sect = (clusterNum - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		uint8_t* cluster_data = ATA_read_sectors(start_sect, FAT_data.sectors_per_cluster);
		assert(cluster_data != NULL);
		return cluster_data;
	}

	// Read cluster data and stop reading when script meets one of stop values
	uint8_t* FAT_cluster_read_stop(unsigned int clusterNum, uint8_t* stop) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);
		unsigned int start_sect = (clusterNum - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		uint8_t* response = ATA_read_sectors_stop(start_sect, FAT_data.sectors_per_cluster, stop);
		assert(response != NULL);
		return response;
	}

	// Read cluster with seek
	uint8_t* FAT_cluster_readoff(unsigned int clusterNum, uint32_t offset) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);
		unsigned int start_sect = (clusterNum - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		uint8_t* cluster_data = ATA_readoff_sectors(start_sect, offset, FAT_data.sectors_per_cluster);
		assert(cluster_data != NULL);
		return cluster_data;
	}

	// Read cluster data and stop reading when script meets one of stop values with seek
	uint8_t* FAT_cluster_readoff_stop(unsigned int clusterNum, uint32_t offset, uint8_t* stop) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);
		unsigned int start_sect = (clusterNum - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
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

	int FAT_cluster_write(void* contentsToWrite, unsigned int clusterNum) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);
		unsigned int start_sect = (clusterNum - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		return ATA_write_sectors(start_sect, contentsToWrite, FAT_data.sectors_per_cluster);
	}

	int FAT_cluster_writeoff(void* contentsToWrite, unsigned int clusterNum, uint32_t offset, uint32_t size) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);
		unsigned int start_sect = (clusterNum - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		return ATA_writeoff_sectors(start_sect, contentsToWrite, FAT_data.sectors_per_cluster, offset, size);
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

	int FAT_copy_cluster2cluster(unsigned int source, unsigned int destination) {
		assert(source >= 2 && source < FAT_data.total_clusters);
		assert(destination >= 2 && destination < FAT_data.total_clusters);
		unsigned int first = (source - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		unsigned int second = (destination - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		return ATA_cpy_sectors2sectors(first, second, FAT_data.sectors_per_cluster);
	}

//========================================================================================
//   ____ _    _   _ ____ _____ _____ ____     ___ _____ _   _ _____ ____  
//  / ___| |  | | | / ___|_   _| ____|  _ \   / _ \_   _| | | | ____|  _ \ 
// | |   | |  | | | \___ \ | | |  _| | |_) | | | | || | | |_| |  _| | |_) |
// | |___| |__| |_| |___) || | | |___|  _ <  | |_| || | |  _  | |___|  _ < 
//  \____|_____\___/|____/ |_| |_____|_| \_\  \___/ |_| |_| |_|_____|_| \_\
//========================================================================================
//	Other functions for working with clusters

	int FAT_cluster_clear(unsigned int clusterNum) {
		assert(clusterNum >= 2 && clusterNum < FAT_data.total_clusters);

		uint8_t clear[FAT_data.sectors_per_cluster * SECTOR_SIZE];
		memset(clear, 0, FAT_data.sectors_per_cluster * SECTOR_SIZE);

		unsigned int start_sect = (clusterNum - 2) * (unsigned short)FAT_data.sectors_per_cluster + FAT_data.first_data_sector;
		return ATA_write_sectors(start_sect, (const uint8_t*)clear, FAT_data.sectors_per_cluster);
	}

	// Add allocated cluster to file
	void FAT_add_cluster2content(Content* content) {
		directory_entry_t content_meta;
		if (content->directory != NULL) content_meta = content->directory->directory_meta;
		else if (content->file != NULL) content_meta = content->file->file_meta;

		unsigned int cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
		while (FAT_cluster_end(cluster, FAT_data.fat_type) == 0) {
			assert(FAT_cluster_bad(cluster, FAT_data.fat_type) == 0);
			assert(cluster != -1 );
			cluster = FAT_read(cluster);
		}

		if (FAT_cluster_end(cluster, FAT_data.fat_type) == 1) {
			unsigned int newCluster = FAT_cluster_allocate();
			assert(FAT_cluster_bad(newCluster, FAT_data.fat_type) == 0);
			assert(FAT_write(cluster, newCluster) == 0);
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

	Directory* FAT_directory_list(const unsigned int cluster, unsigned char attributesToAdd, int exclusive) {
		Directory* currentDirectory = FAT_create_directory();
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);
		
		const uint8_t default_hidden_attributes = (FILE_HIDDEN | FILE_SYSTEM);
		uint8_t attributes_to_hide = default_hidden_attributes;
		if (exclusive == 0) attributes_to_hide &= (~attributesToAdd);
		else if (exclusive == 1) attributes_to_hide = (~attributesToAdd);

		uint8_t* cluster_data = FAT_cluster_read(cluster);
		currentDirectory->data_pointer = (void*)cluster_data;
		if (cluster_data == NULL) {
			kprintf("Function FAT_directory_list: FAT_cluster_read encountered an error. Aborting...\n");
			FAT_unload_directories_system(currentDirectory);
			return NULL;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		unsigned int meta_pointer_iterator_count = 0;
		while (1) {
			if (file_metadata->file_name[0] == ENTRY_END) break;

			else if (strncmp((char*)file_metadata->file_name, "..", 2) == 0 ||
					strncmp((char*)file_metadata->file_name, ".", 1) == 0) {
				file_metadata++;
				meta_pointer_iterator_count++;
			}

			else if (((file_metadata->file_name)[0] == ENTRY_FREE) || ((file_metadata->attributes & FILE_LONG_NAME) == FILE_LONG_NAME)) {	
				if (meta_pointer_iterator_count < FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster / sizeof(directory_entry_t) - 1) {
					file_metadata++;
					meta_pointer_iterator_count++;
				}
				else {
					unsigned int next_cluster = FAT_read(cluster);
					if (FAT_cluster_end(next_cluster, FAT_data.fat_type) == 1) break;
					else if (next_cluster < 0) {
						kprintf("Function FAT_directory_list: FAT_read encountered an error. Aborting...\n");
						FAT_unload_directories_system(currentDirectory);
						return NULL;
					}
					else {
						FAT_unload_directories_system(currentDirectory);
						return FAT_directory_list(next_cluster, attributesToAdd, exclusive);
					}
				}
			}

			else {
				if ((file_metadata->attributes & FILE_DIRECTORY) != FILE_DIRECTORY) {			
					File* file = FAT_create_file();
					file->file_meta = *file_metadata;

					char name[13] = { 0 };
					strcpy(name, (const char*)file_metadata->file_name);
					strncpy(file->name, (const char*)strtok(name, " "), 8);
					strncpy(file->extension, (const char*)strtok(NULL, " "), 4);

					if (currentDirectory->files == NULL) currentDirectory->files = file;
					else {
						File* current = currentDirectory->files;
						while (current->next != NULL) current = current->next;
						current->next = file;
					}
				}

				else {
					if ((file_metadata->attributes & FILE_DIRECTORY) == FILE_DIRECTORY) {
						Directory* upperDir = FAT_create_directory();
						upperDir->directory_meta = *file_metadata;

						char name[13] = { 0 };
						strcpy(name, (char*)file_metadata->file_name);
						strncpy(upperDir->name, strtok(name, " "), 11);
						
						if (currentDirectory->subDirectory == NULL) currentDirectory->subDirectory = upperDir;
						else {
							Directory* current = currentDirectory->subDirectory;
							while (current->next != NULL) current = current->next;
							current->next = upperDir;
						}
					}
				}

				file_metadata++;
				meta_pointer_iterator_count++;
			}
		}

		return currentDirectory;
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

	int FAT_directory_search(const char* filepart, const unsigned int cluster, directory_entry_t* file, unsigned int* entryOffset) {
		assert(cluster >= 2 && cluster < FAT_data.total_clusters);

		char searchName[13] = { 0 };
		strcpy(searchName, filepart);
		if (FAT_name_check(searchName) != 0)
			FAT_name2fatname(searchName);

		uint8_t* cluster_data = FAT_cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function FAT_directory_search: FAT_cluster_read encountered an error. Aborting...\n");
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		unsigned int meta_pointer_iterator_count = 0;
		while (1) {
			if (file_metadata->file_name[0] == ENTRY_END) break;

			else if (strncmp((char*)file_metadata->file_name, searchName, 11) != 0) {
				if (meta_pointer_iterator_count < FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster / sizeof(directory_entry_t) - 1) {
					file_metadata++;
					meta_pointer_iterator_count++;
				}

				else {
					int next_cluster = FAT_read(cluster);
					if (FAT_cluster_end(next_cluster, FAT_data.fat_type) == 1) break;

					else if (next_cluster < 0) {
						kprintf("Function FAT_directory_search: FAT_read encountered an error. Aborting...\n");
						_kfree(cluster_data);
						return -1;
					} else {
						_kfree(cluster_data);
						return FAT_directory_search(filepart, next_cluster, file, entryOffset);
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

	int FAT_directory_add(const unsigned int cluster, directory_entry_t* file_to_add) {
		uint8_t* cluster_data = FAT_cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function FAT_directory_add: FAT_cluster_read encountered an error. Aborting...\n");
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		unsigned int meta_pointer_iterator_count = 0;
		while (1) {
			if (file_metadata->file_name[0] != ENTRY_FREE && file_metadata->file_name[0] != ENTRY_END) {
				if (meta_pointer_iterator_count < FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster / sizeof(directory_entry_t) - 1) {
					file_metadata++;
					meta_pointer_iterator_count++;
				}
				else {
					unsigned int next_cluster = FAT_read(cluster);
					if (FAT_cluster_end(next_cluster, FAT_data.fat_type) == 1) {
						next_cluster = FAT_cluster_allocate();
						if (FAT_cluster_bad(next_cluster, FAT_data.fat_type) == 1) {
							kprintf("Function FAT_directory_add: allocation of new cluster failed. Aborting...\n");
							_kfree(cluster_data);
							return -1;
						}

						if (FAT_write(cluster, next_cluster) != 0) {
							kprintf("Function FAT_directory_add: extension of the cluster chain with new cluster failed. Aborting...\n");
							_kfree(cluster_data);
							return -1;
						}
					}

					_kfree(cluster_data);
					return FAT_directory_add(next_cluster, file_to_add);
				}
			}
			else {
				file_to_add->creation_date 			= FAT_current_date();
				file_to_add->creation_time 			= FAT_current_time();
				file_to_add->creation_time_tenths 	= FAT_current_time_temths();
				file_to_add->last_accessed 			= file_to_add->creation_date;
				file_to_add->last_modification_date = file_to_add->creation_date;
				file_to_add->last_modification_time = file_to_add->creation_time;

				unsigned int new_cluster = FAT_cluster_allocate();
				if (FAT_cluster_bad(new_cluster, FAT_data.fat_type) == 1) {
					kprintf("Function FAT_directory_add: allocation of new cluster failed. Aborting...\n");
					_kfree(cluster_data);

					return -1;
				}
				
				file_to_add->low_bits  = GET_ENTRY_LOW_BITS(new_cluster, FAT_data.fat_type);
				file_to_add->high_bits = GET_ENTRY_HIGH_BITS(new_cluster, FAT_data.fat_type);

				memcpy(file_metadata, file_to_add, sizeof(directory_entry_t));
				if (FAT_cluster_write(cluster_data, cluster) != 0) {
					kprintf("Function FAT_directory_add: Writing new directory entry failed. Aborting...\n");
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

	int FAT_directory_edit(const unsigned int cluster, directory_entry_t* oldMeta, directory_entry_t* newMeta) {
		if (FAT_name_check((char*)oldMeta->file_name) != 0) {
			kprintf("Function FAT_directory_edit: Invalid file name!");
			return -1;
		}

		uint8_t* cluster_data = FAT_cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function FAT_directory_edit: FAT_cluster_read encountered an error. Aborting...\n");
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		unsigned int meta_pointer_iterator_count = 0;
		while (1) {
			if (strstr((char*)file_metadata->file_name, (char*)oldMeta->file_name) == 0) {

				// Correct new_meta data
				oldMeta->last_accessed 		 	= FAT_current_date();
				oldMeta->last_modification_date = FAT_current_date();
				oldMeta->last_modification_time = FAT_current_time_temths();

				memset(oldMeta->file_name, ' ', 11);
				strncpy((char*)oldMeta->file_name, (char*)newMeta->file_name, 11);

				memcpy(file_metadata, newMeta, sizeof(directory_entry_t));
				if (FAT_cluster_write(cluster_data, cluster) != 0) {
					kprintf("Function FAT_directory_edit: Writing updated directory entry failed. Aborting...\n");
					_kfree(cluster_data);
					return -1;
				}

				return 0;
			} 
			
			else if (meta_pointer_iterator_count < FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster / sizeof(directory_entry_t) - 1)  {
				file_metadata++;
				meta_pointer_iterator_count++;
			} 
			
			else {
				unsigned int next_cluster = FAT_read(cluster);
				if ((next_cluster >= END_CLUSTER_32 && FAT_data.fat_type == 32) || (next_cluster >= END_CLUSTER_16 && FAT_data.fat_type == 16) || (next_cluster >= END_CLUSTER_12 && FAT_data.fat_type == 12)) {
					kprintf("Function FAT_directory_edit: End of cluster chain reached. File not found. Aborting...\n");
					_kfree(cluster_data);
					return -2;
				}

				_kfree(cluster_data);
				return FAT_directory_edit(next_cluster, oldMeta, newMeta);
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

	int FAT_directory_remove(const unsigned int cluster, const char* fileName) {
		if (FAT_name_check(fileName) != 0) {
			kprintf("Function FAT_directory_remove: Invalid file name!");
			return -1;
		}

		uint8_t* cluster_data = FAT_cluster_read(cluster);
		if (cluster_data == NULL) {
			kprintf("Function FAT_directory_remove: FAT_cluster_read encountered an error. Aborting...\n");
			return -1;
		}

		directory_entry_t* file_metadata = (directory_entry_t*)cluster_data;
		unsigned int meta_pointer_iterator_count = 0;
		while (1) {
			if (strstr((char*)file_metadata->file_name, fileName) == 0) {
				file_metadata->file_name[0] = ENTRY_FREE;
				if (FAT_cluster_write(cluster_data, cluster) != 0) {
					kprintf("Function FAT_directory_remove: Writing updated directory entry failed. Aborting...\n");
					_kfree(cluster_data);
					return -1;
				}

				return 0;
			} 
			
			else if (meta_pointer_iterator_count < FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster / sizeof(directory_entry_t) - 1)  {
				file_metadata++;
				meta_pointer_iterator_count++;
			} 
			
			else {
				unsigned int next_cluster = FAT_read(cluster);
				if ((next_cluster >= END_CLUSTER_32 && FAT_data.fat_type == 32) || (next_cluster >= END_CLUSTER_16 && FAT_data.fat_type == 16) || (next_cluster >= END_CLUSTER_12 && FAT_data.fat_type == 12)) {
					kprintf("Function FAT_directory_remove: End of cluster chain reached. File not found. Aborting...\n");
					_kfree(cluster_data);
					return -2;
				}

				_kfree(cluster_data);
				return FAT_directory_remove(next_cluster, fileName);
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

	int FAT_content_exists(const char* filePath) {
		char fileNamePart[256];
		unsigned short start = 0;
		unsigned int active_cluster;

		if (FAT_data.fat_type == 32) active_cluster = FAT_data.ext_root_cluster;
		else {
			kprintf("Function FAT_content_exists: FAT16 and FAT12 are not supported!\n");
			return -1;
		}

		directory_entry_t file_info;
		for (unsigned int iterator = 0; iterator <= strlen(filePath); iterator++) {
			if (filePath[iterator] == '\\' || filePath[iterator] == '\0') {
				memset(fileNamePart, '\0', 256);
				memcpy(fileNamePart, filePath + start, iterator - start);

				int retVal = FAT_directory_search(fileNamePart, active_cluster, &file_info, NULL);
				if (retVal == -2) return 0;
				else if (retVal == -1) {
					kprintf("Function FAT_content_exists: An error occurred in FAT_directory_search. Aborting...\n");
					return -1;
				}

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

	Content* FAT_get_content(const char* filePath) {
		Content* fatContent = (Content*)_kmalloc(sizeof(Content));

		fatContent->directory = NULL;
		fatContent->file 	  = NULL;

		char fileNamePart[256] = { 0 };
		unsigned short start = 0;
		unsigned int active_cluster = 0;

		if (FAT_data.fat_type == 32) active_cluster = FAT_data.ext_root_cluster;
		else {
			kprintf("Function FAT_get_content: FAT16 and FAT12 are not supported!\n");
			FAT_unload_content_system(fatContent);
			return NULL;
		}
		
		directory_entry_t content_meta;
		for (unsigned int iterator = 0; iterator <= strlen(filePath); iterator++) 
			if (filePath[iterator] == '\\' || filePath[iterator] == '\0') {
				memset(fileNamePart, '\0', 256);
				memcpy(fileNamePart, filePath + start, iterator - start);

				int retVal = FAT_directory_search(fileNamePart, active_cluster, &content_meta, NULL);
				if (retVal == -2) {
					FAT_unload_content_system(fatContent);
					return NULL;
				}

				else if (retVal == -1) {
					kprintf("Function FAT_get_content: An error occurred in FAT_directory_search. Aborting...\n");
					FAT_unload_content_system(fatContent);
					return NULL;
				}

				start = iterator + 1;
				active_cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
				if (filePath[iterator] != '\0') fatContent->parent_cluster = active_cluster;
			}
		
		if ((content_meta.attributes & FILE_DIRECTORY) != FILE_DIRECTORY) {
			fatContent->file 		       = (File*)_kmalloc(sizeof(File));
			fatContent->file->next         = NULL;
			fatContent->file->data_pointer = NULL;

			uint32_t* content = NULL;
			int content_size = 0;
			
			int cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
			while (cluster < END_CLUSTER_32) {
				uint32_t* new_content = (uint32_t*)_krealloc(content, (content_size + 1) * sizeof(uint32_t));
				if (new_content == NULL) {
					_kfree(content);
					return NULL;
				}

				new_content[content_size] = cluster;

				content = new_content;
				content_size++;

				cluster = FAT_read(cluster);
				if (cluster == BAD_CLUSTER_32) {
					kprintf("Function FAT_get_content: the cluster chain is corrupted with a bad cluster. Aborting...\n");
					_kfree(content);
					return NULL;
				} 
				
				else if (cluster == -1) {
					kprintf("Function FAT_get_content: an error occurred in FAT_read. Aborting...\n");
					_kfree(content);
					return NULL;
				}
			}
			
			fatContent->file->data = (uint32_t*)_kmalloc(content_size * sizeof(uint32_t));
			memcpy(fatContent->file->data, content, content_size * sizeof(uint32_t));
			fatContent->file->data_size = content_size;
			_kfree(content);

			fatContent->file->file_meta = content_meta;

			char name[13] = { 0 };
			strcpy(name, (char*)fatContent->file->file_meta.file_name);
			strncpy(fatContent->file->name, strtok(name, " "), 8);
			strncpy(fatContent->file->extension, strtok(NULL, " "), 4);
			return fatContent;
		}
		else {
			fatContent->directory = (Directory*)_kmalloc(sizeof(Directory)); 
			fatContent->directory->directory_meta 	= content_meta;
			fatContent->directory->files            = NULL;
			fatContent->directory->subDirectory     = NULL;
			fatContent->directory->next             = NULL;
			fatContent->directory->data_pointer     = NULL;
			
			char name[13] = { 0 };
			strcpy(name, (char*)content_meta.file_name);
			strncpy(fatContent->directory->name, strtok(name, " "), 12);
			return fatContent;
		}
	}

	char* FAT_read_content(Content* data) {
		int totalSize = FAT_data.sectors_per_cluster * SECTOR_SIZE * data->file->data_size;
		char* result  = (char*)_kmalloc(totalSize);
		memset(result, 0, totalSize);

		int offset = 0;
		for (int i = 0; i < data->file->data_size; i++) {
			uint8_t* content_part = FAT_cluster_read(data->file->data[i]);
			int size = SECTOR_SIZE * FAT_data.sectors_per_cluster;
			
			memcpy(result + offset, content_part, size);
			_kfree(content_part);

			offset += size;
		}
		
		return result;
	}

	char* FAT_read_content_stop(Content* data, uint8_t* stop) {
		char* result = NULL;

		int offset = 0;
		for (int i = 0; i < data->file->data_size; i++) {
			uint8_t* content_part = FAT_cluster_read_stop(data->file->data[i], stop);
			int size = SECTOR_SIZE * FAT_data.sectors_per_cluster;
			
			result = _krealloc(result, offset + size);
			memcpy(result + offset, content_part, size);
			_kfree(content_part);

			offset += size;
			if (stop[0] == STOP_SYMBOL) break;
		}
		
		return result;
	}

	// Function for reading part of file
	// data - content for reading
	// buffer - buffer data storage
	// offset - file seek
	// size - size of read data
	void FAT_read_content2buffer(Content* data, uint8_t* buffer, uint32_t offset, uint32_t size) {
		uint32_t data_seek     = offset % (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t cluster_seek  = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t data_position = 0;

		for (int i = cluster_seek; i < data->file->data_size && data_position < size; i++) {
			uint32_t copy_size = min(SECTOR_SIZE * FAT_data.sectors_per_cluster - data_seek, size - data_position);
			uint8_t* content_part = FAT_cluster_readoff(data->file->data[i], data_seek);	

			memcpy(buffer + data_position, content_part, copy_size);
			_kfree(content_part);
			
			data_position += copy_size;
			data_seek = 0;
		}
	}

	// Function for reading part of file
	// data - content for reading
	// buffer - buffer data storage
	// offset - file seek
	// size - size of read data
	// stop - value that will stop reading
	void FAT_read_content2buffer_stop(Content* data, uint8_t* buffer, uint32_t offset, uint32_t size, uint8_t* stop) {
		uint32_t data_seek     = offset % (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t cluster_seek  = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t data_position = 0;

		for (int i = cluster_seek; i < data->file->data_size && data_position < size; i++) {
			uint32_t copy_size = min(SECTOR_SIZE * FAT_data.sectors_per_cluster - data_seek, size - data_position);
			uint8_t* content_part = FAT_cluster_readoff_stop(data->file->data[i], data_seek, stop);

			memcpy(buffer + data_position, content_part, copy_size);
			_kfree(content_part);
			
			data_position += copy_size;
			data_seek = 0;

			if (stop[0] == STOP_SYMBOL) break;
		}
	}

	int FAT_ELF_execute_content(char* path, int argc, char* argv[], int type) {
		ELF32_program* program = ELF_read(path, type);

		int (*programEntry)(int, char* argv[]) = (int (*)(int, char* argv[]))(program->entry_point);
		if (programEntry == NULL) return 0;
		
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
	
	int FAT_write_content(Content* content, char* content_data) {

		//=====================
		// CONTENT META SAVING

			directory_entry_t content_meta;
			if (content->directory != NULL) content_meta = content->directory->directory_meta;
			else if (content->file != NULL) content_meta = content->file->file_meta;

		// CONTENT META SAVING
		//=====================
		// EDIT DATA
			
			unsigned int cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
			unsigned int dataLeftToWrite = strlen(content_data);

			while (cluster <= END_CLUSTER_32) {
				unsigned int dataWrite = 0;
				
				if (dataLeftToWrite >= FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster) dataWrite = FAT_data.bytes_per_sector * FAT_data.sectors_per_cluster + 1;
				else dataWrite = dataLeftToWrite;

				char* sector_data = (char*)_kmalloc(dataWrite + 1);
				memset(sector_data, 0, dataWrite + 1);
				strncpy(sector_data, content_data, dataWrite);

				content_data += dataWrite;

				if (FAT_cluster_bad(cluster, FAT_data.fat_type) == 1) {
					kprintf("Function FAT_write_content: the cluster chain is corrupted with a bad cluster. Aborting...\n");
					return -1;
				}
				else if (cluster == -1 ) {
					kprintf("Function FAT_write_content: an error occurred in FAT_read. Aborting...\n");
					return -1;
				}

				if (dataLeftToWrite > 0) FAT_add_cluster2content(content);
				else if (dataLeftToWrite <= 0) {
					unsigned int prevCluster = cluster;
					unsigned int endCluster  = cluster;
					while (cluster < END_CLUSTER_32) {
						prevCluster = cluster;
						cluster = FAT_read(cluster);
						
						if (FAT_cluster_bad(cluster, FAT_data.fat_type) == 1) {
							kprintf("Function FAT_write_content: allocation of new cluster failed. Aborting...\n");
							return -1;
						}

						if (FAT_cluster_deallocate(prevCluster) != 0) {
							kprintf("Deallocation problems.\n");
							break;
						}
					}
					
					if (FAT_cluster_end(endCluster, FAT_data.fat_type) != 1) 
						FAT_set_cluster_end(endCluster, FAT_data.fat_type);
					
					break;
				}

				uint8_t* previous_data = FAT_cluster_read(cluster);
				if (memcmp(previous_data, sector_data, SECTOR_SIZE * FAT_data.sectors_per_cluster) != 0) {
					FAT_cluster_clear(cluster);
					if (FAT_cluster_write(sector_data, cluster) != 0) {
						kprintf("Function FAT_write_content: FAT_cluster_write encountered an error. Aborting...\n");
						_kfree(previous_data);
						return -1;
					}

					_kfree(previous_data);
				}

				dataLeftToWrite -= dataWrite;
				if (dataLeftToWrite == 0) FAT_set_cluster_end(cluster, FAT_data.fat_type);
				
				if (dataLeftToWrite < 0) {
					kprintf("Function FAT_write_content: An undefined value has been detected. Aborting...\n");
					return -1;
				}
			}

			return 0;
		
		// EDIT DATA
		//=====================

		return 0;
	}

	// Write data to content with offset from buffer
	// data - content where data will be placed
	// buffer - data that will be saved in content
	// offset - content seek
	// size - write size
	void FAT_write_buffer2content(Content* data, uint8_t* buffer, uint32_t offset, uint32_t size) {
		uint32_t cluster_seek = offset / (FAT_data.sectors_per_cluster * SECTOR_SIZE);
		uint32_t data_position = 0;
		uint32_t cluster_position = 0;
		uint32_t prev_offset = offset;

		// Write to presented clusters
		for (cluster_position = cluster_seek; cluster_position < data->file->data_size && data_position < size; cluster_position++) {
			uint32_t write_size = min(size - data_position, FAT_data.sectors_per_cluster * SECTOR_SIZE);
			FAT_cluster_writeoff(buffer + data_position, data->file->data[cluster_position], offset, write_size);

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
			FAT_add_cluster2content(data);

			// Write to allocated cluster
			FAT_write_buffer2content(data, new_buffer, new_offset, new_size);
		}
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
	int FAT_change_meta(const char* filePath, directory_entry_t* newMeta) {

		char fileNamePart[256];
		unsigned short start = 0;
		unsigned int active_cluster;
		unsigned int prev_active_cluster;

		//////////////////////
		//	FAT ACTIVE CLUSTER CHOOSING

			if (FAT_data.fat_type == 32) active_cluster = FAT_data.ext_root_cluster;
			else {
				kprintf("Function FAT_change_meta: FAT16 and FAT12 are not supported!\n");
				return -1;
			}

		//	FAT ACTIVE CLUSTER CHOOSING
		//////////////////////

		//////////////////////
		//	FINDING DIR BY PATH

			directory_entry_t file_info; //holds found directory info
			if (strlen(filePath) == 0) { // Create main dir if it not created (root dir)
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
				for (unsigned int iterator = 0; iterator <= strlen(filePath); iterator++) 
					if (filePath[iterator] == '\\' || filePath[iterator] == '\0') {
						prev_active_cluster = active_cluster;

						memset(fileNamePart, '\0', 256);
						memcpy(fileNamePart, filePath + start, iterator - start);

						int retVal = FAT_directory_search(fileNamePart, active_cluster, &file_info, NULL);
						switch (retVal) {
							case -2:
								kprintf("Function FAT_change_meta: No matching directory found. Aborting...\n");
							return -2;

							case -1:
								kprintf("Function FAT_change_meta: An error occurred in FAT_directory_search. Aborting...\n");
							return retVal;
						}

						start = iterator + 1;
						active_cluster = GET_CLUSTER_FROM_ENTRY(file_info, FAT_data.fat_type); //prep for next search
					}
			}

		//	FINDING DIR\FILE BY PATH
		//////////////////////

		//////////////////////
		// EDIT DATA

			if (FAT_directory_edit(prev_active_cluster, &file_info, newMeta) != 0) {
				kprintf("Function FAT_change_meta: FAT_directory_edit encountered an error. Aborting...\n");
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

	int FAT_put_content(const char* filePath, Content* content) {

		//====================
		// CONTENT META SAVING

			directory_entry_t content_meta;
			if (content->directory != NULL) content_meta = content->directory->directory_meta;
			else if (content->file != NULL) content_meta = content->file->file_meta;

		// CONTENT META SAVING
		//====================
		//	FINDING DIR BY PATH

			Content* directory = FAT_get_content(filePath);
			if (directory->directory == NULL) return -1;

			directory_entry_t file_info = directory->directory->directory_meta;
			unsigned int active_cluster = GET_CLUSTER_FROM_ENTRY(file_info, FAT_data.fat_type);
			FAT_unload_content_system(directory);

		//	FINDING DIR\FILE BY PATH
		//====================
		// CHECK IF FILE EXIST
		// A.i.: directory to receive the file is now found, and its cluster 
		// is stored in active_cluster. Search the directory to ensure the 
		// specified file name is not already in use
		
			char output[13];
			FAT_fatname2name((char*)content_meta.file_name, output);
			int retVal = FAT_directory_search(output, active_cluster, NULL, NULL);
			if (retVal == -1) {
				kprintf("Function putFile: directorySearch encountered an error. Aborting...\n");
				return -1;
			}
			else if (retVal != -2) {
				kprintf("Function putFile: a file matching the name given already exists. Aborting...\n");
				return -3;
			}

		// CHECK IF FILE EXIST
		//====================

		if (FAT_directory_add(active_cluster, &content_meta) != 0) {
			kprintf("Function FAT_put_content: FAT_directory_add encountered an error. Aborting...\n");
			return -1;
		}

		return 0; //file successfully written
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
// filePath - path where placed content
// name - name of content (if it file - with extension (if presented), like "test.txt")

	int FAT_delete_content(const char* path) {

		//====================
		// FIND CONTENT

			Content* fatContent = FAT_get_content(path);
			if (fatContent == NULL) {
				kprintf("Function FAT_delete_content: FAT_get_content encountered an error. Aborting...\n");
				return -1;
			}

		// FIND CONTENT
		//====================
		// CONTENT META SAVING

			directory_entry_t content_meta;
			if (fatContent->directory != NULL) content_meta = fatContent->directory->directory_meta;
			else if (fatContent->file != NULL) content_meta = fatContent->file->file_meta;

		// CONTENT META SAVING
		//====================
		// DELETE DATA

			unsigned int data_cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
			unsigned int prev_cluster = 0;
			
			while (data_cluster < END_CLUSTER_32) {
				prev_cluster = FAT_read(data_cluster);
				if (FAT_cluster_deallocate(data_cluster) != 0) {
					kprintf("[%s %i] FAT_cluster_deallocate encountered an error. Aborting...\n", __FILE__, __LINE__);
					FAT_unload_content_system(fatContent);
					return -1;
				}

				data_cluster = prev_cluster;
			}

			if (FAT_directory_remove(fatContent->parent_cluster, (char*)content_meta.file_name) != 0) {
				kprintf("[%s %i] FAT_directory_remove encountered an error. Aborting...\n", __FILE__, __LINE__);
				FAT_unload_content_system(fatContent);
				return -1;
			}
		
		// DELETE DATA
		//====================

		FAT_unload_content_system(fatContent);
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
		Content* fatContent = FAT_get_content(source);
		Content* dstContent = NULL;

		directory_entry_t content_meta;
		directory_entry_t dst_meta;
		if (fatContent->directory != NULL) {
			content_meta = fatContent->directory->directory_meta;
			dstContent   = FAT_create_object(fatContent->directory->name, 1, NULL);

			dst_meta = dstContent->directory->directory_meta;
		}
		else if (fatContent->file != NULL) {
			content_meta = fatContent->file->file_meta;
			dstContent   = FAT_create_object(fatContent->file->name, 0, fatContent->file->extension);

			dst_meta = dstContent->file->file_meta;
		}

		unsigned int data_cluster = GET_CLUSTER_FROM_ENTRY(content_meta, FAT_data.fat_type);
		unsigned int dst_cluster  = GET_CLUSTER_FROM_ENTRY(dst_meta, FAT_data.fat_type);

		while (data_cluster < END_CLUSTER_32) {
			FAT_add_cluster2content(dstContent);
			dst_cluster = FAT_read(dst_cluster);

			FAT_copy_cluster2cluster(data_cluster, dst_cluster);

			data_cluster = FAT_read(data_cluster);
		}

		FAT_put_content(destination, dstContent);
		FAT_unload_content_system(fatContent);
		FAT_unload_content_system(dstContent);
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

	void FAT_fatname2name(char* input, char* output) {
		if (input[0] == '.') {
			if (input[1] == '.') {
				strcpy (output, "..");
				return;
			}

			strcpy (output, ".");
			return;
		}

		unsigned short counter = 0;
		for ( counter = 0; counter < 8; counter++) {
			if (input[counter] == 0x20) {
				output[counter] = '.';
				break;
			}

			output[counter] = input[counter];
		}

		if (counter == 8) 
			output[counter] = '.';

		unsigned short counter2 = 8;
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

	char* FAT_name2fatname(char* input) {
		str2uppercase(input);

		int haveExt = 0;
		char searchName[13] = { '\0' };
		unsigned short dotPos = 0;

		unsigned int counter = 0;
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
		
		unsigned short extCount = 8;
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

	unsigned short FAT_current_time() {
		short data[7];
		get_datetime(data);
		return (data[2] << 11) | (data[1] << 5) | (data[0] / 2);
	}

	unsigned short FAT_current_date() {
		short data[7];
		get_datetime(data);

		uint16_t reversed_data = 0;

		reversed_data |= data[3] & 0x1F;
		reversed_data |= (data[4] & 0xF) << 5;
		reversed_data |= ((data[5] - 1980) & 0x7F) << 9;

		return reversed_data;
	}

	unsigned char FAT_current_time_temths() {
		short data[7];
		get_datetime(data);
		return (data[2] << 11) | (data[1] << 5) | (data[0] / 2);
	}

	int FAT_name_check(const char* input) {
		short retVal = 0;
		unsigned short iterator;
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

	unsigned char FAT_check_sum(unsigned char *pFcbName) {
		short FcbNameLen;
		unsigned char Sum;
		Sum = 0;
		for (FcbNameLen = 11; FcbNameLen != 0; FcbNameLen--) 
			Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;

		return (Sum);
	}

	directory_entry_t* FAT_create_entry(const char* name, const char* ext, int isDir, uint32_t firstCluster, uint32_t filesize) {
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

		data->creation_date = FAT_current_date();
		data->creation_time = FAT_current_time();
		data->creation_time_tenths = FAT_current_time_temths();

		if (FAT_name_check(file_name) != 0)
			FAT_name2fatname(file_name);

		strncpy((char*)data->file_name, file_name, min(11, strlen(file_name)));
		_kfree(file_name);

		return data; 
	}

	Content* FAT_create_object(char* name, int directory, char* extension) {
		Content* content = FAT_create_content();
		if (strlen(name) > 11 || strlen(extension) > 4) {
			printf("Uncorrect name or ext lenght.\n");
			FAT_unload_content_system(content);
			return NULL;
		}
		
		if (directory == 1) {
			content->directory = FAT_create_directory();
			strncpy(content->directory->name, name, 12);
			content->directory->directory_meta = *FAT_create_entry(name, NULL, 1, FAT_cluster_allocate(), 0);
		}
		else {
			content->file = FAT_create_file();
			strncpy(content->file->name, name, 8);
			strncpy(content->file->extension, extension, 4);
			content->file->file_meta = *FAT_create_entry(name, extension, 0, FAT_cluster_allocate(), 1);
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

	Directory* FAT_create_directory() {
		Directory* directory = (Directory*)_kmalloc(sizeof(Directory));
		directory->files        = NULL;
		directory->subDirectory = NULL;
		directory->next         = NULL;
		directory->data_pointer = NULL;

		return directory;
	}

	File* FAT_create_file() {
		File* file = (File*)_kmalloc(sizeof(File));
		file->next         = NULL;
		file->data         = NULL;
		file->data_pointer = NULL;

		return file;
	}

	int FAT_unload_directories_system(Directory* directory) {
		if (directory == NULL) return -1;
		if (directory->files != NULL) FAT_unload_files_system(directory->files);
		if (directory->subDirectory != NULL) FAT_unload_directories_system(directory->subDirectory);
		if (directory->next != NULL) FAT_unload_directories_system(directory->next);
		if (directory->data_pointer != NULL) _kfree(directory->data_pointer);

		_kfree(directory);
		return 1;
	}

	int FAT_unload_files_system(File* file) {
		if (file == NULL) return -1;
		if (file->next != NULL) FAT_unload_files_system(file->next);
		if (file->data_pointer != NULL) _kfree(file->data_pointer);
		if (file->data != NULL) _kfree(file->data);

		_kfree(file);
		return 1;
	}

	int FAT_unload_content_system(Content* content) {
		if (content == NULL) return -1;
		if (content->directory != NULL) FAT_unload_directories_system(content->directory);
		if (content->file != NULL) FAT_unload_files_system(content->file);
		
		_kfree(content);
		return 1;
	}	

//========================================================================================