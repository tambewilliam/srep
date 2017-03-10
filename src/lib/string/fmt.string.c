
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


// Generate a string from the string
// given by the argument fmt, substituting
// anywhere format specifiers are used.
// For each format specifier used,
// other than %%, there must be
// corresponding arguments in
// the order in which they are used
// in the string argument fmt.
// A format specifier syntax is
// %[flags][width]specifier where
// items in brackets are optionals,
// and "specifier" is either
// i for a signed decimal number,
// d for an unsigned decimal number,
// x for an unsigned hexadecimal number,
// o for an unsigned octal number,
// b for an unsigned binary number,
// c for a character,
// s for a null-terminated string.
// "[width]" is the minimum number
// of characters for the substitute
// of the format specifier;
// if the value to substitute has
// less characters, pading is done.
// ei: "value is :%10d".fmt(22);
// yield "value is :        22".
// "[flags]" can be in any order "|" or "'c'".
// When it is "|", the value that substitute
// the format specifier is left aligned
// (instead of right aligned) within
// the given field width.
// When it is "'c'" where c is one or more characters,
// the first character is used for padding instead
// of the default space character, and the subsequent
// characters are padding overwrites between
// paddings and the value to substitute. ei:
// "As follow %'-> ['10d]".fmt(22);
// yield "As follow -----> [22]".
// In other to use the single quote character
// within the flag "'c'", it must be escaped
// using a backslash; ei:
// "As follow %'-> \\''10d]".fmt(22);
// yield "As follow -----> '22]";
// and the backslash character itself is used
// by escaping it using backslash as well; ei:
// "As follow %'-> \\\\'10d]".fmt(22);
// yield "As follow -----> \22]";
string stringfmt (u8* fmt, ...) {
	
	va_list ap;
	
	va_start(ap, fmt);
	
	string retvar = stringduplicate2("");
	
	while (*fmt) {
		// Variable set to the address
		// of the default character to
		// use for padding the substitute
		// of the format specifier.
		u8* pad = " ";
		
		// Variable which when non-null
		// mean that the substitute of
		// the format specifier will be
		// padded to the left, making it
		// aligned to the right; when null
		// the substitute of the format
		// specifier will be padded
		// to the right, making it
		// aligned to the left.
		uint rightalign = 1;
		
		// Variable set to the minimum
		// number of characters for the
		// substitute of the format specifier;
		// if the value of the format specifier
		// has less characters, pading is done.
		uint width = 0;
		
		// Variable keeping the count of characters
		// that are padding overwrites between
		// paddings and the value to substitute.
		uint substituteextrasz = 0;
		
		if (*fmt == '%') {
			
			++fmt;
			
			if (*fmt == '%') {
				
				stringappend4(&retvar, '%');
				
				++fmt;
				
				continue;
			}
			
			// Parse the flag of the format specifier if any.
			while (1) {
			
				if (*fmt == '|') {
					
					rightalign = 0;
					
					++fmt;
					
				} else if (*fmt == '\'') {
					
					++fmt;
					
					if (*fmt == '\\') ++fmt;
					
					// I set the charater to use for padding.
					pad = fmt;
					
					++fmt;
					
					while (1) {
						
						if (*fmt == '\'') {
							
							++fmt;
							
							break;
							
						} else if (*fmt == '\\') {
							
							substituteextrasz += 1;
							
							fmt += 2;
							
						} else {
							
							substituteextrasz += 1;
							
							++fmt;
						}
					}
					
				} else break;
			}
			
			// Read the minimum number of characters
			// for the substitute of the format specifier.
			while (*fmt >= '0' && *fmt <= '9') {
				
				width = (width*10) + (*fmt - '0');
				
				++fmt;
			}
			
			enum {MAXCNTOFDIGITS = (sizeof(uint)*8)};
			
			switch (*fmt) {
				
				case 'i' :{
					// Retrieve the argument value
					// to convert to a string.
					uint i = va_arg(ap, uint);
					
					// Array of bytes that will contain
					// the characters of the decimal number
					// plus a sign and null terminating character.
					u8 s[MAXCNTOFDIGITS+2];
					
					// Variable that will be used
					// to index the array a.
					uint j = MAXCNTOFDIGITS;
					
					// Variable set to 1
					// if i is less than 0.
					uint neg = (sint)i < 0;
					
					if (neg) i = -i;
					
					while (i >= 10) {
						
						s[j] = (i%10) + '0';
						
						--j;
						
						i /= 10;
					}
					
					s[j] = i + '0';
					
					if (neg) s[--j] = '-';
					
					s[MAXCNTOFDIGITS+1] = 0;
					
					uint n = ((MAXCNTOFDIGITS+1) - j) + substituteextrasz;
					
					// Compute padding amount.
					if (width > n)
						i = width - n;
					else i = 0;
					
					// Pad to the left
					// if right alignment.
					if (rightalign && i) {
						
						u8 c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
					}
					
					stringappend2(&retvar, &s[j]);
					
					// Pad to the right
					// if left alignment.
					if (!rightalign && i) {
						
						u8 c;
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
						
						c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
					}
				}
				break;
				
				case 'd' :{
					// Retrieve the argument value
					// to convert to a string.
					uint i = va_arg(ap, uint);
					
					// Array of bytes that will contain
					// the characters of the decimal number
					// plus a null terminating character.
					u8 s[MAXCNTOFDIGITS+1];
					
					// Variable that will be used
					// to index the array s.
					uint j = MAXCNTOFDIGITS-1;
					
					while (i >= 10) {
						
						s[j] = (i%10) + '0';
						
						--j;
						
						i /= 10;
					}
					
					s[j] = i + '0';
					
					s[MAXCNTOFDIGITS] = 0;
					
					uint n = (MAXCNTOFDIGITS - j) + substituteextrasz;
					
					// Compute padding amount.
					if (width > n)
						i = width - n;
					else i = 0;
					
					// Pad to the left
					// if right alignment.
					if (rightalign && i) {
						
						u8 c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
					}
					
					stringappend2(&retvar, &s[j]);
					
					// Pad to the right
					// if left alignment.
					if (!rightalign && i) {
						
						u8 c;
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
						
						c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
					}
				}
				break;
				
				case 'x' :{
					// Retrieve the argument value
					// to convert to a string.
					uint i = va_arg(ap, uint);
					
					// Array of bytes that will contain
					// the characters of the hexadecimal number
					// plus a null terminating character.
					u8 s[MAXCNTOFDIGITS+1];
					
					// Variable that will be used
					// to index the array s.
					uint j = MAXCNTOFDIGITS-1;
					
					while (i >= 16) {
						
						uint ii = i%16;
						
						if (ii < 10) s[j] = ii + '0';
						else s[j] = (ii - 10) + 'a';
						
						--j;
						
						i >>= 4; // Do a division by 16.
					}
					
					if (i < 10) s[j] = i + '0';
					else s[j] = (i - 10) + 'a';
					
					s[MAXCNTOFDIGITS] = 0;
					
					uint n = (MAXCNTOFDIGITS - j) + substituteextrasz;
					
					// Compute padding amount.
					if (width > n)
						i = width - n;
					else i = 0;
					
					// Pad to the left
					// if right alignment.
					if (rightalign && i) {
						
						u8 c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
					}
					
					stringappend2(&retvar, &s[j]);
					
					// Pad to the right
					// if left alignment.
					if (!rightalign && i) {
						
						u8 c;
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
						
						c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
					}
				}
				break;
				
				case 'o' :{
					// Retrieve the argument value
					// to convert to a string.
					uint i = va_arg(ap, uint);
					
					// Array of bytes that will contain
					// the characters of the octal number
					// plus a null terminating character.
					u8 s[MAXCNTOFDIGITS+1];
					
					// Variable that will be used
					// to index the array s.
					uint j = MAXCNTOFDIGITS-1;
					
					while (i >= 8) {
						
						s[j] = (i%8) + '0';
						
						--j;
						
						i >>= 3; // Do a division by 8.
					}
					
					s[j] = i + '0';
					
					s[MAXCNTOFDIGITS] = 0;
					
					uint n = (MAXCNTOFDIGITS - j) + substituteextrasz;
					
					// Compute padding amount.
					if (width > n)
						i = width - n;
					else i = 0;
					
					// Pad to the left
					// if right alignment.
					if (rightalign && i) {
						
						u8 c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
					}
					
					stringappend2(&retvar, &s[j]);
					
					// Pad to the right
					// if left alignment.
					if (!rightalign && i) {
						
						u8 c;
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
						
						c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
					}
				}
				break;
				
				case 'b' :{
					// Retrieve the argument value
					// to convert to a string.
					uint i = va_arg(ap, uint);
					
					// Array of bytes that will contain
					// the characters of the binary number
					// plus a null terminating character.
					u8 s[MAXCNTOFDIGITS+1];
					
					// Variable that will be used
					// to index the array s.
					uint j = MAXCNTOFDIGITS-1;
					
					while (i >= 2) {
						
						s[j] = (i%2) + '0';
						
						--j;
						
						i >>= 1; // Do a division by 2.
					}
					
					s[j] = i + '0';
					
					s[MAXCNTOFDIGITS] = 0;
					
					uint n = (MAXCNTOFDIGITS - j) + substituteextrasz;
					
					// Compute padding amount.
					if (width > n)
						i = width - n;
					else i = 0;
					
					// Pad to the left
					// if right alignment.
					if (rightalign && i) {
						
						u8 c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
					}
					
					stringappend2(&retvar, &s[j]);
					
					// Pad to the right
					// if left alignment.
					if (!rightalign && i) {
						
						u8 c;
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
						
						c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
					}
				}
				break;
				
				case 'c' :{
					
					uint n = 1 + substituteextrasz;
					
					uint i;
					
					// Compute padding amount.
					if (width > n)
						i = width - n;
					else i = 0;
					
					// Pad to the left
					// if right alignment.
					if (rightalign && i) {
						
						u8 c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
					}
					
					stringappend4(&retvar, (u8)va_arg(ap, uint));
					
					// Pad to the right
					// if left alignment.
					if (!rightalign && i) {
						
						u8 c;
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
						
						c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
					}
				}
				break;
				
				case 's' :{
					
					u8* s = va_arg(ap, u8*);
					
					uint n = stringsz(s) + substituteextrasz;
					
					uint i;
					
					// Compute padding amount.
					if (width > n)
						i = width - n;
					else i = 0;
					
					// Pad to the left
					// if right alignment.
					if (rightalign && i) {
						
						u8 c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
					}
					
					stringappend2(&retvar, s);
					
					// Pad to the right
					// if left alignment.
					if (!rightalign && i) {
						
						u8 c;
						
						if (substituteextrasz) {
							
							u8* p = pad+1;
							
							while ((c = *p) != '\'') {
								
								if (c == '\\') c = *++p;
								
								stringappend4(&retvar, c);
								
								++p;
							}
						}
						
						c = *pad;
						
						do stringappend4(&retvar, c);
						while (--i);
					}
				}
				break;
			}
			
		} else stringappend4(&retvar, *fmt);
		
		++fmt;
	}
	
	va_end(ap);
	
	return retvar;
}
