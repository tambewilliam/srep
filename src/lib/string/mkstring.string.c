
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


// Return a variable of type string for which the field ptr has
// been set with the null terminated string given as argument
// and the size computed in its field sz.
// The u8* argument cannot be a null or invalid pointer.
string stringmk (u8* s) {
	return (string) {.ptr = s, .sz = (s ? stringlen(s) : 0)};
}


// Return a variable of type string which has its
// fields ptr and sz set with the corresponding argument.
string stringmk2 (u8* ptr, uint sz) {
	return (string) {.ptr = ptr, .sz = sz};
}


// Return a variable of type string which has
// its fields ptr and sz set using the string
// given as argument.
string stringmk3 (string s) {
	return (string) {.ptr = s.ptr, .sz = s.sz};
}
