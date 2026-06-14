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

#define NBUCKETS 17

struct {
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.

  struct spinlock bucketlock[NBUCKETS];
  struct buf head[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i=0;i<NBUCKETS;i++){
    initlock(&bcache.bucketlock[i], "bcache");
  }

  // Create linked list of buffers
  for (int i=0;i<NBUCKETS;i++){
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  
  int T = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[T % NBUCKETS].next;
    b->prev = &bcache.head[T % NBUCKETS];
    initsleeplock(&b->lock, "buffer");
    bcache.head[T%NBUCKETS].next->prev = b;
    bcache.head[T%NBUCKETS].next = b;
    T++;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.bucketlock[blockno % NBUCKETS]);

  // Is the block already cached?
  for(b = bcache.head[blockno % NBUCKETS].next; b != &bcache.head[blockno % NBUCKETS]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucketlock[blockno % NBUCKETS]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  for (int i=0;i<NBUCKETS;i++){
    if (i != (blockno % NBUCKETS)){
      if (bcache.bucketlock[i].locked){
        continue;
      }
      acquire(&bcache.bucketlock[i]);
    }
    for (b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if (b->refcnt > 0) continue;
      b->prev->next = b->next;
      b->next->prev = b->prev;
      b->next = bcache.head[blockno % NBUCKETS].next;
      b->prev = &bcache.head[blockno % NBUCKETS];
      bcache.head[blockno % NBUCKETS].next->prev = b;
      bcache.head[blockno % NBUCKETS].next = b;
      b->dev = dev ;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      acquiresleep(&b->lock);
      if (i != (blockno % NBUCKETS)) release(&bcache.bucketlock[i]);
      release(&bcache.bucketlock[blockno % NBUCKETS]);
      return b;
    }
    if (i != (blockno % NBUCKETS)) release(&bcache.bucketlock[i]);
  }
  
  panic("bget: no buffers");
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
  uint blockno = b->blockno;

  acquire(&bcache.bucketlock[blockno % NBUCKETS]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[blockno % NBUCKETS].next;
    b->prev = &bcache.head[blockno % NBUCKETS];
    bcache.head[blockno % NBUCKETS].next->prev = b;
    bcache.head[blockno % NBUCKETS].next = b;
  }
  
  release(&bcache.bucketlock[blockno % NBUCKETS]);
}

void
bpin(struct buf *b) {
  uint blockno = b->blockno;
  acquire(&bcache.bucketlock[blockno % NBUCKETS]);
  b->refcnt++;
  release(&bcache.bucketlock[blockno % NBUCKETS]);
}

void
bunpin(struct buf *b) {
  uint blockno = b->blockno;
  acquire(&bcache.bucketlock[blockno % NBUCKETS]);
  b->refcnt--;
  release(&bcache.bucketlock[blockno % NBUCKETS]);
}


