#include "../userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include <string.h>
#include "../threads/thread.h"
#include "../threads/interrupt.h"
#include "../threads/vaddr.h"
#include "pagedir.h"
#include "../filesys/filesys.h"
#include "../filesys/file.h"
#include "../filesys/off_t.h"
#include "../devices/input.h"
#include "../threads/synch.h"
#include "../devices/shutdown.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
static int cnt = 0;
// add lock for filesystem
static struct lock filesys_lock;
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  //  init lock for file system
  lock_init(&filesys_lock);
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
      verify(*(esp+2));
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
//      printf("------%d-------\n",f->eax);
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
    case SYS_HALT:{
      shutdown_power_off();
    }
    case SYS_WAIT:{
      f->eax = process_wait(*(esp+1));
      break;
    }
    case SYS_EXEC:{
//      verify(esp+1);  // pass sc-bad-arg
//      printf("-----------exec:%s-----------------------\n",*(esp+1));
      verify(*(esp+1));
      f->eax = syscall_exec(*(esp+1));
      break;
    }
  }
}
pid_t
syscall_exec(const char *cmd_line){
//  use file open check if father process is finished
//  printf("before execu\n");
  int id = process_execute(cmd_line);
  return id;

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
  if(fd>=thread_current()->fd || fd < 0)
    syscall_exit(-1);
  return thread_current()->fdpairs[fd];
}
int
syscall_open(const char *file){
  int fd = -1;
  lock_acquire(&filesys_lock);
  struct file* f= filesys_open (file);
  lock_release(&filesys_lock);
  if(f){
    fd = thread_current()->fd;
    thread_current()->fdpairs[fd] = f;
    thread_current()->fd++;
  }
  return fd;
}

int
syscall_write(int fd, const void *buffer, unsigned size){
  if (fd == 1){
      lock_acquire(&filesys_lock);
      putbuf(buffer,size);
      lock_release(&filesys_lock);
      return size;
  }
  else{
    struct file * f = fd_to_file(fd);
    if (f){
      lock_acquire(&filesys_lock);
      int res = file_write(f,buffer,size);
      lock_release(&filesys_lock);
      return res;
    } else return -1;
  }
}
int
syscall_read (int fd, void *buffer, unsigned length){
//  printf("%d %d\n",fd,length);
    if(fd == 0){
      uint8_t *buffer = (uint8_t *) buffer;
      for(int i = 0;i<length;i++){
        buffer[i] = input_getc();
      }
      return length;
    }
    struct file * f = fd_to_file(fd);
    if(f) {
        lock_acquire(&filesys_lock);
        int resu = file_read(f,buffer,length);
        lock_release(&filesys_lock);
        return resu;
    }
    else return 0;
}
bool
syscall_create (const char *file, unsigned initial_size){
    lock_acquire(&filesys_lock);
    bool res =  filesys_create(file,initial_size);
    lock_release(&filesys_lock);
    return res;
}

bool
syscall_remove (const char *file){
  lock_acquire(&filesys_lock);
  bool res = filesys_remove(file);
  lock_release(&filesys_lock);
}

void
syscall_seek (int fd, unsigned position){
  struct file * f = fd_to_file(fd);
  if (f){
    lock_acquire(&filesys_lock);
    file_seek (f, position);
    lock_release(&filesys_lock);
  }

  else
    syscall_exit(-1);
}
off_t
syscall_filesize (int fd){
  struct file * f = fd_to_file(fd);
  if (f){
    lock_acquire(&filesys_lock);
    off_t res = file_length (f);
    lock_release(&filesys_lock);
    return res;
  }

  else
    syscall_exit(-1);
}
void
syscall_close(int fd){
  struct file* f = fd_to_file(fd);
  if(f){
    thread_current()->fd--;
    lock_acquire(&filesys_lock);
    file_close(f);
    lock_release(&filesys_lock);
  }
}

unsigned
syscall_tell (int fd){
  struct file * f = fd_to_file(fd);
  lock_acquire(&filesys_lock);
  off_t res = file_tell (f);
  lock_release(&filesys_lock);
  return res;
}

void
syscall_exit(int status){
  printf("%s: exit(%d)\n",thread_current()->name,status);
  thread_current()->return_code = status;
  thread_exit();
}
