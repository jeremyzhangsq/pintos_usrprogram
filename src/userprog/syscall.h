#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdio.h>
#include "../filesys/off_t.h"

void syscall_init (void);
int syscall_write(int fd, const void *buffer, unsigned size);
void syscall_exit(int status);
void verify(void *esp);
bool syscall_create (const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
void syscall_seek (int fd, unsigned position);
struct file* fd_to_file(int fd);
off_t syscall_filesize (int fd);
void syscall_close (int fd);
int syscall_read (int fd, void *buffer, unsigned length);
unsigned syscall_tell (int fd);
int syscall_open(const char *file);
#endif /* userprog/syscall.h */
