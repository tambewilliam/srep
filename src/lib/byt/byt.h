
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


#ifndef LIBBYT
#define LIBBYT

// Compare the two memory regions
// given as argument and return null
// if they match, otherwise return the address
// of the mismatch from the memory region
// given through argument ptr1.
// The number of bytes to compare is given by
// the argument sz and if it is null, 0 is returned.
// The arguments ptr1 and ptr2 cannot be null.
void* bytcmp (void* ptr1, void* ptr2, uint sz);

// Copy the memory region pointed by src
// to the memory region pointed by dest,
// taking into account that they may overlap.
// If size is null, nothing is done.
void bytcpy (void* dest, void* src, uint size);

// Fill with the byte given by c,
// the memory location having
// the given size and pointed by ptr.
void bytset (void* ptr, uint size, u8 c);

// Fill with zero-valued bytes the memory location
// having the given size and pointed by ptr.
void bytsetz (void* ptr, uint size);

#endif
