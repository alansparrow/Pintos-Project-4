#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

#define MAX_ARGS 4

static void syscall_handler (struct intr_frame *);
int user_to_kernel_ptr(const void *vaddr);

int process_add_file (struct file *f);
struct file* process_get_file (int fd);
void process_close_file (int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int i, arg[MAX_ARGS];
  for (i = 0; i < MAX_ARGS; i++)
    {
      arg[i] = * ((int *) f->esp + i);
    }
  switch (arg[0])
    {
    case SYS_HALT:
      {
	halt(); 
	break;
      }
    case SYS_EXIT:
      {
	exit(arg[1]);
	break;
      }
    case SYS_EXEC:
      {
	arg[1] = user_to_kernel_ptr((const void *) arg[1]);
	exec((const char *) arg[1]); 
	break;
      }
    case SYS_WAIT:
      {
	wait(arg[1]);
	break;
      }
    case SYS_CREATE:
      {
	arg[1] = user_to_kernel_ptr((const void *) arg[1]);
	create((const char *)arg[1], (unsigned) arg[2]);
	break;
      }
    case SYS_REMOVE:
      {
	arg[1] = user_to_kernel_ptr((const void *) arg[1]);
	remove((const char *) arg[1]);
	break;
      }
    case SYS_OPEN:
      {
	arg[1] = user_to_kernel_ptr((const void *) arg[1]);
	open((const char *) arg[1]);
	break; 		
      }
    case SYS_FILESIZE:
      {
	filesize(arg[1]);
	break;
      }
    case SYS_READ:
      {
	arg[2] = user_to_kernel_ptr((const void *) arg[2]);
	read(arg[1], (void *) arg[2], (unsigned) arg[3]);
	break;
      }
    case SYS_WRITE:
      { 
	arg[2] = user_to_kernel_ptr((const void *) arg[2]);
	write(arg[1], (const void *) arg[2], (unsigned) arg[3]);
	break;
      }
    case SYS_SEEK:
      {
	seek(arg[1], (unsigned) arg[2]);
	break;
      } 
    case SYS_TELL:
      { 
	tell(arg[1]);
	break;
      }
    case SYS_CLOSE:
      { 
	close(arg[1]);
	break;
      } 
    }
}

void halt (void)
{
  shutdown_power_off();
}

void exit (int status)
{
  printf ("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

pid_t exec (const char *cmd_line)
{
  pid_t pid = process_execute(cmd_line);
  // Wait for pid to complete load here
  // If not successful, return -1
  return pid;
}

int wait (pid_t pid)
{
  return process_wait(pid);
}

// For all file system syscalls, use global file system lock

bool create (const char *file, unsigned initial_size)
{
  // Lock
  bool success = filesys_create(file, initial_size);
  // Unlock
  return success;
}

bool remove (const char *file)
{
  // Lock
  // Unlock
  return true;
}

int open (const char *file)
{
  // Lock
  // Add file to fd descriptor list
  struct file *f = filesys_open(file);
  int fd = process_add_file(f);
  // Unlock
  return fd;
}

int filesize (int fd)
{
  // Lock
  struct file *f = process_get_file(fd);
  int size = file_length(f);
  // Unlock
  return size;
}

int read (int fd, void *buffer, unsigned size)
{
  if (fd == STDIN_FILENO)
    {
      unsigned i;
      uint8_t* local_buffer = (uint8_t *) buffer;
      for (i = 0; i < size; i++)
	{
	  local_buffer[i] = input_getc();
	}
      return size;
    }
  // Lock
  struct file *f = process_get_file(fd);
  int bytes = file_read(f, buffer, size);
  // Unlock
  return bytes;
}

int write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT_FILENO)
    {
      putbuf(buffer, size);
      return size;
    }
  // Lock
  struct file *f = process_get_file(fd);
  int bytes = file_write(f, buffer, size);
  // Unlock
  return bytes;
}

void seek (int fd, unsigned position)
{
  // Lock
  struct file *f = process_get_file(fd);
  file_seek(f, position);
  // Unlock
}

unsigned tell (int fd)
{
  // Lock
  struct file *f = process_get_file(fd);
  off_t offset = file_tell(f);
  // Unlock
  return offset;
}

void close (int fd)
{
  struct file *f = process_get_file(fd);
  file_close(f);
  process_close_file(fd);
}

int user_to_kernel_ptr(const void *vaddr)
{
  if (!is_user_vaddr(vaddr))
    {
      thread_exit();
      return 0; // Should never reach here
    }
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
    {
      thread_exit();
      return 0; // Should never reach here
    }
  return (int) ptr;
}

int process_add_file (struct file *f)
{
  return 1;
}

struct file* process_get_file (int fd)
{
  return NULL;
}

void process_close_file (int fd)
{
  return;
}
