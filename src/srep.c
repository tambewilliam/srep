
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


// Used for files handling.
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Used for mmap()/munmap().
#include <sys/mman.h>

// Used for printf().
#include <stdio.h>

#include <stdtypes.h>
#include <mm.h>
#include <parsearg.h>
#include <pamsyn.h>
#include <string.h>
#include <file.h>
#include <byt.h>


// Size by which stdin buffer is
// increased when not large enough.
#define SIZESTDINBUFFER 512

// Escape sequence used to print
// colored output in the console.
#define CONSOLEREDCOLOR "\033[1;31m"
#define CONSOLEGREENCOLOR "\033[0;32m"
#define CONSOLEMAGENTACOLOR "\033[0;35m"
#define CONSOLENORMALCOLOR "\033[0m"

u8* helptext =
"Usage: srep [OPTIONS] pattern [FILES]\n\n\
\
	The pattern is searched on stdin if no files is given as argument.\n\n\
\
Options:\n\n\
 -a	Alternate behavior that show only the parts\n\
	of a line that matches; and when doing\n\
	substitution, the entire line containing\n\
	the match is substituted.\n\n\
\
 -b	Binary files are treated as text files.\n\
	When doing substitution, binary files\n\
	are skipped unless this option is used.\n\n\
\
 -c	Turn on coloring.\n\n\
\
 -e PAMSYN\n\
	Return the pamsyn with all metacharacters escaped.\n\
	The pamsyn is taken from stdin if no argument is given.\n\n\
\
 -h	Display this help.\n\n\
\
 -i	ignore characters' case.\n\n\
\
 -k	Do not skip symbolic links to file.\n\n\
\
 -l	Prefix each line of output with its line number\n\
	Ignored when doing substitution.\n\n\
\
 -n COUNT\n\
	Only output COUNT matches from each given file,\n\
	or only do COUNT substitutions.\n\n\
\
 -N COUNT_ABOVE_AND_BELOW\n\
 -N COUNT_ABOVE,COUNT_BELOW\n\
	Number of context lines to display\n\
	above and/or below the line\n\
	where a match was found.\n\
	Ignored when doing substitution\n\
	or when option -a is used.\n\
\
 -p	The given search pattern is a pamsyn.\n\n\
\
 -r 	Recursively process files in directories and subdirectories.\n\n\
\
 -R 	Recursively process files in directories and subdirectories,\n\
	following symbolic links.\n\n\
\
 -s SUBSTITUTE\n\
	Substitute the match with SUBSTITUTE in each file\n\
	given as argument. If the input is stdin or given\n\
	through the option -t, the output goes to stdout.\n\
	More than one option -s can be used when using a pamsyn;\n\
	for each portion of a match, there has to be a corresponding\n\
	option -s, otherwise nothing is substituted for that\n\
	particular portion of the match.\n\
	ei: Two option -s can be used to surround the word \"this\"\n\
	wherever it will be found, using the pamsyn \".#{this}.\" .\n\
	In the above example, there are two portions in the match:\n\
	before and after the word \"this\"; so for each portion\n\
	of the match, there has to be a corresponding option -s,\n\
	otherwise nothing is substituted for that particular\n\
	portion of the match.\n\
	If the option -a is used, more than one option -s\n\
	is useless; the additional option -s are ignored\n\
	and all portions of a match are substituted\n\
	using the same option -s.\n\
	When found in the option -s argument,\n\
	the following escape sequences are interpreted:\n\
	'\\n': Newline.\n\
	'\\t': Tab.\n\
	'\\xx': Where xx is a two digits hexadecimal\n\
		number that translate to a byte value.\n\
	'\\\\': Escape the character '\\' which\n\
		is used for escaping.\n\n\
\
 -t TEXT\n\
	The pattern is searched on TEXT instead\n\
	of stdin or files given as arguments.\n\n\
\
 -x PATH\n\
	Exclude paths ending with PATH.\n\
	More than one -x can be given.\n\n\
\
 -X PATHPAMSYN\n\
	Exclude paths for which the ending\n\
	match the pamsyn PATHPAMSYN.\n\
	More than one -X can be given.\n";


// TODO: Implement processing data through stdin
// as they come, instead of waiting to read all
// stdin data before commencing processing.
// When reading from stdin, use the pattern minimum-span
// to minimize blocking-on-read.
// TODO: Implement option -S which will take
// as argument a command to execute; and each
// null terminated output from the command
// is used as substitute.
// TODO: When multiple option -S are used,
// each null terminated output from their command
// is used for subsequent portion of the match
// in the same manner as it is done with -s.
// TODO: Note that the command provided by
// the option -S can stay running; providing
// null terminated substitute as needed.
// If it terminate, it get restarted again
// if substitution is needed.
// If it terminate right after outputing data
// that was not terminated by a null character,
// the termination is taken as the end of the output,
// and it get restarted again if substitution is needed.
// TODO: The command started by the option -S
// will receive through its stdin, the match string
// for which it was called, allowing it to output
// a substitute string that is based on the match.
// TODO: The same escape sequences interpreted with option -s
// will be interpreted with the outputs from option -S .
sint main (sint argc, u8** arg) {
	// Structure holding various
	// configuration information
	// used during runtime.
	struct {
		
		u8* texttouseasinput;
		uint treatbinfileastext;
		uint color;
		uint usepatternmatching;
		uint ignorecharcase;
		uint recurse;
		uint recursedereferencing;
		uint filedereferencing;
		uint showlinenum;
		uint showfilename;
		uint matchfound;
		
		// When set to 1, ignore the other characters
		// of the line and return only the match found.
		// Each match is separated by a new line.
		// If substitute is set, the entire line
		// containing the match is substituted.
		uint altbehavior;
		
		// Number of context lines to display
		// respectively above and below the line
		// where a match was found.
		uint contextlineabove;
		uint contextlinebelow;
		
		// Array of string used for substitutition.
		// Substitutition take place if
		// the field substitute.ptr is non-null.
		// Note that when a substitute is set, the entire
		// text area passed to processtext() through
		// the arguments start and size will be returned
		// through one more calls of the processtext()
		// callback function, along with substitions done.
		// If this field is not set, only the interesting
		// parts (ei: Line containing the match) of
		// the entire text area are returned by
		// processtext() through its callback function.
		struct {
			string* ptr; // Pointer to an array of string.
			uint count; // Number of string in the array.
		} substitute;
		
		// Array of string used to exclude paths
		// for which the ending match.
		struct {
			string* ptr; // Pointer to an array of string.
			uint count; // Number of string in the array.
		} exclude;
		
		// Array of pamsyntokenized used to exclude
		// paths for which the ending match.
		struct {
			pamsyntokenized* ptr; // Pointer to an array of pamsyntokenized.
			uint count; // Number of string in the array.
		} excludepamsyn;
		
		// If non-null, stop searching for
		// match after count match found.
		uint matchcount;
		
		// If the field ptr of linename
		// is non-null the string is prefixed
		// to the line containing the match.
		// linenum is prefixed first before linename
		// get to be prefixed if both are given.
		// Ignored when substitute is set.
		struct {
			u8* ptr;
			uint sz;
		} linename;
		
		// Strings to prefix
		// and suffix to linename.
		struct {
			u8* ptr;
			uint sz;
		} linenameprefix, linenamesuffix;
		
		// Strings to prefix
		// and suffix to linenum.
		struct {
			u8* ptr;
			uint sz;
		} linenumprefix, linenumsuffix;
		
		// Strings to prefix and suffix
		// to every match or substitute.
		struct {
			u8* ptr;
			uint sz;
		} matchprefix, matchsuffix;
		
	} config;
	
	bytsetz((void*)&config, sizeof(config));
	
	u8* pattern;
	uint patternsz; // Used only when (!config.usepatternmatching).
	
	pamsyntokenized t;
	
	// Used to locate the loaded file in memory.
	u8* fstart;
	uint fsize;
	
	#include "processtext.srep.c"
	
	void parseargcallback (u8 option, u8* value) {
		// readnum() result.
		uint readnumresult;
		
		// Read a decimal value
		// from the memory location
		// pointed by "value".
		// Overflow is ignored.
		void readnum () {
			
			readnumresult = 0;
			
			while (*value >= '0' && *value <= '9') {
				
				readnumresult = (readnumresult*10) + (*value-'0');
				
				++value;
			}
		}
		
		// I decrease argc since this argument
		// will be removed from the array
		// of arguments pointed by arg.
		--argc;
		
		if (option == 'p') config.usepatternmatching = 1;
		else if (option == 'i') config.ignorecharcase = 1;
		else if (option == 'a') config.altbehavior = 1;
		else if (option == 'b') config.treatbinfileastext = 1;
		else if (option == 'c') config.color = 1;
		else if (option == 'l') {
			// I turn on the prefixing of the line number.
			// Prefixing the line number is skipped within
			// processtext() when doing substitution.
			config.showlinenum = 1;
			
		} else if (option == 'r') config.recurse = -1;
		else if (option == 'R') config.recursedereferencing = -1;
		else if (option == 'k') config.filedereferencing = 1;
		else if (option == 'e') {
			
			if (value) fprintf(stdout, "%s", pamsynescape(value).ptr);
			else {
				// If I get here, I read from STDIN.
				
				fsize = 0; // Keep track of how many bytes I read.
				fstart = 0; // Need to be set null otherwise mmrealloc() will pagefault.
				
				uint i;
				
				do {
					if (!(fstart = mmrealloc(fstart, fsize + SIZESTDINBUFFER))) {
						
						fprintf(stderr, "srep: failed allocating STDIN buffer\n");
						
						_exit(1); // The shell use 1 for failing.
					}
					
					if ((i = read(STDIN_FILENO, fstart + fsize, SIZESTDINBUFFER)) == -1) {
						
						fprintf(stderr, "srep: error reading STDIN\n");
						
						_exit(1); // The shell use 1 for failing.
					}
					
					fsize += i; // Increase the number of bytes read.
					
				} while (i); // If the number of byte read is NULL, then I am done reading.
				
				// If nothing was read from STDIN I should throw an error.
				if (!fsize) {
					
					fprintf(stderr, "srep: nothing read from STDIN\n");
					
					_exit(1); // The shell use 1 for failing.
				}
				
				// Set the null terminating character.
				((u8*)mmrealloc(fstart, fsize + 1))[fsize] = 0;
				
				fprintf(stdout, "%s", pamsynescape(fstart).ptr);
				
				// I do not need to free the memory allocated
				// from reading stdin, because when this program
				// terminate, the kernel destroy its address space.
			}
			
			fflush(stdout);
			
			_exit(0);
			
		} else if (option == 'n') {
			
			if (value) {
				
				readnum();
				
				// When I get here, I should see
				// a null char and "readnumresult"
				// must be non-null, otherwise
				// I have an error.
				if (!*value && readnumresult)
					config.matchcount = readnumresult;
				else goto errorparsingoptionn;
				
			} else {
				
				errorparsingoptionn:
				
				fprintf(stderr, "srep: -n  expecting non-null argument\n");
				
				_exit(1); // The shell use 1 for failing.
			}
			
		} else if (option == 'N') {
			
			if (value) {
				
				readnum();
				
				// When I get here, I should see
				// a null char otherwise I have an error.
				if (*value == ',') {
					
					config.contextlineabove = readnumresult;
					
					++value;
					
					readnum();
					
					if (*value) goto errorparsingoptionnn;
					
					config.contextlinebelow = readnumresult;
					
				} else if (*value) goto errorparsingoptionnn;
				else {
					
					config.contextlineabove = readnumresult;
					
					config.contextlinebelow = readnumresult;
				}
				
			} else {
				
				errorparsingoptionnn:
				
				fprintf(stderr, "srep: -N  expecting valid argument\n");
				
				_exit(1); // The shell use 1 for failing.
			}
			
		} else if (option == 's') {
			// I test the variable value to avoid adding
			// null pointers to the array of pointers.
			if (value) {
				
				string s = stringnull;
				
				if (*value) {
					
					do {
						if (*value == '\\') {
							
							++value;
							
							if (*value == 'n') stringappend4(&s, '\n');
							else if (*value == 't') stringappend4(&s, '\t');
							else if ((*value >= '0' && *value <= '9') || (*value >= 'a' && *value <= 'f')) {
								
								u8 c;
								
								// Convert the first digit.
								if (*value >= '0' && *value <= '9')
									c = (*value - '0');
									
								else c = (*value - 'a' + 10);
								
								++value;
								
								// Convert the second digit.
								if (*value >= '0' && *value <= '9')
									c = ((c<<4) + (*value - '0'));
									
								else if (*value >= 'a' && *value <= 'f')
									c = ((c<<4) + (*value - 'a' + 10));
									
								else {
									
									fprintf(stderr, "srep: invalid escape sequence in -s argument\n");
									
									_exit(1); // The shell use 1 for failing.
								}
								
								stringappend4(&s, c);
								
							} else if (*value == '\\') stringappend4(&s, '\\');
							else {
								
								fprintf(stderr, "srep: invalid escape sequence in -s argument\n");
								
								_exit(1); // The shell use 1 for failing.
							}
							
						} else stringappend4(&s, *value);
						
						++value;
						
					} while (*value);
					
				} else s = stringduplicate2(""); // s must be non-null, otherwise a pagefault occur during substitution.
				
				// Increase the field count since
				// I will add a new string to the array.
				++config.substitute.count;
				
				config.substitute.ptr =
					mmrealloc(config.substitute.ptr, config.substitute.count*sizeof(u8*));
				
				config.substitute.ptr[config.substitute.count-1] = s;
				
			} else {
				
				fprintf(stderr, "srep: -s expecting argument\n");
				
				_exit(1); // The shell use 1 for failing.
			}
			
		} else if (option == 'x') {
			// I test the variable value to avoid adding
			// null pointers to the array of pointers.
			if (value) {
				
				if (*value) {
					// Increase the field count since
					// I will add a new string to the array.
					++config.exclude.count;
					
					config.exclude.ptr =
						mmrealloc(config.exclude.ptr, config.exclude.count*sizeof(u8*));
					
					config.exclude.ptr[config.exclude.count-1] = stringduplicate2(value);
				}
				
			} else {
				
				fprintf(stderr, "srep: -x expecting argument\n");
				
				_exit(1); // The shell use 1 for failing.
			}
			
		} else if (option == 'X') {
			// I test the variable value to avoid adding
			// null pointers to the array of pointers.
			if (value) {
				
				if (*value) {
					// Increase the field count since I will
					// add a new pamsyntokenized to the array.
					++config.excludepamsyn.count;
					
					config.excludepamsyn.ptr =
						mmrealloc(config.excludepamsyn.ptr, config.excludepamsyn.count*sizeof(u8*));
					
					string s = stringfmt("%s>", value);
					
					pamsyntokenized t = pamsyntokenize(s.ptr);
					
					if (!t.ptr) {
						
						fprintf(stderr, "srep: invalid %luth exclude pamsyn\n", (unsigned long)config.excludepamsyn.count);
						
						_exit(1); // The shell use 1 for failing.
					}
					
					mmfree(s.ptr);
					
					config.excludepamsyn.ptr[config.excludepamsyn.count-1] = t;
				}
				
			} else {
				
				fprintf(stderr, "srep: -x expecting argument\n");
				
				_exit(1); // The shell use 1 for failing.
			}
			
		} else if (option == 't') {
			
			if (config.texttouseasinput) {
				
				fprintf(stderr, "srep: -t cannot be used more than once\n");
				
				_exit(1); // The shell use 1 for failing.
				
			} else if (value) config.texttouseasinput = value;
			else {
				fprintf(stderr, "srep: -t expecting argument\n");
				
				_exit(1); // The shell use 1 for failing.
			}
			
		} else if (option == 'h') {
			
			fprintf(stderr, "%s", helptext);
			
			_exit(0);
			
		} else {
			
			fprintf(stderr, "srep: invalid option '%c'\n", option);
			
			_exit(1); // The shell use 1 for failing.
		}
	}
	
	// Parse the command line for any option given.
	parseargshort1((u8**)arg, parseargcallback, "enNstxX");
	
	++arg; // I move to the address of the first argument, which is the pattern.
	
	// I test if the pattern was given.
	if (!*arg || !**arg) {
		
		fprintf(stderr, "srep: no pattern given: -h for help\n");
		
		_exit(1); // The shell use 1 for failing.
	}
	
	// Used with the syscall fstat().
	struct stat statbuffer;
	
	if (config.usepatternmatching) {
		
		if (config.ignorecharcase) {
			
			pattern = stringfmt("_{%s}", *arg).ptr;
			
		} else pattern = *arg;
		
		if (!(t = pamsyntokenize(pattern)).ptr) {
			
			fprintf(stderr, "srep: invalid pattern\n");
			
			_exit(1); // The shell use 1 for failing.
		}
		
	} else {
		
		pattern = *arg;
		
		patternsz = stringsz(pattern);
	}
	
	// config.contextlineabove and config.contextlinebelow
	// are to be set null when either config.substitute.ptr
	// or config.altbehavior is true.
	if (config.substitute.ptr || config.altbehavior) {
		
		config.contextlineabove = 0;
		config.contextlinebelow = 0;
	}
	
	// This function return 0 if the region
	// given by the arguments text and size
	// contain a non-printable character;
	// otherwise 1 is returned.
	uint isprintabletext (void* text, uint size) {
		
		void* limit = text + size;
		
		uint textlessthanlimit = (text < limit);
		
		while (textlessthanlimit) {
			
			void* text1 = text+1;
			
			textlessthanlimit = (text1 < limit);
			
			u8 c = *(u8*)text;
			
			if (c != '\t' && c != '\n' && c != '\r' &&
				(c < 0x20 || c > 0x7e) &&
				// These test whether it
				// is a utf8 character.
				(((c&0xc0) != 0x80) && // Test if it is a subsequent byte of multi-byte.
				// If it was not a subsequent byte of a multi-byte,
				// test if it is the first byte of a multi-byte.
				(textlessthanlimit && ((*(u8*)text1&0xc0) != 0x80))))
				return 0;
			
			text = text1;
		}
		
		return 1;
	}
	
	if (config.texttouseasinput) {
		
		fstart = config.texttouseasinput;
		fsize = stringsz(fstart);
		
		goto processtextandwritetostdout;
		
	// I move to the address of the first file
	// if it was given and test if at least a file
	// was given, otherwise I read from STDIN.
	} else if (*++arg) {
		// I turn on the prefixing of the filename.
		// Prefixing the filename is skipped within
		// processtext() when doing substitution.
		config.showfilename = 1;
		
		// For each file specified
		// in the command line
		// I need to process them.
		do {
			// This create a new memory session.
			// Calling mmsessionfree() will free
			// any memory that was allocated
			// since mmsessionnew() was called.
			// The use of the memory session
			// remove the need to track all
			// the allocation made, since it is
			// sometime impossible to do so.
			mmsession matchsession = mmsessionnew();
			
			// Callback function used with filelsdir().
			uint filelsdircallback (filelsdirtype type, string path) {
				
				uint i = 0;
				
				// This loop check whether the file
				// is to be excluded testing against
				// all option -x that were given.
				while (i < config.exclude.count) {
					
					string configexcludeptri = config.exclude.ptr[i];
					
					uint configexcludeptrisz = stringmmsz(configexcludeptri);
					
					uint pathsz = stringmmsz(path);
					
					if (configexcludeptrisz <= pathsz &&
						stringiseq4(
							configexcludeptri.ptr,
							path.ptr+(pathsz-configexcludeptrisz),
							configexcludeptrisz))
						return -1; // Skip file.
					
					++i;
				}
				
				i = 0;
				
				// This loop check whether the file
				// is to be excluded testing against
				// all option -X that were given.
				while (i < config.excludepamsyn.count) {
					
					if (pamsynmatch2(
						config.excludepamsyn.ptr[i],
						path.ptr,
						stringmmsz(path)).start)
						return -1; // Skip file.
					
					++i;
				}
				
				if (type == FILELSDIRTYPEDIRECTORY) {
					
					if (config.recurse || config.recursedereferencing) return 1;
					else return -1; // Skip folder.
				}
				
				// Files other than regular are skipped.
				if (type == FILELSDIRTYPEOTHER) return -1;
				else if (type == FILELSDIRTYPESYMLINK) {
					// Note that stat() follow symlink.
					if (stat(path.ptr, &statbuffer) == 0) {
						
						if (S_ISDIR(statbuffer.st_mode)) {
							
							if (config.recursedereferencing) return 1;
							else return -1; // Skip dereferencing to folder.
							
						} else if (!config.filedereferencing) return -1; // Skip dereferencing to file.
						
					} else {
						
						if (config.filedereferencing || config.recursedereferencing)
							fprintf(stderr, "srep: stat() failed on file \"%s\"\n", path.ptr);
						
						return -1; // Skip bad symlink.
					}
				}
				
				uint fdin;
				
				// open() return -1 on error.
				// It might not always be possible to open
				// a file readwrite; so readwrite is used
				// only if a substitution is to be done.
				if ((fdin = open(path.ptr, config.substitute.ptr ? O_RDWR : O_RDONLY)) != -1) {
					
					if (fstat(fdin, &statbuffer) == -1) {
						
						fprintf(stderr, "srep: fstat() failed on file \"%s\"\n", path.ptr);
						
					} else {
						// Skip file if empty.
						if (!(fsize = statbuffer.st_size)) goto skipfile;
						
						// When not doing substitution,
						// mmap() is used faster result,
						// as it can avoid the copy that
						// occur when using read().
						if (config.substitute.ptr) {
							
							fstart = mmalloc(fsize);
							
							if (read(fdin, fstart, fsize) != fsize) {
								
								fprintf(stderr, "srep: read() failed on file \"%s\"\n", path.ptr);
								
								mmfree(fstart);
								
								goto skipfile;
							}
							
						} else if ((fstart = mmap(0, fsize, PROT_READ, MAP_SHARED, fdin, 0)) == (u8*)-1) {
							
							fprintf(stderr, "srep: mmap() failed on file \"%s\"\n", path.ptr);
							
							goto skipfile;
						}
						
						uint fdout;
						
						if (config.substitute.ptr) {
							
							fdout = fdin;
							
							// I disable colors if it was set,
							// since I am going to be working
							// with a file and I do not want
							// the escape sequence for coloring
							// to spill in the file.
							config.color = 0;
							
						} else {
							
							fdout = STDOUT_FILENO;
							
							// I disable coloring if it was set,
							// and the output is not a tty device;
							// since it will not be able to interpret
							// the coloring escape sequence.
							if (config.color) config.color = isatty(STDOUT_FILENO);
						}
						
						// Prefixing the filename is skipped within
						// processtext() when doing substitution.
						if (config.showfilename) {
							
							config.linename.ptr = path.ptr;
							config.linename.sz = stringmmsz(path);
						}
						
						// If coloring is set, I set the strings
						// that will be used to add coloring.
						if (!config.linenameprefix.ptr && config.color) {
							
							config.linenameprefix.ptr = (u8*)CONSOLEMAGENTACOLOR;
							config.linenameprefix.sz = stringsz(CONSOLEMAGENTACOLOR);
							config.linenamesuffix.ptr = (u8*)CONSOLENORMALCOLOR;
							config.linenamesuffix.sz = stringsz(CONSOLENORMALCOLOR);
							
							config.linenumprefix.ptr = (u8*)CONSOLEGREENCOLOR;
							config.linenumprefix.sz = stringsz(CONSOLEGREENCOLOR);
							config.linenumsuffix.ptr = (u8*)CONSOLENORMALCOLOR;
							config.linenumsuffix.sz = stringsz(CONSOLENORMALCOLOR);
							
							config.matchprefix.ptr = (u8*)CONSOLEREDCOLOR;
							config.matchprefix.sz = stringsz(CONSOLEREDCOLOR);
							config.matchsuffix.ptr = (u8*)CONSOLENORMALCOLOR;
							config.matchsuffix.sz = stringsz(CONSOLENORMALCOLOR);
						}
						
						// Variable which get set to 1 for
						// the first match found in the file
						// currently being processed.
						uint matchfound = 0;
						
						// Callback function used by
						// processtext() to output its results.
						uint processtextcallback (u8* ptr, uint sz, uint noneedtocheckifprintable) {
							// Set config.matchfound since
							// I found at least one match.
							config.matchfound = 1;
							
							if (!matchfound) {
								// I check whether the file contain
								// a non-printeable character only
								// if binary files are not to be
								// treated as text files.
								if (!config.treatbinfileastext && !isprintabletext(fstart, fsize)) {
									
									if (config.showfilename)
										fprintf(stderr, "srep: binary file: %s\n", path.ptr);
									else fprintf(stderr, "srep: binary file\n");
									
									return 0;
								}
								
								if (config.substitute.ptr && fdout != STDOUT_FILENO) {
									
									if (ftruncate(fdout, 0) != -1) lseek(fdout, 0, SEEK_END);
									else {
										fprintf(stderr, "srep: ftruncate() failed on \"%s\"\n", path.ptr);
										
										return 0;
									}
								}
								
								matchfound = 1;
							}
							
							// I output the result if doing substitution,
							// or if the file is not a binary file
							// (when I get here with !config.treatbinfileastext,
							// the file has already being checked that
							// it is not a binary file), or if there is no need
							// to check if it is printable, or if the result
							// to output do not contain a non-printable character.
							if (config.substitute.ptr || !config.treatbinfileastext ||
								noneedtocheckifprintable || isprintabletext(ptr, sz)) {
								
								if (write(fdout, ptr, sz) == -1) {
									
									if (fdout != STDOUT_FILENO)
										fprintf(stderr, "srep: failed to write to \"%s\"\n", path.ptr);
									else fprintf(stderr, "srep: failed to write to stdout\n");
									
									return 0;
								}
							}
							
							return 1;
						}
						
						processtext(processtextcallback);
						
						if (config.substitute.ptr)
							mmfree(fstart);
						else	munmap(fstart, fsize);
						
						skipfile:;
					}
					
					close(fdin);
					
				} else fprintf(stderr, "srep: open(%s) failed on file \"%s\"\n", config.substitute.ptr ? "O_RDWR" : "O_RDONLY", path.ptr);
				
				return 1;
			}
			
			string path = stringduplicate2(*arg);
			
			if (!filelsdir(path, filelsdircallback)) {
				fprintf(stderr, "srep: cannot access \"%s\"\n", path.ptr);
			}
			
			// Free any memory that was allocated while processing the file.
			mmsessionfree(matchsession, MMDOSUBSESSIONS);
			
			++arg;
			
		} while (*arg);
		
		_exit(!config.matchfound);
		
	} else {
		// I read from STDIN.
		
		fsize = 0; // Keep track of how many bytes I read.
		fstart = 0; // Need to be set null otherwise mmrealloc() cause a pagefault.
		
		uint n;
		
		do {
			if (!(fstart = mmrealloc(fstart, fsize + SIZESTDINBUFFER))) {
				
				fprintf(stderr, "srep: failed to allocate buffer for STDIN\n");
				
				_exit(1); // The shell use 1 for failing.
			}
			
			if ((n = read(STDIN_FILENO, fstart + fsize, SIZESTDINBUFFER)) == -1) {
				
				fprintf(stderr, "srep: error while reading STDIN\n");
				
				_exit(1); // The shell use 1 for failing.
			}
			
			fsize += n;
			
		} while (n);
		
		if (!fsize) {
			
			fprintf(stderr, "srep: nothing was read from STDIN\n");
			
			_exit(1); //The shell use 1 for failing.
		}
		
		processtextandwritetostdout:
		
		// I disable coloring if it was set,
		// and the output is not a tty device;
		// since it will not be able to interpret
		// the coloring escape sequence.
		if (config.color) config.color = isatty(STDOUT_FILENO);
		
		if (config.color) {
			
			config.linenameprefix.ptr = (u8*)CONSOLEMAGENTACOLOR;
			config.linenameprefix.sz = stringsz(CONSOLEMAGENTACOLOR);
			config.linenamesuffix.ptr = (u8*)CONSOLENORMALCOLOR;
			config.linenamesuffix.sz = stringsz(CONSOLENORMALCOLOR);
			
			config.linenumprefix.ptr = (u8*)CONSOLEGREENCOLOR;
			config.linenumprefix.sz = stringsz(CONSOLEGREENCOLOR);
			config.linenumsuffix.ptr = (u8*)CONSOLENORMALCOLOR;
			config.linenumsuffix.sz = stringsz(CONSOLENORMALCOLOR);
			
			config.matchprefix.ptr = (u8*)CONSOLEREDCOLOR;
			config.matchprefix.sz = stringsz(CONSOLEREDCOLOR);
			config.matchsuffix.ptr = (u8*)CONSOLENORMALCOLOR;
			config.matchsuffix.sz = stringsz(CONSOLENORMALCOLOR);
		}
		
		// Callback function used by
		// processtext() to output its results.
		uint processtextcallback (u8* ptr, uint sz, uint noneedtocheckifprintable) {
			// Set config.matchfound since
			// I found at least one match.
			config.matchfound = 1;
			
			// I output the result if doing substitution,
			// or if there is no need to check if it is printable,
			// or if the result to output do not contain
			// a non-printable character.
			if (config.substitute.ptr ||
				noneedtocheckifprintable ||
				isprintabletext(ptr, sz)) {
				
				if (write(STDOUT_FILENO, ptr, sz) == -1) {
					
					fprintf(stderr, "srep: failed to write to stdout\n");
					
					return 0;
				}
			}
			
			return 1;
		}
		
		processtext(processtextcallback);
		
		_exit(!config.matchfound);
	}
	
	// No need to manually free any memory still
	// allocated, because the program terminate,
	// the kernel destroy its address space.
}
