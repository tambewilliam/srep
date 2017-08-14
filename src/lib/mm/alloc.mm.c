
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


// This function allocate a memory block
// large enough to contain the amount of bytes
// given by the argument size.
// mmalloc(0) will return a valid address that
// can be passed to mmfree(); in other words memory
// is also allocated when doing mmalloc(0).
// The allocated block is added to the current session.
// This function should always be assumed successful.
// The memory address returned is aligned to sizeof(uint).
void* mmalloc (uint size) {
	
	mmblock* b = mmallocblock(size);
	
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


// This function call mmalloc() with the size given
// as argument and zero the allocated block with
// bytsetz() before returning it.
void* mmallocz (uint size) {
	void* ptr = mmalloc(size);
	bytsetz(ptr, size);
	return ptr;
}
