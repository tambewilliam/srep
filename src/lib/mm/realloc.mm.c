
// ---------------------------------------------------------------------
// Copyright (c) 2009, William Fonkou Tambe
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------


// This function resize the memory block pointed by ptr.
// If ptr is null, mmalloc(size) is returned.
// If there are multiple references to the memory block
// being resized, its reference count get decremented
// and it get copied to a new memory block to return.
// If size is -1 and ptr is non-null, no resizing occur;
// and if there are multiple references to the memory block,
// it get duplicated to a new memory block to return
// after its reference count has been decremented,
// otherwise the same memory block is returned.
// This function should always be assumed successful.
// In contrast to realloc() found in gnu stdlib, when size
// is zero, the memory block is not freed but simply marked
// as having a null size; mmfree() is the only function
// which completely free the memory block.
// It is done that way because it make much sens and
// make programming more coherent; furthermore it is faster,
// because a block set to a size of zero in the flow
// of a program is more likely to be reused again
// and resized to a non-zero size.
// Any block being resized will always stay in its session
// of origin even if a copy is done to a larger block during
// reallocation; a block is assigned to a new session only
// during its creation, and until it is freed by mmfree(),
// it will always belong to its session of origin.
// The memory address returned is aligned to sizeof(uint).
void* mmrealloc (void* ptr, uint size) {
	// This function is used to copy data
	// from one block to another block.
	// The mmblock header is not copied
	// and is skipped.
	void copyblock (void* dest, void* src, uint sz) {
		// Note that the argument sz will always have
		// a value that is a multiple of sizeof(uint).
		void* limit = src + sz;
		
		// I skip the header in both the source and
		// destination block and set src and dest where
		// the data in the block start.
		// Substracting 2*sizeof(mmblock*) account for
		// the fields prev and next of struct mmblock which
		// are not part of the header but are used
		// to link free blocks together.
		src += sizeof(mmblock) - 2*sizeof(mmblock*);
		dest += sizeof(mmblock) - 2*sizeof(mmblock*);
		
		while (1) {
			
			if (src < limit) *(uint*)dest = *(uint*)src;
			else return;
			
			src += sizeof(uint);
			dest += sizeof(uint);
		}
	}
	
	if (!ptr) return mmalloc(size); // The fields for session and size will be set by mmalloc().
	
	// This set ptr to the beginning of the block.
	ptr -= (sizeof(mmblock) - 2*sizeof(mmblock*));
	
	#ifdef MMDEBUG
	// If ptr point to a memory location that is not
	// a block which belong to this instance of
	// the memory manager, mmbadsignature() is called.
	if (((mmblock*)ptr)->signature != (uint)&mmarrayofpointerstolinkedlistoffreeblocks)
		mmbadsignature();
	#endif
	
	// If size is -1, no resizing occur.
	if (size == -1) size = (((mmblock*)ptr)->size - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	// This variable hold the actual size of the block
	// that get set in the field size of a struct mmblock.
	// (sizeof(mmblock) - 2*sizeof(mmblock*)) is the space
	// occupied by the mmblock header where (2*sizeof(mmblock*))
	// account for the fields prev and next of a struct mmblock
	// which are not part of the mmblock header but are used
	// to link free blocks together.
	uint blocksize = size + (sizeof(mmblock) - 2*sizeof(mmblock*));
	
	if (((mmblock*)ptr)->refcnt > 1) {
		// If I get here, there are multiple references
		// to the memory block to reallocate; I decrement
		// its reference count and copy it to a new memory block
		// which is to be the result of the reallocation.
		
		--((mmblock*)ptr)->refcnt;
		
		mmblock* b = mmallocblock(size);
		
		// Preserve the destructor address.
		b->dtor = ((mmblock*)ptr)->dtor;
		
		// Copy from ptr to b.
		// Note that copyblock() skips the header part of the blocks.
		copyblock(b, ptr, ((blocksize <= ((mmblock*)ptr)->size) ? blocksize : ((mmblock*)ptr)->size));
		
		#ifdef MMTHREADSAFE
		mutexlock(&mmcurrentsession->mutex);
		b->session = mmcurrentsession;
		#endif
		
		// Add the newly obtained block to the current session.
		// Note that LINKEDLISTCIRCULARADDTOBOTTOM() will not
		// modify mmcurrentsession which must not be modified.
		LINKEDLISTCIRCULARADDTOBOTTOM(prevsessionblock, nextsessionblock, b, mmcurrentsession);
		
		#ifdef MMTHREADSAFE
		mutexunlock(&mmcurrentsession->mutex);
		#endif
		
		// Return the address where the useable part of the block start.
		return (void*)b + (sizeof(mmblock) - 2*sizeof(mmblock*));
	}
	
	// Get the index of the block.
	uint i = ((mmblock*)ptr)->index;
	
	// Get the index within the array of free blocks.
	uint j = mmsizetoindexforblockselection(blocksize);
	
	// If the newly requested size can still fit in
	// the current block, there is no additional work to do.
	if (i == j) {
		// I set the useable size of the block.
		((mmblock*)ptr)->size = blocksize;
		
		// Return the address where the useable part of the block start.
		return ptr + (sizeof(mmblock) - 2*sizeof(mmblock*));
	}
	
	if (j >= MMNBROFLINKEDLIST) {
		// If I get here, the requested reallocation size
		// cannot be allocated using the buddy system algorithm
		// as it would yield waist; instead the best multiple of
		// a pagesize that satisfy the memory request is allocated.
		// A copy is done from the block to reallocate to
		// the block created for the reallocation.
		// The block to reallocate is freed after the copy.
		
		mmblock* b;
		
		// I request memory from the kernel; retrying on failure.
		do b = (mmblock*)mmap(0, j, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
			while (b == (mmblock*)-1);
		
		// Preserve the destructor address.
		b->dtor = ((mmblock*)ptr)->dtor;
		
		// Copy from ptr to b; note that copyblock()
		// skip the header part of the blocks.
		copyblock(b, ptr, ((blocksize <= ((mmblock*)ptr)->size) ? blocksize : ((mmblock*)ptr)->size));
		
		#ifdef MMTHREADSAFE
		mutexlock(&((mmblock*)ptr)->session->mutex);
		b->session = ((mmblock*)ptr)->session;
		#endif
		
		// The block pointed by ptr need to be replaced in
		// its session linkedlist by the new block, hence
		// preserving session informations during reallocation.
		LINKEDLISTCIRCULARSUBSTITUTE(prevsessionblock, nextsessionblock, (mmblock*)ptr, b);
		
		#ifdef MMTHREADSAFE
		mutexunlock(&((mmblock*)ptr)->session->mutex);
		#endif
		
		// I free the block being reallocated.
		mmfreeblock((mmblock*)ptr, 0);
		
		#ifdef MMDEBUG
		b->signature = (uint)&mmarrayofpointerstolinkedlistoffreeblocks;
		#endif
		
		b->debug.changecount = 0;
		b->debug.lastchangefilename = (u8*)0;
		
		b->refcnt = 1;
		
		b->index = j;
		
		// I set the useable size of the block.
		b->size = blocksize;
		
		// Return the address where the useable part of the block start.
		return (void*)b + (sizeof(mmblock) - 2*sizeof(mmblock*));
		
		// The fields startofregion and sizeofregion of the block
		// created for the reallocation are not set since
		// it was not allocated using the buddy system algorithm.
	}
	
	if (i >= MMNBROFLINKEDLIST) {
		// If I get here, the block to reallocate was not
		// allocated using the buddy system algorithm.
		// Note that I get here only if the requested reallocation size
		// could be allocated using the buddy system algorithm; so I allocate
		// the reallocation block using the buddy system algorithm, and
		// copy to it the data from the block to reallocate.
		// The block to reallocate is freed after the copy.
		
		// I call mmallocateblock() to allocate a block.
		mmblock* b = mmallocateblock(j);
		
		// Preserve the destructor address.
		b->dtor = ((mmblock*)ptr)->dtor;
		
		// Copy from ptr to b.
		// Note that copyblock() skips the header part of the blocks.
		copyblock(b, ptr, ((blocksize <= ((mmblock*)ptr)->size) ? blocksize : ((mmblock*)ptr)->size));
		
		#ifdef MMTHREADSAFE
		mutexlock(&((mmblock*)ptr)->session->mutex);
		b->session = ((mmblock*)ptr)->session;
		#endif
		
		// The block pointed by ptr need to be replaced
		// in its session linkedlist by the new block,
		// hence preserving session informations
		// during reallocation.
		LINKEDLISTCIRCULARSUBSTITUTE(prevsessionblock, nextsessionblock, (mmblock*)ptr, b);
		
		#ifdef MMTHREADSAFE
		mutexunlock(&((mmblock*)ptr)->session->mutex);
		#endif
		
		// I free the block being reallocated.
		// It is faster to do the freeing work here than
		// calling mmfreeblock((mmblock*)ptr, 0), since I know
		// that the block to free was not allocated using
		// the buddy system algorithm.
		munmap(ptr, i);
		
		// I set the useable size of the block.
		b->size = blocksize;
		
		// The fields refcnt and index of the block
		// pointed by b were already set by mmallocateblock()
		// so no need to set them again.
		
		// Return the address where the useable
		// part of the block start.
		return (void*)b + (sizeof(mmblock) - 2*sizeof(mmblock*));
	}
	
	// If I get here both the size of the block to
	// reallocate and the requested reallocation size
	// are valid for use with the buddy system algorithm.
	
	// This variable will point to the buddy block.
	mmblock* buddy;
	
	// I start by the case where I need to resize
	// to a smaller memory size because it is faster.
	// There is no copy to do here because the top part
	// of the block will not changed; All I do is to
	// reduce the size of the block by removing
	// the bottom part of of the block.
	if (j < i) {
		// I simply use the same algorithm used
		// in mmallocateblock() to break down a larger block.
		do {
			--i;
			
			// MMSMALLESTBUDDY is the minimum size of a block.
			// So adding to ptr the size of the blocks
			// indexed by i will get us half way the size
			// of the block in ptr.
			// One of the half is returned to the free blocks
			// and its fields startofregion and sizeofregion are set.
			
			buddy = (mmblock*)(ptr + (MMSMALLESTBUDDY<<i));
			
			buddy->startofregion = ((mmblock*)ptr)->startofregion;
			buddy->sizeofregion = ((mmblock*)ptr)->sizeofregion;
			
			mmaddblocktofreeblocks(buddy, i);
			
		} while (i > j);
		
		// Set the new index.
		((mmblock*)ptr)->index = j;
		
		// The block is already marked as used.
		// So no need to mark it again.
		
		// I set the useable size of the block.
		((mmblock*)ptr)->size = blocksize;
		
		// Return the address where the useable
		// part of the block start.
		return ptr + (sizeof(mmblock) - 2*sizeof(mmblock*));
		
	} else if (j > i) {
		// If I get here, I should resize to a bigger memory size.
		
		// I use a slight variant of the algorithm used
		// in mmfreeblock() and if I cannot allocate memory
		// I use the same algorithm used in mmallocateblock()
		// and if that fail too then I have to reverse what
		// was done in order to leave the block pointed
		// by ptr intact.
		
		// I save ptr and i, just in case this
		// algorithm also found in mmfreeblock() fail.
		void* savedptr = ptr;
		// uint savedi = i; // NOTE: No longer needed because of the code commented further below.
		
		findbuddy:
		
		// The check below test whether I am at an even
		// or odd addressing with respect to the type of blocks
		// indexed by i. The test below is a faster version of:
		// (((uint)ptr - (uint)((mmblock*)ptr)->startofregion)/(MMSMALLESTBUDDY<<i))&1
		if ((((uint)ptr - (uint)((mmblock*)ptr)->startofregion) >> (MMSMALLESTBUDDYCLOG2+i)) & 1) {
			// If I get here, it means that the block pointed
			// by ptr is at the right of its buddy, and the buddy
			// is at the left of the block pointed by ptr.
			
			// A block found to be at the right of its buddy
			// will always have a buddy at its left. because
			// (((uint)ptr - (uint)((mmblock*)ptr)->startofregion) >> (MMSMALLESTBUDDYCLOG2+i))
			// used in the test above was certainly not null which
			// mean that there was certainly a space at the left of
			// the block within the region where the block belongs.
			// So there is no need to check if I can have a legitimate
			// buddy on the left, just like I need to do for blocks
			// having their buddy at their right.
			
			buddy = (mmblock*)(ptr - (MMSMALLESTBUDDY<<i));
			
			#ifdef MMTHREADSAFE
			// I lock the linkedlist with
			// index i to safely modify it.
			mutexlock(&mmfreeblocksmutexes[i]);
			#endif
			
			// Check whether the buddy is not inuse
			// and that it was not splitted.
			if (!buddy->refcnt && buddy->index == i) {
				// If I get here I need to combine both block.
				
				// I detach the buddy from the linkedlist indexed by i.
				LINKEDLISTCIRCULARREMOVE(prev, next, buddy, mmarrayofpointerstolinkedlistoffreeblocks[i]);
				
				// I can update ptr to the address of the left buddy
				// since it will be the head of the combined block.
				ptr = buddy;
				
				#ifdef MMTHREADSAFE
				mutexunlock(&mmfreeblocksmutexes[i]);
				#endif
				
				// I increase i without setting the field index
				// and refcnt of the newly combined block yet;
				// it will be done at the end.
				++i;
				
				// If I still do not have a big enough block,
				// I need to try to find another buddy.
				// On the other hand, if I have a big enough block,
				// I jump to mergingsucceeded.
				if (i < j) goto findbuddy;
				else goto mergingsucceeded;
				
			} else {
				#ifdef MMTHREADSAFE
				mutexunlock(&mmfreeblocksmutexes[i]);
				#endif
			}
			
		} else {
			// If I get here the buddy is at the right
			// of the block pointed by ptr.
			
			buddy = (mmblock*)(ptr + (MMSMALLESTBUDDY<<i));
			
			// I first check that I can have a legitimate
			// buddy on the right, by making sure that the address
			// of the first byte within the buddy at the right
			// is not beyond the end boundary of the region
			// to which the block belong.
			if ((void*)buddy < (((mmblock*)ptr)->startofregion + ((mmblock*)ptr)->sizeofregion)) {
				#ifdef MMTHREADSAFE
				// I lock the linkedlist with
				// index i to safely modify it.
				mutexlock(&mmfreeblocksmutexes[i]);
				#endif
				
				// Check whether the buddy is not inuse
				// and that it was not splitted.
				if (!buddy->refcnt && buddy->index == i) {
					// If I get here I need to combine both block.
					
					// I detach the buddy from the linkedlist indexed by i.
					LINKEDLISTCIRCULARREMOVE(prev, next, buddy, mmarrayofpointerstolinkedlistoffreeblocks[i]);
					
					// ptr is set to the address where the head
					// of the combined block is located.
					
					#ifdef MMTHREADSAFE
					mutexunlock(&mmfreeblocksmutexes[i]);
					#endif
					
					// I increase i without setting the fields index
					// and refcnt of the newly combined block yet;
					// it will be done at the end.
					++i;
					
					// If I still do not have a big enough block,
					// I need to try to find another buddy.
					// On the other hand, if I have a big enough block,
					// I jump to mergingsucceeded.
					if (i < j) goto findbuddy;
					else goto mergingsucceeded;
					
				} else {
					#ifdef MMTHREADSAFE
					mutexunlock(&mmfreeblocksmutexes[i]);
					#endif
				}
			}
		}
		
		// Getting here mean that I tried doing all
		// the possible mergings without getting
		// the reqested size indexed by j.
		
		// I call mmallocateblock() to get the block that I need.
		void* newptr = (void*)mmallocateblock(j);
		
		#ifdef MMTHREADSAFE
		mutexlock(&((mmblock*)savedptr)->session->mutex);
		((mmblock*)newptr)->session = ((mmblock*)savedptr)->session;
		#endif
		
		// The block pointed by savedptr need to be
		// replaced in its session linkedlist by
		// the new block, hence preserving session
		// informations during reallocation.
		LINKEDLISTCIRCULARSUBSTITUTE(prevsessionblock, nextsessionblock, (mmblock*)savedptr, (mmblock*)newptr);
		
		#ifdef MMTHREADSAFE
		mutexunlock(&((mmblock*)savedptr)->session->mutex);
		#endif
		
		// Preserve the destructor address.
		((mmblock*)newptr)->dtor = ((mmblock*)savedptr)->dtor;
		
		// Copy from savedptr to newptr; note that copyblock()
		// skip the header part of the blocks.
		copyblock(newptr, savedptr, ((mmblock*)savedptr)->size);
		
		// I add to the linkedlist indexed by i,
		// the block pointed by ptr, since I copied the content
		// of the block pointed by savedptr to newptr.
		// Remember that the block pointed by ptr is
		// the result of trying to merge blocks in order
		// to obtain a block size of index j.
		// The block pointed by savedptr is inside the block
		// pointed by ptr obtained after doing mergings; so
		// although I lost time because of mmallocateblock(),
		// I have gained by doing defragmentation-work when
		// I merged blocks together; and those blocks are now
		// freed by calling mmaddblocktofreeblocks().
		// Even if ptr is still equal to savedptr because at
		// least a single defragmentation could not be done,
		// everything is still fine since savedptr is always
		// going to either have the same value as ptr or point
		// to a block within the block pointed by ptr. Note that
		// during merging of blocks I did not modify any of
		// the fields startofregion or sizeofregion of blocks
		// because I am only working within the region of
		// the block being freed.
		mmaddblocktofreeblocks((mmblock*)ptr, i);
		
		// The fields refcnt and index of the block pointed
		// by newptr were already set by mmallocateblock() so
		// no need to set them again.
		
		// I set the useable size of the block.
		((mmblock*)newptr)->size = blocksize;
		
		// Return the address where the useable
		// part of the block start.
		return newptr + (sizeof(mmblock) - 2*sizeof(mmblock*));
		
		// NOTE: The code below will never be executed
		// because mmallocateblock() should always be successfull.
		// But for reference it is kept to show how
		// the reversing work can be done.
		#if 0
		// If I get here then mmallocateblock() failed.
		// I should reverse the work done when I merged
		// blocks together. It is only a matter of breaking
		// back down blocks and attaching them to their
		// respective linkedlist. It is similar to
		// the algorithm used in mmallocateblock() but different.
		// The fields index and refcnt do not need to be set
		// again because they were not touched, that is why
		// I do not directly use mmaddblocktofreeblocks() because
		// it has overheads and will do the redundant work of
		// updating the fields index and refcnt.
		// Similarly the field startofregion and sizeofregion
		// where not touched so I will not need to set them again.
		
		if (i == savedi) return 0; // There is no work to do if i did not change.
		
		do {
			--i;
			
			// Note that the block pointed by savedptr is within the bigger block pointed by
			// ptr. So this relation is always true: (savedptr > ptr); That is why I use
			// (savedptr < ptr + (MMSMALLESTBUDDY << i)) to determine where within the block
			// pointed by ptr, the block pointed by savedptr is.
			
			buddy = (mmblock*)(ptr + (MMSMALLESTBUDDY << i));
			
			if (savedptr < (void*)buddy) {
				// If I get here the saved block is in the left-buddy and I should
				// attach the right-buddy back to its linkedlist.
				
				#ifdef MMTHREADSAFE
				// I lock the linkedlist with
				// index i to safely modify it.
				mutexlock(&mmfreeblocksmutexes[i]);
				#endif
				
				LINKEDLISTCIRCULARADDTOBOTTOM(prev, next, buddy, mmarrayofpointerstolinkedlistoffreeblocks[i]);
				
				#ifdef MMTHREADSAFE
				mutexunlock(&mmfreeblocksmutexes[i]);
				#endif
				
				// There is no need to update ptr because the saved block is part of the block
				// currently pointed by ptr or on the side of the block pointed by ptr.
				
			} else {
				// If I get here the saved block is in the right-buddy and I should
				// free the left-buddy.
				
				#ifdef MMTHREADSAFE
				// I lock the linkedlist with
				// index i to safely modify it.
				mutexlock(&mmfreeblocksmutexes[i]);
				#endif
				
				LINKEDLISTCIRCULARADDTOBOTTOM(prev, next, (mmblock*)ptr, mmarrayofpointerstolinkedlistoffreeblocks[i]);
				
				#ifdef MMTHREADSAFE
				mutexunlock(&mmfreeblocksmutexes[i]);
				#endif
				
				// I update ptr to the right-buddy
				// containing the saved block.
				ptr += (MMSMALLESTBUDDY << i);
			}
			
		} while (i > savedi);
		
		// When I get here, I am sure that ptr
		// has been restored to its saved value.
		
		// Any attempt to resize the memory block to
		// a bigger size failed, so I should return null.
		return 0;
		#endif
		
		// Jumping here mean that I was able to merge
		// blocks and get the size indexed by j.
		mergingsucceeded:
		
		// I need to copy data only if while doing mergings
		// the data within the initial block were not anymore
		// at the top of the block.
		// In addition, the block pointed by savedptr will need
		// to be replaced in its session linkedlist by the block
		// pointed by ptr; hence preserving session informations
		// during reallocation.
		if (ptr != savedptr) {
			#ifdef MMTHREADSAFE
			mutexlock(&((mmblock*)savedptr)->session->mutex);
			((mmblock*)ptr)->session = ((mmblock*)savedptr)->session;
			#endif
			
			LINKEDLISTCIRCULARSUBSTITUTE(prevsessionblock, nextsessionblock, (mmblock*)savedptr, (mmblock*)ptr);
			
			#ifdef MMTHREADSAFE
			mutexunlock(&((mmblock*)savedptr)->session->mutex);
			#endif
			
			// Preserve the destructor address.
			((mmblock*)ptr)->dtor = ((mmblock*)savedptr)->dtor;
			
			// Copy from savedptr to ptr; note that copyblock()
			// skip the header part of the blocks.
			copyblock(ptr, savedptr, ((mmblock*)savedptr)->size);
		}
		
		// I get here only if merging succeeded and
		// I didn't need to call mmallocateblock().
		// It is important to do this in this order.
		// The resultant block from the merging procedure
		// is only protected by the fact that its index
		// is not yet the same as the index of its buddy,
		// since while doing mergings I was not updating
		// the field index.
		((mmblock*)ptr)->refcnt = 1;
		((mmblock*)ptr)->index = j;
		
		#ifdef MMDEBUG
		// I set the signature field of the block.
		((mmblock*)ptr)->signature = (uint)&mmarrayofpointerstolinkedlistoffreeblocks;
		#endif
		
		// I set the useable size of the block.
		((mmblock*)ptr)->size = blocksize;
		
		// Return the address where the useable
		// part of the block start.
		return ptr + (sizeof(mmblock) - 2*sizeof(mmblock*));
	}
}


// This function call mmrealloc() with
// the pointer and size given as argument;
// if the reallocation is increasing the
// size of the memory block, the added
// portion of the memory block get zeroed
// with bytsetz() before returning it.
void* mmreallocz (void* ptr, uint size) {
	// mmsz() will check that the memory block
	// belong to this instance of the memory manager.
	uint oldsize = mmsz(ptr);
	
	ptr = mmrealloc(ptr, size);
	
	// If size is -1, no resizing occur.
	if (size != -1 && size > oldsize) bytsetz(ptr + oldsize, size - oldsize);
	
	return ptr;
}
