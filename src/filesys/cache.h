#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "devices/block.h"
#include "threads/synch.h"
#include <list.h>

#define MAX_FILESYS_CACHE_SIZE 128

struct list filesys_cache;
uint32_t filesys_cache_size;
struct lock filesys_cache_lock;

struct cache_entry {
  uint8_t block[BLOCK_SECTOR_SIZE];
  block_sector_t sector;
  bool dirty;
  bool accessed;
  bool read;
  struct list_elem elem;
};

void filesys_cache_init (void);
struct cache_entry* filesys_cache_block_get (block_sector_t sector,
					     bool dirty);
struct cache_entry* filesys_cache_block_evict (block_sector_t sector,
					       bool dirty);
void filesys_cache_write_to_disk (bool halt);
void set_cache_entry_read_flag (struct cache_entry *c);

#endif /* filesys/cache.h */
