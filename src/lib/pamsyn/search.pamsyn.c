
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


/* This function will return the address of the first
and last non-hidden character of what was matched,
using the pamsyntokenized on the memory region given
through the arguments start and size.
Remember that the meta-character '#' is used to hide
a match from the result.
A null pamsynsearched is returned if a match
could not be found.
The argument t cannot be null. */
pamsynsearched pamsynsearch(pamsyntokenized t, u8* start, uint size) {
	// This variable will contain what is to be returned.
	pamsynsearched result = {0, 0};
	
	// Callback function used with pamsynfind().
	uint processmatch(pamsynfound* m) {
		// I check if a pamsynfound was given to us;
		// if not I return 2 to tell pamsynfind() to do a slow lookup.
		if (!m) return 2;
		
		// This skip pamsynfound that are nochar.
		while (m->nochar) {
			if (!(m = m->next)) {
				// If the pamsynfound linked list was only made out of nochar,
				// I return 0 to tell pamsynfind() to stop looking for matches.
				return 0;
			}
		}
		
		// Here I set the address of the first non-hidden character.
		result.start = m->start;
		
		// Here I find the last non-hidden character.
		
		result.end = m->end;
		
		while (m = m->next) {
			// I skip pamsynfound that are nochar.
			if (!m->nochar) result.end = m->end;
		}
		
		// I return 0 to tell pamsynfind() to stop looking for matches.
		return 0;
	}
	
	pamsynfind(t, start, size, processmatch);
	
	return result;
}
