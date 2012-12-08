#include "filesys/cache.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"

void filesys_cache_init (void)
{
  list_init(&filesys_cache);
  lock_init(&filesys_cache_lock);
  filesys_cache_size = 0;
}

struct cache_entry* filesys_cache_block_get (block_sector_t sector,
					     bool dirty)
{
  lock_acquire(&filesys_cache_lock);
  struct cache_entry *c;
  struct list_elem *e;
  for (e = list_begin(&filesys_cache); e != list_end(&filesys_cache);
       e = list_next(e))
    {
      c = list_entry(e, struct cache_entry, elem);
      if (c->sector == sector)
	{
	  c->read = true;
	  c->dirty |= dirty;
	  c->accessed = true;
	  lock_release(&filesys_cache_lock);
	  return c;
	}
    }
  c = filesys_cache_block_evict(sector, dirty);
  if (!c)
    {
      PANIC("Not enough memory for buffer cache.");
    }
  lock_release(&filesys_cache_lock);
  return c;
}

struct cache_entry* filesys_cache_block_evict (block_sector_t sector,
					       bool dirty)
{
  struct cache_entry *c;
  if (filesys_cache_size < MAX_FILESYS_CACHE_SIZE)
    {
      filesys_cache_size++;
      c = malloc(sizeof(struct cache_entry));
      if (!c)
	{
	  return NULL;
	}
      list_push_back(&filesys_cache, &c->elem);
    }
  else
    {
      bool loop = true;
      while (loop)
	{
	  struct list_elem *e;
	  for (e = list_begin(&filesys_cache); e != list_end(&filesys_cache);
	       e = list_next(e))
	    {
	      c = list_entry(e, struct cache_entry, elem);
	      if (c->read)
		{
		  continue;
		}
	      if (c->accessed)
		{
		  c->accessed = false;
		}
	      else
		{
		  if (c->dirty)
		    {
		      block_write(fs_device, c->sector, &c->block);
		    }
		  loop = false;
		  break;
		}
	    }
	}
    }
  c->read = true;
  c->sector = sector;
  block_read(fs_device, sector, &c->block);
  c->dirty = dirty;
  c->accessed = true;
  return c;
}

void filesys_cache_write_to_disk (bool halt)
{
  lock_acquire(&filesys_cache_lock);
  struct list_elem *next, *e = list_begin(&filesys_cache);
  while (e != list_end(&filesys_cache))
    {
      next = list_next(e);
      struct cache_entry *c = list_entry(e, struct cache_entry, elem);
      if (c->dirty)
	{
	  block_write (fs_device, c->sector, &c->block);
	  c->dirty = false;
	}
      if (halt)
	{
	  list_remove(&c->elem);
	  free(c);
	}
      e = next;
    }
  lock_release(&filesys_cache_lock);
}
