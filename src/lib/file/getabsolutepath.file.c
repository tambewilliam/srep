
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


// The function filegetrelativepath() convert
// an absolute path to a relative path, while
// the function filegetabsolutepath() convert
// a relative path to an absolute path.
// Their argument cwd should be an absolute
// path to the current working directory.
// The argument target of filegetrelativepath()
// should be an absolute path to the target file.
// The argument target of filegetabsolutepath()
// should be a relative path to the target file.
// 
// filegetrelativepath() example:
// cwd:    "/usr/include/kernel/sound"
// target: "/usr/shared/data"
// result: "../../../shared/data"
// 
// filegetabsolutepath() example:
// cwd:    "/usr/include/kernel/sound"
// target: "../../../shared/data"
// result: "/usr/shared/data"

string filegetabsolutepath (u8* cwd, u8* target) {
	
	string cwddup = stringduplicate2(cwd);
	
	string targetdup = stringduplicate2(target);
	
	if (cwddup.ptr[stringmmsz(cwddup) -1] != '/') stringappend4(&cwddup, '/');
	
	if (targetdup.ptr[stringmmsz(targetdup) -1] != '/') stringappend4(&targetdup, '/');
	
	uint i;
	
	// This loop replace all occurrence of "//"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright2(cwddup, "//"))
		stringremove(&cwddup, i - (uint)cwddup.ptr, 1);
	
	// This loop replace all occurrence of "//"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright2(targetdup, "//"))
		stringremove(&targetdup, i - (uint)targetdup.ptr, 1);
	
	// This loop replace all occurrence of "/./"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright2(cwddup, "/./"))
		stringremove(&cwddup, i - (uint)cwddup.ptr, 2);
	
	// This loop replace all occurrence of "/./"
	// by "/" in order to obtain a better result.
	while (i = (uint)stringsearchright2(targetdup, "/./"))
		stringremove(&targetdup, i - (uint)targetdup.ptr, 2);
	
	i = 0;
	
	// I count the number of "../"
	// that start the relative path.
	while (targetdup.ptr[0] == '.' && targetdup.ptr[1] == '.' && targetdup.ptr[2] == '/') {
		
		++i;
		
		stringremove(&targetdup, 0, 3);
	}
	
	// This loop will strip the end of the path in
	// cwddup by the amount of "../" computed in i.
	while (i) {
		
		string dirname = filedirname(cwddup.ptr);
		
		mmfree(cwddup.ptr);
		
		cwddup = dirname;
		
		--i;
	}
	
	if (cwddup.ptr[stringmmsz(cwddup)-1] != '/') stringappend4(&cwddup, '/');
	
	stringappend1(&cwddup, targetdup);
	
	mmfree(targetdup.ptr);
	
	// This loop replace the occurrence of "/./"
	// by "/" in order to obtain a better result,
	// as the argument target could have been
	// a path starting with "./"
	while (i = (uint)stringsearchright2(cwddup, "/./"))
		stringremove(&cwddup, i - (uint)cwddup.ptr, 2);
	
	// I remove "/" from the end of the result.
	stringchop(&cwddup, 1);
	
	return cwddup;
}
