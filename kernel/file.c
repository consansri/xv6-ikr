//
// Support functions for system calls that involve file descriptors.
//


#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/sleeplock.h"
#include "include/fat32.h"
#include "include/file.h"
#include "include/pipe.h"
#include "include/stat.h"
#include "include/proc.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/vm.h"

extern int console_input_disabled;
struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
  struct file *f;
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    memset(f, 0, sizeof(struct file));
  }
  #ifdef DEBUG
  printf("fileinit\n");
  #endif
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  push_off();
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      pop_off();
      return f;
    }
  }
  pop_off();
  return NULL;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  push_off();
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  pop_off();
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  push_off();
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    pop_off();
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  pop_off();

  if(ff.type == FD_PIPE){
    pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_ENTRY){
    eput(ff.ep);
  } else if (ff.type == FD_DEVICE) {
    // reenable console when a program lets go of direct uart access
    if(f->major == AUX_UART_DEV_ID)
      console_input_disabled = 0;

  }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_ENTRY){
    elock(f->ep);
    estat(f->ep, &st);
    eunlock(f->ep);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  switch (f->type) {
    case FD_PIPE:
        r = piperead(f->pipe, addr, n);
        break;
    case FD_DEVICE:
        if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
          return -1;
        r = devsw[f->major].read(1, addr, n);
        break;
    case FD_ENTRY:
        elock(f->ep);
          if((r = eread(f->ep, 1, addr, f->off, n)) > 0)
            f->off += r;
        eunlock(f->ep);
        break;
    default:
      panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_ENTRY){
    elock(f->ep);
    if (ewrite(f->ep, 1, addr, f->off, n) == n) {
      ret = n;
      f->off += n;
    } else {
      ret = -1;
    }
    eunlock(f->ep);
  } else {
    panic("filewrite");
  }

  return ret;
}

// Read from dir f.
// addr is a user virtual address.
int
dirnext(struct file *f, uint64 addr)
{
  struct proc *p = myproc();

  if(f->readable == 0 || !(f->ep->attribute & ATTR_DIRECTORY))
    return -1;

  struct dirent de;
  struct stat st;
  int count = 0;
  int ret;
  elock(f->ep);
  while ((ret = enext(f->ep, &de, f->off, &count)) == 0) {  // skip empty entry
    f->off += count * 32;
  }
  eunlock(f->ep);
  if (ret == -1)
    return 0;

  f->off += count * 32;
  estat(&de, &st);
  if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
    return -1;

  return 1;
}