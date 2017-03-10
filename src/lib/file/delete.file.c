
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


// This function delete a file.
// If the file to delete is a folder,
// and flag == FILEDELETEONLY, it must be empty;
// otherwise if flag == FILEDELETECONTENT,
// its content is deleted.
// Null is returned on failure.
uint filedelete (u8* path, filedeleteflag flag) {
	
	if (fileis(path) != FILEISFOLDER) return !unlink(path);
	else {
		// This function recursively
		// delete the content of
		// the folder given as argument.
		void dofoldercontent(u8* path) {
			
			if (chdir(path) == 0) {
				
				DIR* dirfd;
				
				if (dirfd = opendir(".")) {
					
					struct dirent* direntry;
					
					while (direntry = readdir(dirfd)) {
						
						u8* direntryname = direntry->d_name;
						
						if (direntryname[0] == '.' &&
							(direntryname[1] == '.' || !direntryname[1]))
							continue;
						
						if (fileis(direntryname) != FILEISFOLDER) unlink(direntryname);
						else {
							dofoldercontent(direntryname);
							
							rmdir(direntryname);
						}
					}
					
					closedir(dirfd);
				}
				
				chdir("..");
			}
		}
		
		if (flag == FILEDELETECONTENT) {
			// Save current working directory to restore it later.
			// In fact if path is multiple subfolders deep,
			// the current working directory will change
			// upen returning from dofoldercontent().
			u8* cwd = filegetcwd();
			
			dofoldercontent(path);
			
			// Restoring current working directory.
			chdir(cwd);
			
			mmfree(cwd);
		}
		
		return !rmdir(path);
	}
}
