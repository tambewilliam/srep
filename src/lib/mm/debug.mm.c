
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


// Debugging function substituting the function mmalloc().
void* mmdebugalloc (uint size, u8* filename, uint linenumber) {
	
	void* ptr = mmalloc(size);
	
	// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
	// the address in ptr set it at the beginning of the block.
	mmblock* b = (mmblock*)(ptr - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	b->debug.changecount = 0;
	b->debug.lastchangefilename = filename;
	b->debug.lastchangelinenumber = linenumber;
	
	// Not all platforms support execinfo.h .
	#if defined(__linux__)
	b->debug.backtrace[backtrace(b->debug.backtrace, MMDEBUGCALLBACKCOUNT-1)] = (void*)0;
	#else
	b->debug.backtrace[0] = (void*)0;
	#endif
	
	return ptr;
}


// Debugging function substituting the function mmallocz().
void* mmdebugallocz (uint size, u8* filename, uint linenumber) {
	
	void* ptr = mmallocz(size);
	
	// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
	// the address in ptr set it at the beginning of the block.
	mmblock* b = (mmblock*)(ptr - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	b->debug.changecount = 0;
	b->debug.lastchangefilename = filename;
	b->debug.lastchangelinenumber = linenumber;
	
	// Not all platforms support execinfo.h .
	#if defined(__linux__)
	b->debug.backtrace[backtrace(b->debug.backtrace, MMDEBUGCALLBACKCOUNT-1)] = (void*)0;
	#else
	b->debug.backtrace[0] = (void*)0;
	#endif
	
	return ptr;
}


// Debugging function substituting the function mmrealloc().
void* mmdebugrealloc (void* ptr, uint size, u8* filename, uint linenumber) {
	
	ptr = mmrealloc(ptr, size);
	
	// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
	// the address in ptr set it at the beginning of the block.
	mmblock* b = (mmblock*)(ptr - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	++b->debug.changecount;
	b->debug.lastchangefilename = filename;
	b->debug.lastchangelinenumber = linenumber;
	
	// Not all platforms support execinfo.h .
	#if defined(__linux__)
	b->debug.backtrace[backtrace(b->debug.backtrace, MMDEBUGCALLBACKCOUNT-1)] = (void*)0;
	#else
	b->debug.backtrace[0] = (void*)0;
	#endif
	
	return ptr;
}


// Debugging function substituting the function mmreallocz().
void* mmdebugreallocz (void* ptr, uint size, u8* filename, uint linenumber) {
	
	ptr = mmreallocz(ptr, size);
	
	// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
	// the address in ptr set it at the beginning of the block.
	mmblock* b = (mmblock*)(ptr - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	++b->debug.changecount;
	b->debug.lastchangefilename = filename;
	b->debug.lastchangelinenumber = linenumber;
	
	// Not all platforms support execinfo.h .
	#if defined(__linux__)
	b->debug.backtrace[backtrace(b->debug.backtrace, MMDEBUGCALLBACKCOUNT-1)] = (void*)0;
	#else
	b->debug.backtrace[0] = (void*)0;
	#endif
	
	return ptr;
}


// This function will iterate through each block currently
// allocated within the session given by the argument s; and
// for each block, the callback function given as argument will
// be called and passed as argument its address, size, refcount,
// the number of time it was reallocated, the name of the file
// within which it was allocated or last reallocated, and
// the line number within the file; if flag == MMIGNORESUBSESSIONS,
// this function also iterate through subsessions.
// The callback function arguments lastchangelinenumber and backtrace
// should not be used if the callback function argument lastchangefilename
// is null; the callback function argument lastchangefilename will
// be null if the block was allocated or reallocated using mmalloc()
// or mmrealloc() as oppose to mmdebugalloc() or mmdebugrealloc().
// It is safe to allocate memory within the callback function without
// fear of poluting the current session or the session given by
// the argument s, because internally this function will create
// a new session before making any calls to the callback function
// given as argument; mmsessioncancel(MMIGNORESUBSESSIONS) will be
// called on that previously created session before this function
// return, and any unfreed memory block allocated within the callback
// function will be moved to the current session; any subsession
// created within the callback function will be preserved.
void mmdebugsession (mmsession s, mmsessionflag flag,
	void(* callback)(void* addr, uint size, uint refcnt, uint changecount,
		u8* lastchangefilename, uint lastchangelinenumber,
		void** backtrace)) {
	
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
	
	if (flag == MMDOSUBSESSIONS) {
		
		#ifdef MMTHREADSAFE
		mutexlock(&mmsessionmutex);
		#endif
		
		mmsessioninternal* ss = mmcurrentsession;
		
		#ifdef MMTHREADSAFE
		mutexlock(&ss->mutex);
		#endif
		
		#ifdef MMTHREADSAFE
		mutexunlock(&mmsessionmutex);
		#endif
		
		// A new session is created so that if memory is allocated
		// within the callback function, it do not pollute the session
		// given by the argument s.
		mmsession sessionforcallback = mmsessionnew();
		
		while (1) {
			
			b = (mmblock*)ss;
			
			while ((b = b->nextsessionblock) != (mmblock*)ss)
				callback((void*)b + (sizeof(mmblock) - 2*sizeof(mmblock*)),
					b->size - (sizeof(mmblock) - 2*sizeof(mmblock*)),
					b->refcnt,
					b->debug.changecount,
					b->debug.lastchangefilename,
					b->debug.lastchangelinenumber,
					b->debug.backtrace);
			
			if (ss == s.s) {
				#ifdef MMTHREADSAFE
				mutexunlock(&ss->mutex);
				#endif
				
				break;
			}
			
			#ifdef MMTHREADSAFE
			mutexlock(&mmsessionmutex);
			#endif
			
			#ifdef MMTHREADSAFE
			mutexunlock(&ss->mutex);
			#endif
			
			ss = ss->prevsession;
			
			#ifdef MMTHREADSAFE
			mutexlock(&ss->mutex);
			#endif
			
			#ifdef MMTHREADSAFE
			mutexunlock(&mmsessionmutex);
			#endif
		}
		
		// The session created to isolate all blocks allocated within
		// the callback function is canceled and any memory block
		// created within that session get moved to the current session.
		mmsessioncancel(sessionforcallback, MMIGNORESUBSESSIONS);
		
	} else {
		// A new session is created so that if memory is allocated
		// within the callback function, it do not pollute the session
		// given by the argument s.
		mmsession sessionforcallback = mmsessionnew();
		
		#ifdef MMTHREADSAFE
		mutexlock(&s.s->mutex);
		#endif
		
		b = (mmblock*)s.s;
		
		while ((b = b->nextsessionblock) != (mmblock*)s.s)
			callback((void*)b + (sizeof(mmblock) - 2*sizeof(mmblock*)),
				b->size - (sizeof(mmblock) - 2*sizeof(mmblock*)),
				b->refcnt,
				b->debug.changecount,
				b->debug.lastchangefilename,
				b->debug.lastchangelinenumber,
				b->debug.backtrace);
		
		#ifdef MMTHREADSAFE
		mutexunlock(&s.s->mutex);
		#endif
		
		// The session created to isolate all blocks allocated within
		// the callback function is canceled and any memory block
		// created within that session get moved to the current session.
		mmsessioncancel(sessionforcallback, MMIGNORESUBSESSIONS);
	}
}
