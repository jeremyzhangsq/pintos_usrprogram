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





