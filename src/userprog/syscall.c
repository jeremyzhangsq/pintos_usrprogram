#include "../userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "../threads/thread.h"
#include "../threads/interrupt.h"
#include "../threads/vaddr.h"
#include "pagedir.h"
#include "../filesys/filesys.h"
#include "../filesys/file.h"
#include "../filesys/off_t.h"
#include "../devices/input.h"

static void syscall_handler (struct intr_frame *);
static int cnt = 0;
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int *esp = f->esp;
  verify(esp);
  switch(*esp){
    case SYS_OPEN:{
      verify(esp+1);  // pass sc-bad-arg
      verify(*(esp+1));// pass open-null and open-bad-ptr
      f->eax = syscall_open(*(esp+1));
      break;
    }
    case SYS_WRITE:{
      verify(esp+1);
      verify(esp+2);
      verify(esp+3);
      f->eax = syscall_write(*(esp+1),*(esp+2),*(esp+3));
      break;
    }
    case SYS_READ:{
      verify(esp+1);
      verify(esp+2);
      verify(*(esp+2));
      verify(esp+3);
      f->eax = syscall_read(*(esp+1),*(esp+2),*(esp+3));
      break;
    }
    case SYS_EXIT:{
      verify(esp+1);  // pass sc-bad-arg
      syscall_exit(*(esp+1));
      break;
    }
    case SYS_CREATE:{
      verify(esp+1);  // pass sc-bad-arg
      verify(esp+2);  // pass sc-bad-arg
      verify(*(esp+1));// pass create-null and create-bad-ptr
      f->eax = syscall_create(*(esp+1),*(esp+2));
      break;
    }
    case SYS_REMOVE:{
      verify(esp+1);  // pass sc-bad-arg
      verify(*(esp+1));
      f->eax = syscall_remove (*(esp+1));
      break;
    }
    case SYS_SEEK:{
      verify(esp+1);  // pass sc-bad-arg
      verify(esp+2);  // pass sc-bad-arg
      syscall_seek(*(esp+1),*(esp+2));
      break;
    }
    case SYS_FILESIZE:{
      verify(esp+1);  // pass sc-bad-arg
      f->eax = syscall_filesize(*(esp+1));
      break;
    }
    case SYS_TELL:{
      verify(esp+1);  // pass sc-bad-arg
      f->eax = syscall_tell(*(esp+1));
      break;
    }
    case SYS_CLOSE:{
      verify(esp+1);  // pass sc-bad-arg
      syscall_close(*(esp+1));
      break;
    }
  }
}

void
verify(void *esp){
//  if(esp == NULL) syscall_exit(-1);
  if(is_kernel_vaddr(esp)) syscall_exit(-1);
  if(pagedir_get_page(thread_current()->pagedir,esp) == NULL) syscall_exit(-1);
}

struct file*
fd_to_file (int fd)
{
//  printf("-----fd-%d--max %d------\n",fd,thread_current()->fd);
  if(fd>=thread_current()->fd || fd < 0)
    syscall_exit(-1);
  return thread_current()->fdpairs[fd];
}
int
syscall_open(const char *file){
  int fd = -1;
  struct file* f= filesys_open (file);
  if(f){
    fd = thread_current()->fd;
    thread_current()->fdpairs[fd] = f;
    thread_current()->fd++;
  }
  return fd;
}

int
syscall_write(int fd, const void *buffer, unsigned size){
  switch (fd){
    case 1:{
      putbuf(buffer,size);
      return size;
    }
  }
}
int
syscall_read (int fd, void *buffer, unsigned length){
    if(fd == 0){
      int8_t *buffer = buffer;
      for(int i = 0;i<length;i++){
        buffer[i] = input_getc();
      }
    }
    else{
      struct file * f = fd_to_file(fd);
      if(f) return file_read(f,buffer,length);
      else syscall_exit(-1);
    }
}
bool
syscall_create (const char *file, unsigned initial_size){
    return filesys_create(file,initial_size);
}

bool
syscall_remove (const char *file){
  return filesys_remove(file);
}

void
syscall_seek (int fd, unsigned position){
  struct file * f = fd_to_file(fd);
  if (f)
    file_seek (f, position);
  else
    syscall_exit(-1);
}
off_t
syscall_filesize (int fd){
  struct file * f = fd_to_file(fd);
  if (f)
    return file_length (f);
  else
    syscall_exit(-1);
}
void
syscall_close(int fd){
  struct file* f = fd_to_file(fd);
  if(f){
    thread_current()->fd--;
    file_close(f);
  }


}

unsigned
syscall_tell (int fd){
  struct file * f = fd_to_file(fd);
  return file_tell (f);
}

void
syscall_exit(int status){
  printf("%s: exit(%d)\n",thread_current()->name,status);
  thread_exit();
}
