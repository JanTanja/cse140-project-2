#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/

int calcTag(address addr, int tag_bit) {
  return addr >> tag_bit;
}

int calcIndex(address addr, int tag_bit, int offset_bit) {
  int tempIndex = addr << tag_bit;
  tempIndex = tempIndex >> (offset_bit + tag_bit);
  return tempIndex;
}

int calcOffset(address addr, int tag_bit, int offset_bit) {
  int tempOffset = addr << (offset_bit + tag_bit);
  tempOffset = tempOffset >> (offset_bit + tag_bit);
  return tempOffset;
}

void accessMemory(address addr, word* data, WriteEnable we)
{
  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  int offset_bit = uint_log2(block_size);
  int index_bit = uint_log2(set_count);
  int tag_bit = 32 - index_bit - offset_bit;

  
  int tag = calcTag(addr, tag_bit);
  int index = calcIndex(addr, tag_bit, offset_bit);
  int offset = calcOffset(addr, tag_bit, offset_bit);

  int block = 0;
  address tmpaddr;

  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */

  /* Start adding code here */

  int i = 0;
  while (i < assoc)
  {
    cacheBlock currBlock = cache[index].block[i]; // access current block of cache
    if(tag == currBlock.tag) // if tag in cache matches that of memory 
    {
      if(currBlock.valid == VALID) // if valid bit is valid 
      {
        currBlock.accessCount = currBlock.accessCount + 1; // increase value of cache[index].block[i]
        for(int j = 0; j < assoc; j++)
        {
          cache[index].block[j].lru.value = cache[index].block[j].lru.value + 1; 
          // increase value of block as per LRU algorithm
        }
        currBlock.lru.value = 0;

        if(we == READ)
        {
          memcpy((void *)data, (void *) ((byte *) cache[index].block[block].data + offset), sizeof(word));
          // read cache block contents from memory to data 
          break;
        } 
        else if (we == WRITE || we != READ)
        {
          memcpy((void *)((byte *) cache[index].block[block].data + offset), (void *)data, sizeof(word));
          // write from data in processor to cache
          if(memory_sync_policy == WRITE_THROUGH) // replacement
          {
            tmpaddr = addr >> offset_bit;
            tmpaddr = tmpaddr << offset_bit; // shift left again 
            for(int j = 0; j < block_size; j++)
            { // accessDRAM calls physical mem between address and memory
              accessDRAM(tmpaddr + j, (byte *)currBlock.data+j, WORD_SIZE, WRITE);

            }
            break;
          } 
          else 
          {
            currBlock.dirty = DIRTY;
            break;
          }
        }
        
      highlight_offset(index, i, offset, HIT);
	    highlight_block(index, i);

      } 
      else 
      { 

        tmpaddr = addr >> offset_bit;
        tmpaddr = tmpaddr << offset_bit;
        // access from memory 
        for(int j = 0; j < block_size; j++)
        {
          accessDRAM(tmpaddr+j, (byte *) currBlock.data+j, WORD_SIZE, WRITE);
        }
        
        currBlock.accessCount = currBlock.accessCount + 1;
        currBlock.dirty = VIRGIN;
        currBlock.tag = tag;
        currBlock.valid = VALID;
        currBlock.lru.value = 0;
        
        for (int j = 0; j < assoc; j++)
        {
          cache[index].block[j].lru.value = cache[index].block[j].lru.value + 1;
        }
        currBlock.valid = VALID;
        currBlock.lru.value = 0;
        
        if (we == READ)
        {
          memcpy((void *) data, (void *) ((byte *) currBlock.data + offset), sizeof(word));
          // read cache block contents from memory to data 
          break;
        }
        else if (we == WRITE || we != READ)
        {
          memcpy((void *) ((byte *) currBlock.data + offset), (void *) data, sizeof(word));
          // write from data in processor to cache
          
          if (memory_sync_policy == WRITE_THROUGH)
          {
            tmpaddr = addr >> offset_bit;
            tmpaddr = tmpaddr << offset_bit;
            int jIt = 0;
            while (jIt < block_size)
            {
              accessDRAM(tmpaddr + jIt, (byte *) currBlock.data + jIt, WORD_SIZE, WRITE);
              jIt++;
              // access DRAM again
            }
            break;
          }
          else
          {
            currBlock.dirty = DIRTY;
            // dirty bit
            break;
          }
        }
	
  highlight_offset(index, i, offset, HIT);
	highlight_block(index, i);

      }
    } 
    
    // END OF ****************************** if(tag == cache[index].block[i].tag) ****************************************
    
    
    else // cache miss
    {
      for (int it = 0; it < assoc; it++)
      {
        if (cache[index].block[it].valid == INVALID)
        {
          block = it;
        }
      }
  
      if (policy == RANDOM)
      {
        block = randomint(assoc);
        highlight_offset(index, block, offset, MISS);
	      highlight_block(index, block);
        // random policy for replacement
      }
      else if (policy == LFU && policy != RANDOM)
      {
        int small = 0xFFFFFFFF;
        int iter = 0;
        while (iter < assoc) 
        //for (int i = 0; i < assoc; i++)
        {
          if (cache[index].block[iter].accessCount < small)
          {
            block = iter;
            small = cache[index].block[iter].accessCount;
            // set the smallest value according to accessCount 
          }
        }
        highlight_offset(index, i, offset, MISS);
	      highlight_block(index, i);
      }
      else  
      {
        int big = 0;
        for (int i = 0; i < assoc; i++)
        {
          if (cache[index].block[i].lru.value > big)
          // 0 is a temporary placeholder for big, find the big (freq) value
          {
            block = i;
            big = cache[index].block[i].lru.value > big;
          }
        }
      }

      if (cache[index].block[block].dirty == DIRTY) 
      {
        if (cache[index].block[block].valid == VALID) 
        {
          tmpaddr = cache[index].block[block].tag << (offset_bit + index_bit);
          tmpaddr = tmpaddr | (index << offset_bit);
          for (int j = 0; j < block_size; j += 4)
          {
            accessDRAM(tmpaddr + j, (byte *) cache[index].block[block].data + j, WORD_SIZE, WRITE);
          }
        }
      }
      tmpaddr = addr >> offset_bit;
      tmpaddr = tmpaddr << offset_bit;
      for (int j = 0; j < block_size; j++)
      {
        accessDRAM(tmpaddr + j, (byte *) cache[index].block[block].data + j, WORD_SIZE, READ);
      }

      currBlock.valid = VALID;
      currBlock.tag = tag;
      currBlock.lru.value = 0;
      currBlock.dirty = VIRGIN;
      
      cache[index].block[block].accessCount = cache[index].block[block].accessCount + 1;
      int kIt = 0;
      while (kIt < assoc)
      {
        cache[index].block[kIt].lru.value++;
        kIt = kIt + 1;
      }
      cache[index].block[block].lru.value = 0;
      
      if (we == READ)
      {
        memcpy((void *) data, (void *) ((byte *) cache[index].block[block].data + offset), sizeof(word));
        break;
      }
      else if (we != READ || we == WRITE)
      {
        memcpy((void *) ((byte *) cache[index].block[block].data + offset), (void *) data, sizeof(word));
        
        if (memory_sync_policy == WRITE_THROUGH)
        {
          tmpaddr = addr >> offset_bit;
          tmpaddr = tmpaddr << offset_bit;
          for (int j = 0; j < block_size; j++)
          {
            accessDRAM(tmpaddr + j, (byte *) cache[index].block[block].data + j, WORD_SIZE, WRITE);
          }
          break;
        }
        else
        {
          cache[index].block[block].dirty = DIRTY;
          break;
        }
      }

  highlight_offset(index, i, offset, MISS);
	highlight_block(index, i);
    }
    i++;
}

  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
  accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}
