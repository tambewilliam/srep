
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


// This function store in the file
// for which the path is given as
// argument, the data given through
// the arguments ptr and sz.
// The file is created if it do not exist.
// Null is returned if a failure
// occured, otherwise 1 is returned.
// Note that a failure can occur even
// when data has been written to the file;
// in such a case, the failure can be
// due to all the data not having been
// written out to the file or to an error
// while setting the new size of the file
// after having overwritten its content.
uint filestore (u8* path, void* ptr, uint sz, filestoreflag flag) {
	
	uint fd;
	
	// open() return -1 on error.
	if ((fd = open(path, O_WRONLY|O_CREAT, 0600)) != -1) {
		// If I get here I have successfully opened the file.
		
		uint noerror;
		
		// If I am appending data to the file,
		// I seek the end of the file.
		if (flag == FILESTOREAPPEND) {
			
			if (lseek(fd, 0, SEEK_END) == -1) noerror = 0;
			else noerror = 1;
			
		} else noerror = 1;
		
		if (noerror) {
			// Null should be returned
			// if all the data could not
			// be written to the file.
			if (write(fd, ptr, sz) != sz) noerror = 0;
			
			if (noerror && flag == FILESTOREOVERWRITE) {
				// If I was overwriting the data in the file,
				// I truncate it to the size that was written.
				// Note that the truncation is being done
				// after the write to the file, as it yield
				// a faster result because internally, the
				// kernel wouldn't need to deallocate pages
				// of the file that will contain the new data.
				if (ftruncate(fd, sz) == -1) noerror = 0; // ftruncate() return -1 on error.
			}
		}
		
		// Close the file.
		close(fd);
		
		return noerror;
		
	} else return 0;
}
