
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


/* The functions below convert a string having
an unsigned decimal, hexadecimal,
octale or binary form, to an integer.
The string representation of the integer number
should not contain spaces, and any character
must be lower case.
0 is returned if an incorrect character
is found or if there is an overflow.
Here below are examples of strings accepted:
Decimal: 234
Hexadecimal: 0xaa13
Octale: 0o1234
Binary: 0b101110
The u8* argument cannot be a null or invalid pointer. */

uint stringconverttoint2(u8* s) {
	/* This algorithm is faster when using null terminated strings because
	I don't need to check if the current position during the parsing is going
	to go beyond the size of the string. I simply make use of the null
	terminated character as an incorrect character in order to stop. */
	
	uint i = 0, savedi = 0;
	
	if (*s == '0') {
		
		s++;
		
		if (*s == 'x') {
			// If I get here the string pointed by s is a hexadecimal number.
			
			s++;
			
			while (1) {
				
				if (*s >= '0' && *s <= '9') {
					savedi = i;
					i = (i<<4) + (*s - '0'); // <<4 does a multiplication by 16.
					
				} else if (*s >= 'a' && *s <= 'f') {
					savedi = i;
					i = (i<<4) + ((*s - 'a') + 10); // <<4 does a multiplication by 16.
					
				} else break;
				
				// If i is lesser than its previously saved value then an overflow occured.
				if (i < savedi) return 0;
				
				s++;
			}
			
		} else if (*s == 'b') {
			// If I get here the string pointed by s is a binary number.
			
			s++;
			
			while (*s == '0' || *s == '1') {
				
				savedi = i;
				
				i = (i<<1) + (*s - '0'); // <<1 does a multiplication by 2.
				
				// If i is lesser than its previously saved value then an overflow occured.
				if (i < savedi) return 0;
				
				s++;
			}
			
		} else if (*s == 'o') {
			// If I get here the string pointed by s must be an octale number.
			
			while (*s >= '0' && *s <= '7') {
				
				savedi = i;
				
				i = (i<<3) + (*s - '0'); // <<3 does a multiplication by 8.
				
				// If i is lesser than its previously saved value then an overflow occured.
				if (i < savedi) return 0;
				
				s++;
			}
			
		/* If the base of the number cannot
		be determined, it could be a decimal
		number that started with 0. */
		} else goto parsedecimalnumber;
		
	} else {
		// If I get here the string pointed by s is a decimal number.
		
		parsedecimalnumber:
		
		while (*s >= '0' && *s <= '9') {
			
			savedi = i;
			
			i = (i*10) + (*s - '0');
			
			// If i is lesser than its previously saved value then an overflow occured.
			if (i < savedi) return 0;
			
			s++;
		}
	}
	
	// If *s is not null then I have an incorrect
	// character and I should return 0.
	if (*s) return 0;
	else return i;
}

uint stringconverttoint1(string s) {
	/* The string given as argument to this function,
	cannot be assumed null terminated, but the duplicated
	string is guarantied to be null terminated. */
	
	string duplicate = stringduplicate1(s);
	
	uint retvar = stringconverttoint2(duplicate.ptr);
	
	mmfree(duplicate.ptr);
	
	return retvar;
}
