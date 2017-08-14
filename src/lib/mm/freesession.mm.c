
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


void mmsessionfree (mmsession s, mmsessionflag flag) {
	
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
	
	#ifdef MMTHREADSAFE
	mutexlock(&mmsessionmutex);
	#endif
	
	if (flag == MMDOSUBSESSIONS) {
		// s.s is set to the parent session of
		// the session that will be freed.
		s.s = s.s->prevsession;
		
		// This loop free all blocks from the current session
		// all the way up to the session pointed by s.s .
		// Locking is not used on each session to free,
		// because they should no longer be in use.
		while (mmcurrentsession != s.s) {
			// When freeing an entire session, there is no
			// need to detach any block from their session circular
			// linkedlist. I simply free all the blocks of the circular
			// linkedlist and then I deallocate the session.
			// I need to remember that struct session also act as
			// a dumb block so that I can create a circular linkedlist
			// of blocks belonging to the same session and always
			// have a pointer to at least one element of the circular
			// linkedlist when I need to free all blocks of a session.
			// Blocks are freed in the reverse
			// order in which they were allocated,
			// as they may have destructors set
			// which must be called in the reverse
			// order in which their respective
			// blocks were allocated.
			b = mmcurrentsession->prevsessionblock;
			while (b != (mmblock*)mmcurrentsession) {
				b = b->prevsessionblock;
				mmfreeblock(b->nextsessionblock, 0);
			}
			
			// I set mmcurrentsession to the parent session.
			mmcurrentsession = mmcurrentsession->prevsession;
			
			// Now that I have set mmcurrentsession to the parent
			// session, I can free the session that was previously current.
			// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
			// the address in sessiontofree set it at the beginning of
			// the block to free.
			// Note that memory allocated for a session is not added to
			// any session, hence mmfreeblock() is called with its argument
			// flag set to null.
			mmfreeblock((mmblock*)((void*)mmcurrentsession->nextsession - (sizeof(mmblock) - 2*sizeof(mmblock*))), 0);
		}
		
		mmcurrentsession->nextsession = 0;
		
	} else {
		// If the session being freed is the current session,
		// mmcurrentsession is set to the parent session of s.s .
		if (mmcurrentsession == s.s) mmcurrentsession = s.s->prevsession;
		
		// When freeing an entire session, there is no need to
		// detach any block from their session circular linkedlist.
		// I simply free all the blocks of the circular linkedlist
		// and then I deallocate the session.
		// I need to remember that struct session also act as a dumb
		// block so that I can create a circular linkedlist of blocks
		// belonging to the same session and always have a pointer to
		// at least one element of the circular linkedlist when I need
		// to free all blocks of a session.
		// Blocks are freed in the reverse
		// order in which they were allocated,
		// as they may have destructors set
		// which must be called in the reverse
		// order in which their respective
		// blocks were allocated.
		b = s.s->prevsessionblock;
		while (b != (mmblock*)s.s) {
			b = b->prevsessionblock;
			mmfreeblock(b->nextsessionblock, 0);
		}
		
		// I detach the session from the linkedlist of sessions.
		// The field prevsession of a session created through mmsessionnew()
		// is never null because it is either set to mmfirstsession or set
		// to a session created through mmsessionnew().
		s.s->prevsession->nextsession = s.s->nextsession;
		if (s.s->nextsession) s.s->nextsession->prevsession = s.s->prevsession;
		
		// I free the session for which I freed all the blocks.
		// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
		// the address in sessiontofree set it at the beginning of
		// the block to free.
		// Note that memory allocated for a session is not added
		// to any session, hence mmfreeblock() is called with
		// its argument flag set to null.
		mmfreeblock((mmblock*)((void*)s.s - (sizeof(mmblock) - 2*sizeof(mmblock*))), 0);
	}
	
	#ifdef MMTHREADSAFE
	mutexunlock(&mmsessionmutex);
	#endif
}
