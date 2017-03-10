
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


#include <stdtypes.h>
#include <macros.h>

#if !defined(HASHKIND)
#error HASHKIND not set
#endif

#if !defined(HASHKINDNULL)
#error HASHKINDNULL not set
#endif

// Extra layers of indirection needed
// to get HASHTYPE concatenation.
#define __HASHTYPE(x,y) x##y
#define _HASHTYPE(x,y) __HASHTYPE(x,y)
#define HASHTYPE _HASHTYPE(hash,HASHKIND)

// Hash function which return a digest
// of the memory location given through
// the argument ptr; hasing a byte
// at a time for the count of bytes
// given through the argument sz.
HASHKIND HASHTYPE (u8* ptr, uint sz) {
	
	HASHKIND retvar = HASHKINDNULL;
	
	while (1) {
		
		retvar = HASH(*ptr, retvar);
		
		if (--sz) ++ptr;
		else break;
	}
	
	return retvar;
}
