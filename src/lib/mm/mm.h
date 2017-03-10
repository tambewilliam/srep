
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


// Memory manager which support up to 64bits systems.
// 
// A syscall is used to request memory from
// the kernel in multiple of a pagesize and manage it.
// Any memory region previously requested is returned
// to the kernel if the program free all the allocated blocks
// within that region; doing so not only is memory efficient
// but safer because it help in catching incorrect
// memory references through pagefaults.
// 
// This memory manager can safely cohabitate with any
// other memory manager implementation or with another
// instance of itself, within the same address space,
// as long as memory allocated by a particular instance
// of the memory manager get freed by the same instance
// of the memory manager.
// 
// mmrealloc() is efficient because it is helped internally
// by the field size of each allocated struct mmblock to
// determine the minimum size to copy when relocating a block.
// The size of the useable part of the block is set internally
// when a memory request is made through mmalloc() or mmrealloc().
// 
// Allocating memory through mmalloc() or mmrealloc()
// should always be assumed a successful operation.
// Faillure to allocate memory is a condition that has
// nothing to do with the code using the memory manager,
// but with the system; so memory should always be
// assumed available.
// The memory manager keep trying to request memory
// from the system until there is enough memory
// available to grant the request.
// 
// When providing a pointer to any of the function making up
// the memory manager, a check on the pointer is done to insure
// that the memory pointer was one previously generated by
// the same instance of the memory manager.
// It also help in preventing errors such as double freeing.


#ifndef LIBMM
#define LIBMM

// This function allocate a memory block
// large enough to contain the amount of bytes
// given by the argument size.
// mmalloc(0) will return a valid address that
// can be passed to mmfree(); in other words memory
// is also allocated when doing mmalloc(0).
// The allocated block is added to the current session.
// This function should always be assumed successful.
// The memory address returned is aligned to sizeof(uint).
void* mmalloc (uint size);

// This function call mmalloc() with the size given
// as argument and zero the allocated block with
// bytsetz() before returning it.
void* mmallocz (uint size);

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
void* mmrealloc (void* ptr, uint size);

// This function call mmrealloc() with
// the pointer and size given as argument;
// if the reallocation is increasing the
// size of the memory block, the added
// portion of the memory block get zeroed
// with bytsetz() before returning it.
void* mmreallocz (void* ptr, uint size);

// This function free a previously allocated block.
void mmfree (void* ptr);

// This function return the size with
// which the memory block was allocated.
uint mmsz (void* ptr);

// This function set the memory block destructor
// which when non-null get called before
// the memory block is freed.
// The pointer to the same memory block is returned.
void* mmsetdtor (void* ptr, void(* dtor)(void*));


// Reference counting functions.

// This function increment the reference count
// of the memory block given as argument.
void mmrefup (void* ptr);

// This function decrement the reference count
// of the memory block given as argument, and
// return a pointer to the same memory block.
// If the memory block reference count was 1,
// it get freed and null is returned.
void* mmrefdown (void* ptr);


// Garbage collection is inherently implemented
// through the use of memory sessions.
// 
// mmsessionnew() create a new memory session and
// return a handler for the session; the newly created
// session is a subsession and become the current session.
// Any subsequent call to mmalloc() or mmallocz()
// allocate blocks of memory within the newly created
// session; similarly, any subsequent call to mmsessionnew()
// create subsessions.
// 
// mmsessionextract() move the allocated memory block
// given as argument ptr to the most parent
// of the memory session given as argument.
// 
// mmsessionfree() free the session given as argument;
// any memory allocated within that session is freed.
// The samething is done to subsessions if the argument
// flag == MMDOSUBSESSIONS.
// If the session being freed is the current session,
// the parent session of the session being freed
// become the current session.
// 
// mmsessioncancel() simply cancel the session given
// as argument without freeing any memory allocated in
// in the session; memory blocks allocated within
// the session given as argument, get moved to
// the parent session of the session being canceled;
// the samething is done to subsessions if the argument
// flag == MMDOSUBSESSIONS.
// If the session being canceled is the current session,
// the parent session of the session being canceled
// become the current session.
// 
// mmrealloc() keep a block within
// its session when reallocating it.

typedef struct {
	
	void* _;
	
} mmsession;

mmsession mmsessionnew();

void mmsessionextract (mmsession s, void* ptr);

typedef enum {
	// When used, subsessions are ignored.
	MMIGNORESUBSESSIONS,
	
	// When used, subsessions are processed.
	MMDOSUBSESSIONS
	
} mmsessionflag;

void mmsessionfree (mmsession s, mmsessionflag flag);

void mmsessioncancel (mmsession s, mmsessionflag flag);

// This function return how much memory in bytes has
// been allocated within the session given as argument.
// If flag == MMDOSUBSESSIONS, subsessions are
// included in the computation.
uint mmsessionusage (mmsession s, mmsessionflag flag);

// When this macro is defined calls to mmalloc(),
// mmallocz() and mmrealloc() are replaced by
// their debug equivalent mmdebugalloc(), mmdebugallocz()
// and mmdebugrealloc(); the function mmdebugsession()
// is also made available; those debug functions
// allow to track the origin of memory leaks.
// All part of the program using the memory manager
// must be recompiled so that debugging functions
// substitute regular functions.
// ei: mmdebugalloc() will replace mmalloc().
//#define MMDEBUG

#ifdef MMDEBUG
#define mmalloc(size) mmdebugalloc(size, __FILE__, __LINE__)
#define mmallocz(size) mmdebugallocz(size, __FILE__, __LINE__)
#define mmrealloc(ptr, size) mmdebugrealloc(ptr, size, __FILE__, __LINE__)
#define mmreallocz(ptr, size) mmdebugreallocz(ptr, size, __FILE__, __LINE__)
void* mmdebugalloc (uint size, u8* filename, uint linenumber);
void* mmdebugallocz (uint size, u8* filename, uint linenumber);
void* mmdebugrealloc (void* ptr, uint size, u8* filename, uint linenumber);
void* mmdebugreallocz (void* ptr, uint size, u8* filename, uint linenumber);
#endif

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
		void** backtrace));

// This function return how much memory in bytes
// has been allocated accross all sessions.
uint mmusage();

#endif
