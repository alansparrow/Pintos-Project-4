#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define CLOSE_ALL -1
void process_close_file (int fd);

void syscall_init (void);

#endif /* userprog/syscall.h */
