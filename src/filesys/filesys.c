#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/cache.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
#include "threads/malloc.h"

#define ASCII_SLASH 47

/* Partition that contains the file system. */
struct block *fs_device;

struct dir* get_containing_dir (const char* path);
char* get_filename (const char* path);
static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  filesys_cache_init();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  filesys_cache_write_to_disk(true);
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool isdir) 
{
  block_sector_t inode_sector = 0;
  struct dir *dir = get_containing_dir(name);
  char* file_name = get_filename(name);
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, isdir)
                  && dir_add (dir, file_name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  free(file_name);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir* dir = get_containing_dir(name);
  char* file_name = get_filename(name);
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, file_name, &inode);
  dir_close (dir);
  free(file_name);

  if (!inode)
    {
      return NULL;
    }

  if (inode_is_dir(inode))
    {
      return dir_open(inode);
    }
  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir* dir = get_containing_dir(name);
  char* file_name = get_filename(name);
  bool success = dir != NULL && dir_remove (dir, file_name);
  dir_close (dir);
  free(file_name);

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

bool filesys_chdir (const char* name)
{
  struct dir* dir = get_containing_dir(name);
  char* file_name = get_filename(name);
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, file_name, &inode);
  dir_close (dir);
  free(file_name);

  dir = dir_open (inode);
  if (dir)
    {
      dir_close(thread_current()->cwd);
      thread_current()->cwd = dir;
      return true;
    }
  return false;
}

struct dir* get_containing_dir (const char* path)
{
  char s[strlen(path) + 1];
  memcpy(s, path, strlen(path) + 1);

  if (strlen(path) > 0 && s[strlen(path) - 1] == ASCII_SLASH)
    {
      return NULL;
    }

  struct dir* dir;
  if (s[0] == ASCII_SLASH || !thread_current()->cwd)
    {
      dir = dir_open_root();
    }
  else
    {
      dir = dir_reopen(thread_current()->cwd);
    }

  char *save_ptr, *next_token, *token = strtok_r(s, "/", &save_ptr);
  if (token)
    {
      next_token = strtok_r(NULL, "/", &save_ptr);
    }
  else
    {
      next_token = NULL;
    }
  while (next_token != NULL)
    {
      struct inode *inode;
      if (!dir_lookup(dir, token, &inode))
	{
	  return NULL;
	}
      if (inode_is_dir(inode))
	{
	  dir_close(dir);
	  dir = dir_open(inode);
	}
      else
	{
	  inode_close(inode);
	}
      token = next_token;
      next_token = strtok_r(NULL, "/", &save_ptr);
    }
  return dir;
}

char* get_filename (const char* path)
{
  char s[strlen(path) + 1];
  memcpy(s, path, strlen(path) + 1);

  char *token, *save_ptr, *prev_token = "";
  for (token = strtok_r(s, "/", &save_ptr); token != NULL;
       token = strtok_r (NULL, "/", &save_ptr))
    {
      prev_token = token;
    }
  char *file_name = malloc(strlen(prev_token) + 1);
  memcpy(file_name, prev_token, strlen(prev_token) + 1);
  return file_name;
}
