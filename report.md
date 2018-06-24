# PROJECT 2: USER PROGRAM
张诗奇 11510580

## TASK1: Argument Passing

### 整体思路

1. 字符串传递
将执行的程序和所需要传递的方法以字符串的形式传入`init.c`中的`run_task(char **argv)`中，进而将包含着程序名称和参数的字符串，传入`process_exectue()`方法。在`process_exectue()`方法中，首先将字符串中的程序名通过`string.h`中的`strtok_r (char *s, const char *delimiters, char **save_ptr)`方法解析出来，紧接着将解析出来的文件名和复制过的源字符串一起传入`start_process()`中的`load()`中。
2. 字符串解析
将整体cmd传入的整个字符串利用`strtok_r (char *s, const char *delimiters, char **save_ptr)`方法解析出来，并将每一个单字符串储存在数组中，方便后续内存分配，同时，将文件名传入filesystem中，以打开对应文件。
3. 内存分配
按照`PHY_BASE`和栈指针`*esp`依次将参数，参数栈地址，`argc`和`argv`压入栈中。
### 细节实现

- 对于`strtok_r ()`的理解

`string.h`中定义的方法`strtok_r (char *s, const char *delimiters, char **save_ptr)`对第一步和第二步的操作有很大的作用。该方法按照`char *delimiters`对字符串进行分词，在第一次使用的时候需要将源字符串`char* s`传入，分词结束后会将分词返回，并将剩余`char *`保存在`char **save_ptr`中。后续每一次分词只需要将`s`传入`NULL`并按照`save_ptr`找到剩余的字符串即可。在分词的过程中，该方法不仅将原字符串按照分隔符切割开，同时在每一个切分后的字符串加入`\0`标志。 

- 利用`strtok_r ()`解析字符串
  ```c
  //  parse args and store into array
  char* token,save_ptr;
  char* argv[MAX_ARGC],ptrs[MAX_ARGC];
  int argc =0;
  for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
    argv[argc] = token;
    ptrs[argc] = save_ptr;
    argc++;
  }
  ```
  我们首先定义一个可以接受的最大参数个数`MAX_ARGC = 100`。`char* argv[MAX_ARGC],ptrs[MAX_ARGC]`分别用来储存每一个参数和他们对应的地址。同时记录参数个数`argc`。
- 打开文件
  通过字符串解析过后`argv[0]`中存储的就是执行的程序名称，我们需要去pintos的文件系统中打开它。这里我们采用建立临时的方式来建立文件系统，并将对应的文件传入到这个临时文件系统中，并且传入我们的命令字符串。
  ```shell
  pintos -v -k -T 2 --bochs  --filesys-size=2 -p PATH/TO/FILE -a FILENAME -- -q  -f 'COMMAND LINE STRING'
  ```
   执行完导入文件操作之后，便可调用`load()`方法中的`filesys_open (file_name)`方法来打开文件并且执行后续检查文件header的工作。
- 内存分配
   在内存分配开始之前，首先程序会调用`setup_stack (esp)`方法来初始化一个栈并将栈底地址传给`esp`，具体的栈底地址是由`PHY_BASE`决定的。这里需要注意的是`esp`为`void **`类型，因此`*esp`才是真正的stack pointer。
   栈底的地址大，在压栈的过程中，地址是逐渐变小的，但是在每一次分配一块内存时，我们又是从小到大去分配的，因此压栈和内存分配的过程为，先将栈指针向下移动指定位置，然后再将当前`*esp`所在的位置到上一次`*esp`位置之间的内存进行分配。
   首先我们将`argv[]`中参数的具体值压入到栈中。 这里我们需要一个辅助的函数`int charsize(char *c)`来获得当前`char* c`的长度。由于题目要求占位符`\0`的长度也应被计入，因此长如需要格外加一。
   ```c
   int charsize(char *c){
    int i=0;
    while(c[i]!='\0') {
        i++;
    }
    return ++i;
   }
   ```
   整体的压栈过程为将`*esp`向下移动字符串大小个位置，并利用`memcpy`将`argv[i]`地址的`charsize(argv[i])`大小的内容复制到栈指针指向的位置，同时我们将当前字符串所在的地址（即栈顶指针）储存到`ptrs[i]`中，方便后续地址压栈操作。在这个过程中我们需要记录一下总共占用的地址字节数`offset`方便下一步内存对齐操作。
   ```c
   for(int cnt = argc-1;cnt>=0;cnt--){
    offset += charsize(argv[cnt]);
    *esp -= charsize(argv[cnt]);
    ptrs[cnt] = *esp;
    memcpy(*esp, argv[cnt], charsize(argv[cnt]));
   }
   ```
   紧接着我们来进行内存对齐操作，由于一个字节占八位，pintos为三十二位操作系统，因此我们用零将内存补齐到4的整数倍，当前内存多出来的字节为`offset%4`，那么需要补齐的字节为`4-offset%4`。
   ```c
   *esp -= (4 - offset%4);
   memset(*esp, 0, 4 - offset%4);
   ```
   由于在访问参数时使用`argc`，因此为了防止越界，我们也为`argv[argc]`分配一个空指针，并且存储到内存中。
   ```c
   *esp -= sizeof(char*);
   memset(*esp, 0, sizeof(char*));
   ```
   接下来，我们按照`ptrs[]`数组中的内容，将每一参数字符串在栈中的地址压入到栈中。这里我们需要注意`ptrs[]`是一个`char*`数组，我们所需要的地址为`cahr*`所指向地址里面的值，因此我们需要用`&ptrs[i]`得到当前`char*`指向的地址。
   ```c
   for(int cnt = argc-1; cnt>=0;cnt--){
      *esp -= sizeof(ptrs[cnt]);
      memcpy(*esp, &ptrs[cnt], sizeof(char*));
    }
   ```
   剩下的操作为将`char** argv`,`int argc`和`void(*)() return address`压入栈中。对于`char** argv`，我们需要获得写入`argv[0]`地址时`*esp`所在的位置，因此我们在移动栈指针之前先将其存一下。
   ```c
   void *temp = *esp;
   *esp -= sizeof(char**);
   memcpy(*esp, &temp, sizeof(char**));
   ```
   对于剩下的两个元素，分别将栈指针移动一个整数大小的位置和一个函数返回指针大小的位置，并将对应内容放到内存中即可。
   ```c
   *esp -= 4;
   memcpy(*esp, &argc, 4);
   *esp -= sizeof(void*);
   memset(*esp, NULL, sizeof(void *));
   ```
   
## TASK2: System Calling

### 原理解析：
在kernel space中，Pintos通过`int0x30`来处理system calls。具体流程是user space将参数压入栈并且执行`int0x30`，而kernal space会通过`syscall_handler()`方法处理来自user space的内容。例如：

```c
    #define syscall1(NUMBER, ARG0) ({                                     
          int retval;                                                    
          asm volatile                                                   
            ("pushl %[arg0]; pushl %[number]; int $0x30; addl $8, %%esp" 
               : "=a" (retval)                                           
               : [number] "i" (NUMBER),[arg0] "g" (ARG0)
               : "memory");                                              
          retval;                                                        
       })
```

`syscall_handler()`通过判断去到的stack pointer的值，进入switch-case，来执行不同的system call，对应system call的参数在接下来的stack pointer中。整个`syscall_handler()`如下所示

```c
    static void syscall_handler (struct intr_frame *f UNUSED) {
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
        case SYS_EXEC:{
          verify(esp+1);  // pass sc-bad-arg
          verify(*(esp+1));
          f->eax = syscall_exec(*(esp+1));
          break;
        }
      }
    }

```

在每个case中每当拿到`esp`都要检查他的合法性，检查对于传入的是指针参数，我们需要检查这个`esp`所指的地址是否合法，即`verify(*(esp))`,虽然我们是从stack中取出的地址，但是会有testcase故意移动当前的栈指针（例如test case: sc-bad-arg），因此我们也需要对每个传进来的参数`esp`进行验证。
```c
    void verify(void *esp){
    //  if(esp == NULL) syscall_exit(-1);
      if(is_kernel_vaddr(esp)) syscall_exit(-1);
      if(pagedir_get_page(thread_current()->pagedir,esp) == NULL) 	syscall_exit(-1);
    }
```
### Filesystem Call：
对于filesystem call可以分为三类，第一类为打开关闭操作，第二类为读写操作，第三类是其他轻量级操作。除了打开操作以外，user space都是只能拿到文件修饰符，并且对文件修饰符进行操作，然而我们在kernel中的文件系统API大多使用文件指针进行操作，因此我们需要建立一个文件修饰符和文件指针的映射。我们在thread类中，添加如下属性，用数组来的index表示文件修饰符，数组内容表示文件指针。

``` c
    struct file* fdpairs[MAX_FD];
    int fd;
```

具体对于数组和fd指针的更新会在打开关闭文件操作中使用，下面我们来看一下辅助找到对应文件指针的方法

```c
    struct file*
    fd_to_file (int fd)
    {
      if(fd>=thread_current()->fd || fd < 0)
        syscall_exit(-1);
      return thread_current()->fdpairs[fd];
    }
```

对于轻量级的操作，代码如下：

```c
    bool
    syscall_remove (const char *file){
      lock_acquire(&filesys_lock);
      bool res = filesys_remove(file);
      lock_release(&filesys_lock);
    }
```

```c
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
```

```c
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
```

```c
    unsigned
    syscall_tell (int fd){
      struct file * f = fd_to_file(fd);
      lock_acquire(&filesys_lock);
      off_t res = file_tell (f);
      lock_release(&filesys_lock);
      return res;
    }
```

这些轻量级的方法需要在`fd_to_file()`方法和锁的配合下，通过调用filesystem中的API实现。

对于打开和关闭文件：

```c
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
```

```c
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

```

我们需要注意，在打开文件的时候将当前文件指针保存到文件指针的数组中，同时更新文件修饰符的指针位置，以便下次插入文件指针，对于关闭文件时，记得将其文件指针后移，以表示删除原位置的文件指针。

对于读取文件和写入文件：

```c
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
```

```c
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
```

首先需要注意文件修饰符的的0和1分别对应写到控制台和从键盘读取，因此我们需要将这两种特殊情况拿出来考虑，从键盘读取是调用`input_getc()`实现一个字符一个字符读取，打印到控制台调用`putbuf(buffer,size)`。

### Process System Call：
在讲解process systemcall之前，需要注意在`process_execute()`方法中如果对字符串进行分词操作，需重新复制一份，因为原字符串会在别处使用。

```c
      char *fn_copy;
      char *fn_copy2;
      tid_t tid;

      fn_copy = palloc_get_page (0);
      fn_copy2 = palloc_get_page (0);
      if (fn_copy == NULL){
          return TID_ERROR;
      }

      strlcpy (fn_copy, file_name, PGSIZE);
      strlcpy (fn_copy2, file_name, PGSIZE);
      char *save_ptr;
      strtok_r (fn_copy2, " ", &save_ptr);

      /* Create a new thread to execute FILE_NAME. */
      tid = thread_create (fn_copy2, PRI_DEFAULT, start_process, fn_copy);
```

同时我们需要将`exception.c`的中的page_fault进行修改，

```c
    case SEL_UCSEG:
          /* User's code segment, so it's a user exception, as we
             expected.  Kill the user process.  */
          printf ("%s: dying due to interrupt %#04x (%s).\n",
                  thread_name (), f->vec_no, intr_name (f->vec_no));
          intr_dump_frame (f);
		  syscall_exit(-1);
```

process system call中需要实现执行和等待两个函数，其中，执行操作实现简单，仅需要调用`process_execute()`将对应进程加入等待队列即可，对于等待函数，是整个的难点，需要修改thread的数据结构并且加锁来进行同步。首先我们来看，对于thread的结构体有哪些修改。

``` c
	struct semaphore  childlock;
    struct semaphore  loadlock;
    bool load;
    int exist;
    struct list children;
    struct list_elem childelem;
    int return_code;	
```

`childlock`为实现父子进程间同步的信号量，在每个进程创建时初始化信号量为零，并在进程结束`process_exit()`时释放信号量，对于刚刚创立的子进程在未完成操作时，父进程的`sema_down`会被阻塞，直到子进程释放信号量。对于`loadlock`,我们需要确保子进程load结束后，父进程再进行相应操作，以防出现虽然子进程load失败，但是父进程却没有等他的情况，变量`load`用来标记当前子进程是否load成功，对于变量`exist`用来处理重复操作两次相同子进程的情况。 对于`children`这个链表，用来存储所有孩子进程，注意在子进程创建时入链，父进程等待子进程结束后，将其删除。

``` c
    int
    process_wait (tid_t child_tid UNUSED)
    {
        int pid = -1;
        struct thread *cthread = get_child_by_id(child_tid);
        if(list_empty(&thread_current()->children) || cthread==NULL){
            return pid;
        }
        if(cthread->tid == child_tid){
            sema_down(&cthread->childlock);
            pid = cthread->return_code;
            if(thread_current()->exist == pid)
                pid = -1;
            else
                thread_current()->exist = pid;
            list_remove(&cthread->childelem);
            sema_up(&cthread->childlock);
            return pid;
        }
        return pid;
    }
```

对于`loadlock`信号量的处理如下，在load结束后释放信号量，在进程执行的时候等待信号量释放。

```c
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;

  success = load (file_name, &if_.eip, &if_.esp);
  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success){
      thread_current()->return_code = -1;
      thread_current()->load = false;
      sema_up(&thread_current()->loadlock);
      thread_exit ();
  }
    thread_current()->load = true;
    sema_up(&thread_current()->loadlock);
  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}
```



```c
    tid_t
    process_execute (const char *file_name)
    {
      char *fn_copy;
      char *fn_copy2;
      tid_t tid;
      /* Make a copy of FILE_NAME.
         Otherwise there's a race between the caller and load(). */
      fn_copy = palloc_get_page (0);
      fn_copy2 = palloc_get_page (0);
      if (fn_copy == NULL){
          return TID_ERROR;
      }

      strlcpy (fn_copy, file_name, PGSIZE);
      strlcpy (fn_copy2, file_name, PGSIZE);
      char *save_ptr;
      strtok_r (fn_copy2, " ", &save_ptr);

      /* Create a new thread to execute FILE_NAME. */
      tid = thread_create (fn_copy2, PRI_DEFAULT, start_process, fn_copy);
      if (tid == TID_ERROR){
          palloc_free_page (fn_copy);
      }
        struct thread* cthread = get_child_by_id(tid);
        if(cthread){
            if(!cthread->load){
                cthread->tid = -1;
                sema_down(&cthread->loadlock);
                return -1;
            }
        }
      return tid;
    }
```



