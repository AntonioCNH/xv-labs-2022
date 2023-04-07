// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKET_SIZE 13

struct {
  struct spinlock lock[BUCKET_SIZE];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[BUCKET_SIZE];
} bcache;


void
binit(void)
{

  for(int i = 0; i < BUCKET_SIZE; i++) {
    initlock(&bcache.lock[i], "bcache");
    bcache.head[i].next = 0x0;
  }

  // Create linked list of buffers
  for(int i = 0; i < NBUF; i++) {
    int index = i % BUCKET_SIZE;
    bcache.buf[i].next = bcache.head[index].next;
    bcache.head[index].next = &bcache.buf[i];
    initsleeplock(&bcache.buf[i].lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  int index = blockno % BUCKET_SIZE;
  
  struct buf *b;
  // Is the block already cached?
  struct buf *spare = 0x0;
  acquire(&bcache.lock[index]);
  for(b = bcache.head[index].next; b ; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      if(b->refcnt == 0) {
        b->valid = 0;
      }
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      return b;
    }
    if(b->refcnt == 0) {
      spare = b;
    }
  }
  if(spare) {
    spare->dev = dev;
    spare->blockno = blockno;
    spare->valid = 0;
    spare->refcnt = 1;
    release(&bcache.lock[index]);
    acquiresleep(&spare->lock);
    return spare;
  }
  release(&bcache.lock[index]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  struct buf *pre;
  for(int i = 0; i < BUCKET_SIZE; i++) {
    if(i != index) {
      acquire(&bcache.lock[i]);
      for(b = bcache.head[i].next, pre = &bcache.head[i]; b ; pre = b,b = b->next) {
        if(b->refcnt == 0) {
          acquire(&bcache.lock[index]);
          b->next = bcache.head[index].next;
          bcache.head[index].next = b;
          release(&bcache.lock[index]);
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          pre->next = b->next;
          release(&bcache.lock[i]);
          acquiresleep(&b->lock);
          return b;
        }
      }
      release(&bcache.lock[i]);
    }
  }

  // struct buf *new_buf = memset((void*)new_buf, 0, sizeof(struct buf));
  // acquire(&bcache.lock[index]);
  // new_buf->next = bcache.head[index].next;
  // bcache.head[index].next = new_buf;
  // new_buf->dev = dev;
  // new_buf->blockno = blockno;
  // new_buf->refcnt = 1;
  // new_buf->valid = 0;
  // release(&bcache.lock[index]);
  // return new_buf;
  panic("bget: no valid buf\n");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int index = b->blockno % BUCKET_SIZE;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  int index = b->blockno % BUCKET_SIZE;
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void
bunpin(struct buf *b) {
  int index = b->blockno % BUCKET_SIZE;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}


