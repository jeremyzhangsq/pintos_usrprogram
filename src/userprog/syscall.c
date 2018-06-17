#include "../userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "../threads/thread.h"
#include "../threads/interrupt.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
//    printf("syscall!\n");
  int *esp = f->esp;
  switch(*esp){
    case SYS_WRITE:{
      write(*(esp+1),*(esp+2),*(esp+3));
      break;
    }
    case SYS_EXIT:{
      exit(*(esp+1));
        break;
    }
  }

}
int
write(int fd, const void *buffer, unsigned size){
  switch (fd){
    case 1:{
      putbuf(buffer,size);
      return size;
    }
  }
}

void
exit(int status){
  printf("%s: exit(%d)\n",thread_current()->name,status);
  thread_exit();
}
