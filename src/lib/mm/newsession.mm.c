
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


mmsession mmsessionnew () {
	// This function is similar to mmalloc() but only
	// allocate a block of sizeof(session), and do not add
	// the allocated block to the current session.
	void* allocmemforsession () {
		// This variable hold the actual size of the block
		// that gets set in the field size of a struct mmblock.
		// (sizeof(mmblock) - 2*sizeof(mmblock*)) is the space
		// occupied by the mmblock header where (2*sizeof(mmblock*))
		// account for the fields prev and next of a struct mmblock
		// which are not part of the mmblock header but are used
		// to link free blocks together.
		uint blocksize = sizeof(mmsessioninternal) + (sizeof(mmblock) - 2*sizeof(mmblock*));
		
		// Get the index within the array of free blocks and
		// call mmallocateblock() to allocate a block.
		// There is no need to check whether
		// mmsizetoindexforblockselection(blocksize) >= MMNBROFLINKEDLIST
		// because the memory occupied by a struct session will
		// always be small.
		mmblock* b = mmallocateblock(mmsizetoindexforblockselection(blocksize));
		
		// I set the useable size of the block.
		b->size = blocksize;
		
		// Return the address where the useable part of the block start.
		return (void*)b + (sizeof(mmblock) - 2*sizeof(mmblock*));
	}
	
	mmsession newsession;
	
	newsession.s = (mmsessioninternal*)allocmemforsession();
	
	// The fields prevsessionblock and nextsessionblock
	// below are set to newsession.s because the linkedlist
	// of blocks belonging to the same session is circular.
	// I need to remember that struct session also act as
	// a dumb block so that I can create a circular linkedlist
	// of blocks belonging to the same session and always
	// have a pointer to at least one element of the circular
	// linkedlist when I need to free all blocks of a session.
	newsession.s->prevsessionblock = (mmblock*)newsession.s;
	newsession.s->nextsessionblock = (mmblock*)newsession.s;
	
	#ifdef MMTHREADSAFE
	mutexlock(&mmsessionmutex);
	#endif
	
	// Add the newly created session to the linkedlist of sessions.
	mmcurrentsession->nextsession = newsession.s;
	newsession.s->prevsession = mmcurrentsession;
	newsession.s->nextsession = 0;
	
	// Make the newly created session, the current session.
	mmcurrentsession = newsession.s;
	
	#ifdef MMTHREADSAFE
	// Initialize the session mutex.
	newsession.s->mutex = mutexnull;
	#endif
	
	#ifdef MMTHREADSAFE
	mutexunlock(&mmsessionmutex);
	#endif
	
	return newsession;
}
