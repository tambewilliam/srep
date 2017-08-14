
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


// Compare the two memory regions
// given as argument and return null
// if they match, otherwise return the address
// of the mismatch from the memory region
// given through argument ptr1.
// The number of bytes to compare is given by
// the argument sz and if it is null, 0 is returned.
// The arguments ptr1 and ptr2 cannot be null.
void* bytcmp (void* ptr1, void* ptr2, uint sz) {
	// If size is null I assume
	// both memory region equal.
	if (!sz) return 0;
	
	while (1) {
		
		if (*(u8*)ptr1 != *(u8*)ptr2) return ptr1;
		
		if (!--sz) return (void*)0;
		
		++ptr1; ++ptr2;
	}
}
