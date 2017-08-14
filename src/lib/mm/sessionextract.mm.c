
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


void mmsessionextract (mmsession s, void* ptr) {
	
	mmblock* b;
	
	// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
	// the address in s.s set it at the beginning of the block.
	b = (mmblock*)((void*)s.s - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	#ifdef MMDEBUG
	// If b point to a memory location that is not a block
	// which belong to this instance of the memory manager,
	// mmbadsignature() is called.
	if (b->signature != (uint)&mmarrayofpointerstolinkedlistoffreeblocks)
		mmbadsignature();
	#endif
	
	// I save the current value of s as it will be used when
	// calling the non-variadic version of mmsessionfree().
	mmsession sessiontofree = s;
	
	#ifdef MMTHREADSAFE
	mutexlock(&mmsessionmutex);
	#endif
	
	// s.s is set to the parent session
	// of the session that will be freed.
	s.s = s.s->prevsession;
	
	#ifdef MMTHREADSAFE
	mutexlock(&s.s->mutex);
	#endif
	
	#ifdef MMTHREADSAFE
	mutexunlock(&mmsessionmutex);
	#endif
	
	// Get the address of the memory block.
	b = ptr - (sizeof(mmblock) - 2*sizeof(mmblock*));
	
	#ifdef MMDEBUG
	// If b point to a memory location that is not a block
	// which belong to this instance of the memory manager,
	// mmbadsignature() is called.
	if (b->signature != (uint)&mmarrayofpointerstolinkedlistoffreeblocks)
		mmbadsignature();
	#endif
	
	#ifdef MMTHREADSAFE
	mutexlock(&b->session->mutex);
	#endif
	
	// Remove the block from its session linkedlist.
	// Note that the block may not belong to the session that
	// I am freeing; the linkedlist need to be correctly
	// linked for the call to mmsessionfree() below.
	LINKEDLISTCIRCULARREMOVE(prevsessionblock, nextsessionblock, b, ((mmblock*){(mmblock*)0}));
	
	#ifdef MMTHREADSAFE
	mutexunlock(&b->session->mutex);
	#endif
	
	#ifdef MMTHREADSAFE
	b->session = s.s;
	#endif
	
	// Add the block to the linkedlist of the parent
	// session of the session that will be freed.
	// Note that LINKEDLISTCIRCULARADDTOBOTTOM()
	// will not modify s.s which must not be modified.
	LINKEDLISTCIRCULARADDTOBOTTOM(prevsessionblock, nextsessionblock, b, s.s);
	
	#ifdef MMTHREADSAFE
	mutexunlock(&s.s->mutex);
	#endif
}
