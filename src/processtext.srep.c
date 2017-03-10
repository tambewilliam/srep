
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


void processtext (uint(* callback)(u8*, uint, uint)) {
	// Where the text to process end.
	u8* fend1 = (fstart+fsize);
	u8* fend = (fend1-1);
	
	// Where the line containing the match start.
	u8* lstart;
	
	// Where the line containing the match end.
	// It is important to set lend to zero before
	// processing each file with pamsynmatch1(),
	// because when matchcallback() begin,
	// it use lend to find out if subsequent
	// matches should be added to the same line.
	u8* lend = (u8*)0;
	
	// The limits lstart and lend can
	// enclose more than one line.
	// When doing substitutions they are set so that
	// the entire text area to process is enclosed.
	
	// lastmatchinglinecr is used only when doing
	// substitution and config.altbehavior is non-null.
	// It represent where the newline-character
	// of the last maching line is in memory.
	// Should be set null before a call to pamsynmatch1().
	// It is also important to set lastmatchinglinecr
	// null because matchcallback() use it to remember
	// where the last match line ended when doing substitution.
	u8* lastmatchinglinecr = (u8*)0;
	
	// Keep track of the number of match found
	// if config.matchcount is non-null.
	uint matchcount = 0;
	
	// Variable used to store the pointer argument
	// of a pending call of the callback function,
	// for the rest of the text input from the last
	// element of a pamsynmatchfound linkedlist
	// given as argument to matchcallback
	u8* pendingcallbackarg = (u8*)0;
	
	// Variable used only when
	// (!config.substitute.ptr && config.altbehavior) .
	u8* mprevend = (u8*)0;
	
	// Callback function used with pamsynmatch1().
	uint matchcallback (pamsynmatchfound* m) {
		// This variable will be used only when doing
		// substituion to index the appropriate substitute
		// string to use within the array of pointers
		// to string pointed by config.substitute.ptr .
		// This index should always be null
		// if config.altbehavior is true.
		// It is initialized when doing substitution
		// for the first pamsynmatchfound.
		uint substituteindex;
		
		pamsynmatchfound* p; // Used to save the pointer of the previous pamsynmatchfound.
		pamsynmatchfound* n; // Multipurpose pointer.
		
		// I check if a pamsynmatchfound was given.
		if (!m) {
			// I return 0 to tell pamsynmatch1() to stop looking
			// for matches after config.matchcount match(es) found.
			if (config.matchcount && ++matchcount == config.matchcount) return 0;
			
			// Do not perform a slow lookup,
			// just look for the next match.
			// This is the correct behaviour.
			return 1;
		}
		
		// config.contextlineabove and config.contextlinebelow
		// are set null when either config.substitute.ptr or
		// config.altbehavior is true.
		
		if (config.substitute.ptr) {
			// I get here when doing substitution.
			// If config.altbehavior is true, I make each
			// element of the pamsynmatchfound linkedlist
			// given as argument span an entire line, and
			// dealloc elements that are on the same line
			// with a previous pamsynmatchfound element.
			if (config.altbehavior) {
				// I will be only working with n which
				// will update the linkedlist pointed by m.
				// This way I don't have to restore m to the top
				// of the match linkedlist, since the pointer m
				// is used throughout matchcallback().
				n = m;
				
				// This loop will make each pamsynmatchfound
				// span the entire line where it is found.
				while (1) {
					
					uint o = (n->nochar == 2 && n->start < fend);
					// This test insure that if the
					// pamsynmatchfound->nochar == 2
					// and its fields start and end point
					// to a '\n', then it must span
					// the following line.
					if (o) {
						
						n->end = (n->start += o);
						
						n->nochar = 0;
					}
					
					if (n->start > lastmatchinglinecr) { // Check if it is after the carrier-return of the last matching line.
						// The pamsynmatchfound pointed by n will
						// not be removed when I get here as it is done
						// for the else case of this if() clause.
						
						findlinestart:
						
						if (n->start > fstart) {
							
							if (*--n->start != '\n') goto findlinestart;
							
							// I increment n->start to make sure
							// that n->start be positionned after
							// the newline character where the line start.
							// It is faster to do it here so I don't have
							// to compute substactions twice.
							++n->start;
						}
						
						// Here I make sure not to include
						// the newline character of the line unless
						// it was part of the match otherwise
						// it will get substituted also.
						// I only want to substitute the containt
						// of the line if the match is in the line.
						findlineend:
						
						if (n->end < fend) {
							// The above check guaranty that
							// I can increment n->end without
							// causing a pagefault because there
							// is at least one character ahead.
							
							if (*++n->end != '\n') goto findlineend;
							
							lastmatchinglinecr = n->end;
							
							// I decrement n->end to make sure
							// that it be positionned before
							// the newline character leading to
							// the next line. Because I only want
							// to substitute the content of the line.
							// It is faster to do it here so I don't
							// have to compute additions twice, and
							// it allow to update lastmatchinglinecr
							// without any other ajustments.
							--n->end;
							
						// If the above if() clause is not true,
						// lastmatchinglinecr still need to be set.
						// Notice that lastmatchinglinecr could cause
						// a pagefault if I try to dereference it
						// but I am safe because I will not do that.
						// But it is necessary to have lastmatchinglinecr == n->end +1;
						// so that when the callback function get called again
						// all the testing logic work just fine, because
						// lastmatchinglinecr is supposed to be set at
						// the carriage-return position, but since there
						// is no carriage-return, I assume that there
						// is one right after the end of the file.
						} else lastmatchinglinecr = n->end +1;
						
						// This test insure that a pamsynmatchfound
						// is no longer a nochar if its fields
						// start and end are differents.
						if (n->start != n->end) n->nochar = 0;
						
						if (n->next) {
							
							p = n; // Save the address of the current pamsynmatchfound.
							
							n = n->next;
							
							continue;
							
						} else break;
						
					} else if (n->end > lastmatchinglinecr || (n->end == lastmatchinglinecr && n->nochar != 1)) {
						// If I get here I simply try to set lastmatchinglinecr
						// to the next newline character. Update the position
						// of the string pointed by pendingcallbackarg
						// so that it start after the end of the space
						// to substitute in the file.
						// The pamsynmatchfound pointed by n will be removed
						// from its linkedlist when jumping to removematchobject.
						
						// I need to remember that a pamsynmatchfound
						// can no longer be seen as nochar when it is
						// expanded to span a line; there is no need
						// to update its field nochar since it will
						// get removed from its linkedlist
						// when jumping to removematchobject.
						
						// I check whether I previously set p with
						// a previous pamsynmatchfound block. I do so
						// by simply checking whether n has changed
						// since it was set to the same address as m.
						if (n == m) {
							
							findnextcr:
							
							if (n->end < fend) {
								
								if (*++n->end != '\n') goto findnextcr;
								
								lastmatchinglinecr = n->end;
								
								// I decrement n->end to make sure
								// that it be positionned before
								// the newline character leading to
								// the next line. Because I only want
								// to substitute the content of the line.
								// It is faster to do it here so I don't
								// have to compute additions twice, and
								// it allow to update lastmatchinglinecr
								// without any other ajustments.
								--n->end;
								
							// If the above if() clause is not true,
							// lastmatchinglinecr still need to be set.
							// Notice that lastmatchinglinecr could cause
							// a pagefault if I try to dereference it
							// but I am safe because I will not do that.
							// But it is necessary to have lastmatchinglinecr == n->end +1;
							// so that when the callback function get called again
							// all the testing logic work just fine, because
							// lastmatchinglinecr is supposed to be set at
							// the carriage-return position, but since there
							// is no carriage-return, I assume that there
							// is one right after the end of the file.
							} else lastmatchinglinecr = n->end +1;
							
							// When doing substitution, pendingcallbackarg
							// get set with the rest of the text input start from
							// the previous pamsynmatchfound in the linkedlist.
							// Since the new pamsynmatchfound linkedlist given
							// as argument overlap the last pamsynmatchfound,
							// pendingcallbackarg need to be reset.
							// I only do the reset if I am dealing with the first
							// pamsynmatchfound of the linkedlist given as argument.
							// I can never come here when lend is null, because
							// the very first pamsynmatchfound will make us go to
							// the case where I have if (n->start > lastmatchinglinecr).
							// After that clause is evaluated I will no longer
							// be at the first pamsynmatchfound. The next time
							// this callback function will be called, lend will
							// already have been set to a value different from null.
							// And the then first pamsynmatchfound would be able
							// to get us here; it would be safe since lend would
							// have already been set.
							pendingcallbackarg = n->end +1;
							
							if (n->next) m = n->next;
							// TODO: Make pamsynmatch1() continue searching from (n->end +1).
							// TODO: Update the logic used when (!config.usepatternmatching)
							// so that it will handle the values this function return.
							else return 1; // Will make pamsynmatch1() continue with regular search.
							
						} else {
							// The limits of the previous pamsynmatchfound
							// need to be updated. I used a similar algorithm
							// from the one not far above.
							// Read the comments there to undertstand.
							// But before doing that I need to set the field end
							// of the previous pamsynmatchfound to the field end
							// of the current pamsynmatchfound which was found
							// to be at the current lastmatchinglinecr position
							// or after it. This way I can start from there
							// to search for the next carriage-return character
							// and update lastmatchinglinecr afterward.
							p->end = n->end;
							
							// I need to remember that pamsynmatchfound
							// can no longer be seen as nochar since they are
							// expanded to span a line. Furthermore when
							// I get here the pamsynmatchfound is to be deleted.
							
							findlineend2:
							
							if (p->end < fend) {
								
								if (*++p->end != '\n') goto findlineend2;
								
								lastmatchinglinecr = p->end;
								
								--p->end;
								
							} else lastmatchinglinecr = p->end +1;
							
							// I update the value of the field next
							// of the previous match to the field next
							// of the current match since the current
							// match block pointed by n will be removed.
							// I do not have to check whether p is null
							// (pagefault if it is null) because this code
							// will surely always be executed after p is set.
							// The above if (n == m) clause is meant to detect that.
							p->next = n->next;
						}
						
						goto removematchobject;
						
					} else {
						// If I get here, the matching object is part
						// of the last match line, so I simply remove it.
						
						// This make sure that I update m if there
						// was no previous pamsynmatchfound set in p.
						if (n == m) {
							
							if (n->next) m = n->next;
							else return 1; // Will make pamsynmatch1() continue with regular search.
							
							// It is not necessary to update
							// pendingcallbackarg because
							// lastmatchinglinecr would not have
							// changed and pendingcallbackarg
							// would still be correctly set up.
							
						// I update the value of the field next
						// of the previous match to the field next
						// of the current match since the current
						// match block pointed by n will be removed.
						// I do not have to check whether p is null
						// (pagefault if it is null) because this code
						// will surely always be executed after p is set.
						// The above if (n == m) clause is meant to detect that.
						} else p->next = n->next;
						
						removematchobject:
						
						if (n = n->next) continue;
						else break;
					}
				}
			}
			
		} else {
			// Since I am not going to do any substitution,
			// I need to remove all nochar from the
			// pamsynmatchfound linkedlist given as argument.
			
			n = m;
			
			while (1) {
				
				if (n->nochar) {
					// If n == m then I am the very first
					// pamsynmatchfound of the linkedlist.
					// p is not yet set.
					if (n == m) {
						
						if (n->next) m = n->next;
						else return 1; // Will make pamsynmatch1() continue with regular search.
						
					// I update the value of the field next
					// of the previous match to the field next
					// of the current match since the current
					// match block pointed by n will be removed.
					// I do not have to check whether p is null
					// (pagefault if it is null) because this code
					// will surely always be executed after p is set.
					} else p->next = n->next;
					
					if (n = n->next) continue;
					else break;
					
				} else {
					
					if (n->next) {
						
						p = n; // Save the address of the current pamsynmatchfound.
						
						n = n->next;
						
						continue;
						
					} else break;
				}
			}
		}
		
		if (mprevend) {
			// I get here when
			// (!config.substitute.ptr && config.altbehavior);
			// This logic insure that linename and linenum are
			// prefixed once for matches that are on the same line.
			
			// I can never get here with a pamsynmatchfound
			// that is a nochar because when I am not
			// doing substitutions, I make sure to remove
			// all pamsynmatchfound that are nochar from
			// the linkedlist passed to us by pamsynmatch1().
			
			// I first test for (mprevend < fend) to avoid pagefault
			while (mprevend < fend && *mprevend != '\n') ++mprevend;
			
			if (m->start <= mprevend) goto addmatchingstring;
			// Add a newline for nicer result,
			// since the other characters
			// of the line are to be ignored.
			else if (!callback("\n", 1, 1)) return 0;
		}
		
		// I test if the newly given match is
		// on the same line as the previous match.
		// There is no need to test if lend is non null
		// because the test (m->start <= lend) will be
		// false if lend is null. This test will be true
		// only after lend has been set to a non-null value.
		// This expression comparing against lend could have been:
		// (m->start + (m->nochar == 2 && m->start < lend)) <= lend,
		// but it is ok as-is since when doing substitution
		// lend == fend, and when not doing substitution
		// all nochar are removed.
		if (m->start <= lend) {
			
			if (pendingcallbackarg) {
				
				uint pendingcallbackargsz = (((uint)m->start + (m->nochar == 2)) - (uint)pendingcallbackarg);
				
				if (pendingcallbackargsz)
					if (!callback(
						pendingcallbackarg,
						pendingcallbackargsz,
						0))
						return 0;
				
				pendingcallbackarg = (u8*)0;
			}
			
			lendadjusted:;
			
			uint contextlinebelow = config.contextlinebelow;
			
			// This expression to be compared against lend
			// could have been: ptr = (m->end - (m->nochar != 1)),
			// but it is ok as-is since when doing substitution
			// lend == fend, and when not doing substitution
			// all nochar are removed.
			u8* ptr = m->end;
			
			// If the new match span beyond
			// the end of the line, update lend.
			if (ptr > lend || contextlinebelow) {
				
				lend = ptr;
				
				keeplooking:
				
				// I first increment lend so I can move past
				// the address of the current newline character
				// otherwise the while loop below will
				// directly evaluate to false, because
				// I am still on a newline character.
				// It is safe to increase lend without check
				// because the test (m->end > lend) guaranty
				// that I can move one character ahead
				// without causing a pagefault.
				do ++lend; while (lend < fend && *lend != '\n');
				
				if (contextlinebelow) {
					
					--contextlinebelow;
					
					goto keeplooking;
				}
			}
			
			goto addmatchingstring;
		}
		
		// Variable to be used for
		// caching the value of m->next .
		pamsynmatchfound* mnext;
		
		do {
			if (pendingcallbackarg) {
				
				uint lendnotatnl = (*lend != '\n');
				
				uint pendingcallbackargsz = (((uint)lend - (uint)pendingcallbackarg) + lendnotatnl);
				
				if (pendingcallbackargsz)
					if (!callback(
						pendingcallbackarg,
						pendingcallbackargsz,
						0))
						return 0;
				
				// Display the newline character separately,
				// if it exist, instead of as part of the line,
				// because if the line has non-printable chracters,
				// it may not get outputed by the callback function,
				// and the newline itself will not get outputed.
				if (!lendnotatnl) callback("\n", 1, 1);
				
				pendingcallbackarg = (u8*)0;
			}
			
			// Here lstart and lend are set
			// depending on the config.
			// Note that in the case of substitution,
			// lstart and lend is set in a way that
			// will make the if() clause above always do
			// goto addmatchingstring, and skip this part.
			
			// Substitution has to be tested first.
			if (config.substitute.ptr)  {
				// Note that here, setting lstart with fstart
				// and lend with fend will make the entire
				// text area passed to processtext() displayed
				// with the corresponding substitutions.
				lstart = fstart;
				lend = fend;
				
			} else if (config.altbehavior) {
				// Setting lstart and lend to null will
				// do a strict match which is only done
				// when I am not doing substitutions.
				lstart = (u8*)0;
				//lend = (u8*)0; // lend was already set null.
				
			} else {
				
				lstart = m->start;
				
				uint contextlineabove = config.contextlineabove;
				
				// If lstart was already positioned at
				// a newline character, then it is part of
				// the match, so the logic below is fine.
				keeplooking2:
				
				if (lstart > fstart) {
					
					if (*--lstart != '\n') goto keeplooking2;
					
					if (contextlineabove) {
						
						--contextlineabove;
						
						goto keeplooking2;
					}
					
					// I increment lstart to make sure that
					// lstart be positionned after the newline
					// where the line start
					++lstart;
				}
				
				if (lstart <= lend) {
					// I get here, when the test
					// (m->start <= lend) for
					// the block containing the
					// label "lendadjusted" failed,
					// but the context line above
					// the match is on the same line
					// as the previous match.
					
					u8* ptr = lend +1;
					
					if (!callback(
						ptr,
						((uint)m->start - (uint)ptr),
						0))
						return 0;
					
					lend = m->start -1;
					
					goto lendadjusted;
				}
				
				lend = m->end;
				
				uint contextlinebelow = config.contextlinebelow;
				
				keeplooking3:
				
				// I first test for (lend < fend) to avoid pagefault
				while (lend < fend && *lend != '\n') ++lend;
				
				if (contextlinebelow) {
					
					++lend;
					
					--contextlinebelow;
					
					goto keeplooking3;
				}
				
				// Any leading blank space on the line
				// is removed; having them is not necessary,
				// as it can make the line to display too long.
				while (lstart != lend && (*lstart == ' ' || *lstart == '\t')) ++lstart;
			}
			
			// Possibly insert linenum or linename
			// if substitution is not set.
			if (!config.substitute.ptr) {
				
				if (config.linenameprefix.ptr) {
					
					if (!callback(
						config.linenameprefix.ptr,
						config.linenameprefix.sz,
						1))
						return 0;
				}
				
				if (config.linename.ptr) {
					
					if (!callback(
						config.linename.ptr,
						config.linename.sz,
						0))
						return 0;
					
					if (!callback(":", 1, 1)) return 0;
				}
				
				if (config.linenamesuffix.ptr) {
					
					if (!callback(
						config.linenamesuffix.ptr,
						config.linenamesuffix.sz,
						1))
						return 0;
				}
				
				if (config.linenumprefix.ptr) {
					
					if (!callback(
						config.linenumprefix.ptr,
						config.linenumprefix.sz,
						1))
						return 0;
				}
				
				if (config.showlinenum) {
					// This function count the number of
					// newline character from the position
					// where the first pamsynmatchfound
					// of the linkedlist is located.
					uint newlinecount () {
						
						u8* c = (lstart ?: m->start);
						
						uint n = 1;
						
						while (c > fstart) n += (*--c == '\n');
						
						return n;
					}
					
					u8 s[STRINGMAXDECDIGITS+1];
					
					uint ssz = stringconverttodec(newlinecount(), (u8*)&s);
					
					if (!callback(
						s,
						ssz,
						0))
						return 0;
					
					if (!callback(":", 1, 1)) return 0;
				}
				
				if (config.linenumsuffix.ptr) {
					
					if (!callback(
						config.linenumsuffix.ptr,
						config.linenumsuffix.sz,
						1))
						return 0;
				}
			}
			
			// I also test that (lstart <= m->start)
			// because while adjusting lstart to remove
			// leading blank space on the line, lstart
			// may end up being greater than m->start.
			if (lstart && lstart <= m->start) {
				// lstart can never become greater
				// than (m->start + (m->nochar == 2))
				// so the substraction is always safe.
				uint sz = ((uint)m->start + (m->nochar == 2)) - (uint)lstart;
				
				if (sz)
				if (!callback(
					lstart,
					sz,
					0))
					return 0;
			}
			
			
			addmatchingstring:
			
			if (config.matchprefix.ptr) {
				
				if (!callback(
					config.matchprefix.ptr,
					config.matchprefix.sz,
					1))
					return 0;
			}
			
			// If there is a substitute string, it get
			// inserted otherwise its match get inserted.
			if (config.substitute.ptr) {
				// When the field substitute is set
				// I will always assume that I have
				// at least one valid pointer string in
				// the array of pointer to null-terminated string.
				// So I don't need to check whether
				// subsituteindex < config.substitute.count;
				// before assigning the null-terminated string.
				substituteindex = 0;
				
				uint sz = stringmmsz(config.substitute.ptr[substituteindex]);
				
				if (!callback(
					config.substitute.ptr[substituteindex].ptr,
					sz,
					1))
					return 0;
				
			} else {
				// I can never get here with a pamsynmatchfound
				// that is a nochar because when I am not
				// doing substitutions, I make sure to remove
				// all nochar from the pamsynmatchfound linkedlist
				// given as argument.
				
				if (!callback(
					m->start,
					((uint)m->end - (uint)m->start) +1,
					1))
					return 0;
			}
			
			if (config.matchsuffix.ptr) {
				
				if (!callback(
					config.matchsuffix.ptr,
					config.matchsuffix.sz,
					1))
					return 0;
			}
			
			if (lend) {
				// I add any of the pamsynmatchfound block on
				// the same line than the first pamsynmatchfound
				// passed by pamsynmatch1() through m.
				
				// I add any of the pamsynmatchfound block on
				// the same line than the first pamsynmatchfound
				// passed by pamsynmatch1() through m.
				// This expression comparing against lend could have been:
				// (mnext = m->next) && (mnext->start + (mnext->nochar == 2 && mnext->start < lend)) <= lend,
				// but it is ok as-is since when doing substitution
				// lend == fend, and when not doing substitution
				// all nochar are removed.
				while ((mnext = m->next) && mnext->start <= lend) {
					
					uint contextlinebelow = config.contextlinebelow;
					
					// This expression to be compared against lend
					// could have been: ptr = (mnext->end - (mnext->nochar != 1)),
					// but it is ok as-is since when doing substitution
					// lend == fend, and when not doing substitution
					// all nochar are removed.
					u8* ptr = mnext->end;
					
					// If the match to include span beyond
					// the end of the line, update lend.
					if (ptr > lend || contextlinebelow) {
						
						lend = ptr;
						
						keeplooking4:
						
						// I first increment lend so I can move past
						// the address of the current newline character
						// otherwise the while loop below will
						// directly evaluate to false, because
						// I am still on a newline character.
						// It is safe to increase lend without check
						// because the test (mnext->end > lend)
						// guaranty that I can move one character
						// ahead without causing a pagefault.
						do ++lend; while (lend < fend && *lend != '\n');
						
						if (contextlinebelow) {
							
							--contextlinebelow;
							
							goto keeplooking4;
						}
					}
					
					// ### With GCC, ptr must be computed
					// ### outside of the function-call expression,
					// ### otherwise there is no guaranty that
					// ### its value would have been computed
					// ### by the time it is used by the expression
					// ### of the callback() second argument.
					ptr = (m->end + (m->nochar != 1));
					
					// ptr will never become greater
					// than (mnext->start + (mnext->nochar == 2))
					// so the substraction is always safe.
					uint sz = (((uint)mnext->start + (mnext->nochar == 2)) - (uint)ptr);
					
					if (sz)
					if (!callback(
						ptr,
						sz,
						0))
						return 0;
					
					if (config.matchprefix.ptr) {
						
						if (!callback(
							config.matchprefix.ptr,
							config.matchprefix.sz,
							1))
							return 0;
					}
					
					m = mnext;
					
					if (config.substitute.ptr) {
						// I can only increment substituteindex
						// in order to use the other strings in
						// the array of pointer to null-terminted
						// string pointed by substitute only
						// if config.altbehavior is false, because
						// when it is true, pamsynmatchfound are
						// modified to span the entire line in which
						// they are found and some pamsynmatchfound
						// get deleted; Hence substitute strings defined
						// in the array of pointers wouldn't correspond
						// to the pamsynmatchfound to substitute.
						if (!config.altbehavior) {
							
							if ((substituteindex+1) < config.substitute.count) ++substituteindex;
							// No subsitution is done when there is no substitution left to use.
							else goto donotsubstitute;
						}
						
						uint sz = stringmmsz(config.substitute.ptr[substituteindex]);
						
						if (!callback(
							config.substitute.ptr[substituteindex].ptr,
							sz,
							1))
							return 0;
						
						donotsubstitute:;
						
					} else {
						// I can never get here with a pamsynmatchfound
						// that is a nochar because when I am not
						// doing substitutions, I make sure to remove
						// all pamsynmatchfound that are nochar from
						// the linkedlist passed to us by pamsynmatch1().
						
						if (!callback(
							m->start,
							((uint)m->end - (uint)m->start) +1,
							1))
							return 0;
					}
					
					if (config.matchsuffix.ptr) {
						
						if (!callback(
							config.matchsuffix.ptr,
							config.matchsuffix.sz,
							1))
							return 0;
					}
				}
				
				// If the match is at the very end
				// of the file meaning m->end == lend
				// then m->end+(m->nochar != 1) could cause
				// a pagefault when (m->nochar != 1) evaluate to 1,
				// but since the size (fend1 - pendingcallbackarg)
				// will be zero characters, I am ok.
				pendingcallbackarg = m->end + (m->nochar != 1);
				
			} else {
				// I get here when
				// (!config.substitute.ptr && config.altbehavior);
				// This logic insure that linename and linenum are
				// prefixed once for matches that are on the same line.
				
				// I can never get here with a pamsynmatchfound
				// that is a nochar because when I am not
				// doing substitutions, I make sure to remove
				// all pamsynmatchfound that are nochar from
				// the linkedlist passed to us by pamsynmatch1().
				
				// Note that this lend variable
				// will be local to this scope.
				u8* lend = m->end;
				
				// I first test for (lend < fend) to avoid pagefault
				while (lend < fend && *lend != '\n') ++lend;
				
				// I add any of the pamsynmatchfound block on
				// the same line than the first pamsynmatchfound
				// passed by pamsynmatch1() through m.
				// This expression comparing against lend could have been:
				// (mnext = m->next) && (mnext->start + (mnext->nochar == 2 && mnext->start < lend)) <= lend,
				// but it is ok as-is since when doing substitution
				// lend == fend, and when not doing substitution
				// all nochar are removed.
				while ((mnext = m->next) && mnext->start <= lend) {
					// This expression to be compared against lend
					// could have been: ptr = (mnext->end - (mnext->nochar != 1)),
					// but it is ok as-is since when doing substitution
					// lend == fend, and when not doing substitution
					// all nochar are removed.
					u8* ptr = mnext->end;
					
					// If the match to include span beyond
					// the end of the line, update lend.
					if (ptr > lend) {
						
						lend = ptr;
						
						// I first increment lend so I can move past
						// the address of the current newline character
						// otherwise the while loop below will
						// directly evaluate to false, because
						// I am still on a newline character.
						// It is safe to increase lend without check
						// because (mnext->end > lend) guaranty that
						// I can move one character ahead without
						// causing a pagefault. m->end was set by
						// pamsynmatch1(), so I can trust its value.
						do ++lend; while (lend < fend && *lend != '\n');
					}
					
					if (config.matchprefix.ptr) {
						
						if (!callback(
							config.matchprefix.ptr,
							config.matchprefix.sz,
							1))
							return 0;
					}
					
					m = mnext;
					
					if (!callback(
						m->start,
						((uint)m->end - (uint)m->start) +1,
						1))
						return 0;
					
					if (config.matchsuffix.ptr) {
						
						if (!callback(
							config.matchsuffix.ptr,
							config.matchsuffix.sz,
							1))
							return 0;
					}
				}
				
				mprevend = m->end;
			}
			
		// mnext would have certainly
		// been properly set by the time
		// this while() test occur.
		} while ((m = mnext));
		
		// I return 0 to tell pamsynmatch1()
		// to stop looking for matches after
		// config.matchcount match(es) found.
		if (config.matchcount && ++matchcount == config.matchcount) return 0;
		
		// Return 1 so that the match function
		// continue searching for matches.
		return 1;
	}
	
	if (config.usepatternmatching)
		pamsynmatch1(t, fstart, fsize, matchcallback);
	else {
		u8* ptr = fstart;
		
		uint sz = fsize;
		
		void processmatch () {
			
			u8* ptrpatternsz = ptr + patternsz;
			
			if (!matchcallback(
				&((pamsynmatchfound){
					.start = ptr,
					.end = ptrpatternsz -1,
					.nochar = 0,
					.next = (struct pamsynmatchfound*)0}))) {
				
				sz = 0;
				
				return;
			}
			
			ptr = ptrpatternsz;
			
			sz = (uint)fend1 - (uint)ptr;
		}
		
		if (config.ignorecharcase)
			while (sz > patternsz && (ptr = stringsearchrightignorecase6(ptr, sz, pattern)))
				processmatch();
		else while (sz > patternsz && (ptr = stringsearchright6(ptr, sz, pattern)))
			processmatch();
	}
	
	if (pendingcallbackarg) {
		
		uint lendnotatnl = (*lend != '\n');
		
		uint pendingcallbackargsz = (((uint)lend - (uint)pendingcallbackarg) + lendnotatnl);
		
		if (pendingcallbackargsz)
			callback(
				pendingcallbackarg,
				pendingcallbackargsz,
				0);
		
		// Display the newline character separately,
		// if it exist, instead of as part of the line,
		// because if the line has non-printable chracters,
		// it may not get outputed by the callback function,
		// and the newline itself will not get outputed.
		// Also, since this is the last output of the file,
		// for nicer result, display the newline character
		// if it do not exit and if not doing substitution.
		if (!lendnotatnl || !config.substitute.ptr) callback("\n", 1, 1);
		
	} else if (mprevend) {
		// Add a newline for nicer result,
		// since the other characters
		// of the line are to be ignored.
		callback("\n", 1, 1);
	}
}
