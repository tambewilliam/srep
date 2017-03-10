
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


/* This function search for a match and pass a match found
in the form of a pamsynfound to the specified callback function
everytime a match is found. Note that it can happen that
a null pointer is passed to the callback function, and that
happen when a match occured but there was no non-hidden character
in the match; so it is important for the callback function
to make sure that its argument is not a null pointer.
- When the callback function return 0, this function
stop looking for matches.
- When the callback function returns 1, this function
look for the next match starting after the previous match.
If the match found was a nochar, this function will ignore
the return value of the callback function and perform
a slow lookup as described below; this is because
nochar match have no size.
- when the callback function returns >1, this function
look for the next match as if it didn't find a match, meaning
without positioning itself after the last character of
the previous match.
The callback function pointer must be non-null and valid.
The argument size cannot be null.
The callback function do not need to worry about freeing
the pamsynfound object that it receive as it will be done
by pamsynfind() upon the callback function returning.
The argument t cannot be null. */
void pamsynfind(pamsyntokenized t, u8* start, uint size, uint (*callback)(pamsynfound*)) {
	// Variable to keep starting and current position
	// within the area I am matching against.
	u8* startpos;
	u8* curpos;
	
	// Keep the address of the current token being processed.
	pamsyntoken* curtoken;
	
	/* Keep the addresses of created duplicates.
	Those duplicates are pointers to array of pointers
	to tokens. It is necessary to initialize it to zero. */
	pamsyntoken*** duplicates = 0;
	uint szduplicates = 0;
	
	// Compare a token and a string.
	uint cmp() {
		
		uint true, false;
		
		// Reverse the meaning of true and false
		// if the token was within !{} .
		if (curtoken->flags.isnot) {
			true = 0;
			false = 1;
			
		} else {
			true = 1;
			false = 0;
		}
		
		// Check first if the token was representing
		// any of the metachars '<' or '>'
		
		// Check if the token represent the metacharacter '<' .
		if (curtoken->flags.isfirst) {
			if (curpos == start) return true; else return false;
		}
		
		// Check if the token represent the metacharacter '>' .
		if (curtoken->flags.islast) {
			if (curpos == start+size) return true; else return false;
		}
		
		/* I compare the string only if a string was attached
		to the field str of the token.
		I return 1 so I can skip when I had used a metachar '?'
		or the token get ignored if its string size was null. */
		if (!curtoken->str.ptr) return 1;
		
		uint i = 0;
		
		// Check if the token represent a character set
		// within brackets. ei: [af-j];
		if (curtoken->flags.ischarset) {
			/* When the field ischar of a token is set,
			I should have at least a single set of 2 characters,
			that's why I can use a do while() loop here. */
			do {
				if (*curpos >= curtoken->str.ptr[i] && *curpos <= curtoken->str.ptr[i+1])
					return true;
				
			} while (curtoken->str.ptr[i += 2]);
			
			return false;
		}
		
		// If I get here the token has a string in its field str
		// that I compare against the text at curpos.
		
		i = curtoken->str.sz;
		
		if (i) {
			
			u8* ptr1 = curtoken->str.ptr;
			
			u8* ptr2 = curpos;
			
			while (1) {
				
				if (*ptr1 != *ptr2) break;
				
				if (!--i) break;
				
				ptr1++; ptr2++;
			}
		}
		
		if (i) return false;
		else return true;
	}
	
	/* Create a duplicate of the set of tokens to repeat and
	set curtoken to the first token of the duplicated set.
	curtoken need to hold valid repeat informations.
	1 is returned if duplication took place otherwise 0 is
	returned if I completely worked out a duplicate.
	This is the most complicate part of the entire library. */
	uint dup() {
		uint spaceleft;
		
		// Update the field sel and count to what will be the next
		// repeat to do or set the count field to an appropriate value.
		void update() {
			// keep in mind that I can have something like +1,8{a} or +8,1{a} and
			// that is the reason why I evaluate the field count less than and greater than.
			if (curtoken->repeat.count < curtoken->repeat.seq[curtoken->repeat.sel+1]) {
				curtoken->repeat.count++;
				// If the field repeat.count is bigger than spaceleft I can go ahead
				// and select another repeat sequence. It is useless to continue with the same sequence.
				if (curtoken->repeat.count > spaceleft) goto updatesel;
				
			} else if (curtoken->repeat.count > curtoken->repeat.seq[curtoken->repeat.sel+1]) {
				curtoken->repeat.count--;
				// If the field repeat.count is bigger than spaceleft, set count to the space left
				// only if is it bigger than the second limit of the sequence.
				if (curtoken->repeat.count > spaceleft) {
					if (spaceleft > curtoken->repeat.seq[curtoken->repeat.sel+1]) {
						curtoken->repeat.count = spaceleft;
					} else goto updatesel;
				}
				
			} else {
				updatesel:
				/* If I get here
				curtoken->repeat.count == curtoken->repeat.seq[curtoken->repeat.sel+1]
				I need to update the field sel to the next repeat sequence. */
				curtoken->repeat.sel += 2;
				
				// I set the field count to the lower value of the new repeat sequence
				// only if the field sel < szseq
				if (curtoken->repeat.sel < curtoken->repeat.szseq) {
					
					curtoken->repeat.count = curtoken->repeat.seq[curtoken->repeat.sel];
					
					if (curtoken->repeat.count > spaceleft) {
						if (curtoken->repeat.count > curtoken->repeat.seq[curtoken->repeat.sel+1]
							&& spaceleft > curtoken->repeat.seq[curtoken->repeat.sel+1]) {
							
							curtoken->repeat.count = spaceleft;
							
						} else goto updatesel;
					}
					
				} // The else case is tested outside of this function.
			}
		}
		
		/* Before exiting dup() after a successful duplication
		I call update(); so here I need to make sure that after
		the previous call to update() the field repeat.sel was not
		equal to the field repeat.szseq, because when it is the case
		it mean that the entire repeat sequence had been completely
		worked out and I should reset the field repeat.sel
		and return null. */
		
		// If the field sel == szseq then I cannot duplicate the token,
		// because it has been worked out.
		if (curtoken->repeat.sel == curtoken->repeat.szseq) {
			// I need to reinitialize the field sel and count so that
			// the repeat information of curtoken can still be processed if I ever come back.
			curtoken->repeat.sel = 0;
			curtoken->repeat.count = curtoken->repeat.seq[0];
			return 0;
		}
		
		/* "+3{+0-9{a}}" is similar to "+0-9{a}+0-9{a}+0-9{a}" but
		if applied to a text such as "a", the pattern "+3{+0-9{a}}" will
		fail while the pattern "+0-9{a}+0-9{a}+0-9{a}" will succeed.
		This is because a repetition number cannot be larger than
		the space left to match even if within the repetition brace
		they could be a pattern which can match a space lesser than
		the repetition size as it is the case for "+3{+0-9{a}}", but
		it is hard to write a code which can detect that. */
		spaceleft = size-((uint)curpos-(uint)start);
		
		/* If the field repeat.count is greater than spaceleft,
		I should call update() to set it to a reasoneable value, because
		I may have a very large value in the field repeat.count whereas
		spaceleft is small and it can take much more processing to create
		all the duplicate until I finish processing the repeat sequence. */
		if (curtoken->repeat.count > spaceleft) update();
		
		/* Because I called update() right above, I need to check and
		make sure that the field repeat.sel is not equal to the field
		repeat.szseq, because when it is the case the field repeat.count
		is not set and what should be done is to reset
		the field repeat.sel and return null. */
		if (curtoken->repeat.sel == curtoken->repeat.szseq) {
			// I need to reinitialize the field sel and count so that
			// the repeat information of curtoken can still be processed if I ever come back.
			curtoken->repeat.sel = 0;
			curtoken->repeat.count = curtoken->repeat.seq[0];
			return 0;
		}
		
		// When I get here the field repeat.count has a correct value
		// and I can proceed to peform duplication.
		
		// I find out where the repeat start.
		uint m = 0; while (curtoken->holder[m] != curtoken) m++;
		// I find out where the repeat end, looking at the field head.
		uint n = m; while (curtoken->holder[n]->head != curtoken) n++;
		
		pamsyntoken** duptok;
		
		// I assume two cases depending on the value of count.
		if (curtoken->repeat.count) {
			
			uint s1 = (n-m)+1;
			
			// Create enough space to hold the duplicate set.
			uint j = s1*curtoken->repeat.count;
			
			// j+1 to account for the first slot in the duplicate set
			// that will hold the number of tokens in the duplicate.
			duptok = mmalloc((j+1)*sizeof(pamsyntoken*));
			duptok[0] = (pamsyntoken*)j; // save the size.
			
			// Advance past the slot of the size.
			duptok++;
			
			do {
				j--;
				
				duptok[j] = mmalloc(sizeof(pamsyntoken));
				
			} while (j);
			
			// j is zero when I get here.
			
			uint s2 = n-m;
			
			// This loop control go through the entire array of duplicates
			// created in the loop above starting from the last duplicate.
			while (j < curtoken->repeat.count) {
				
				uint i = 0;
				
				// This loop go through a single duplicate in the array
				// starting from the last token of a duplicate.
				while (i <= s2) {
					// x will correspond to a token in the array
					// containing the duplicates.
					uint x = (s1*(curtoken->repeat.count-j))-(i+1);
					
					// y will correspond to a token in the set being duplicated. 
					uint y = n-i;
					
					duptok[x]->holder = duptok;
					duptok[x]->str.ptr = curtoken->holder[y]->str.ptr;
					duptok[x]->str.sz = curtoken->holder[y]->str.sz;
					duptok[x]->flags = curtoken->holder[y]->flags;
					// duptok[x]->prev = 0; // I don't need to set this field for speed.
					duptok[x]->strcmpid = curtoken->holder[y]->strcmpid;
					duptok[x]->repeat.seq = curtoken->holder[y]->repeat.seq;
					duptok[x]->repeat.szseq = curtoken->holder[y]->repeat.szseq;
					duptok[x]->repeat.sel = curtoken->holder[y]->repeat.sel;
					duptok[x]->repeat.count = curtoken->holder[y]->repeat.count;
					
					uint k;
					
					/* Return the position of a token in curtoken->holder .
					A valid address that exist in the array of pointers
					to tokens pointed by curtoken->holder should be passed
					to this function, otherwise a pagefault is likely to occur. */
					uint find(pamsyntoken* t) {
						
						uint i = 0;
						
						while (curtoken->holder[i] != t) i++;
						
						return i;
					}
					
					// I need to rewrite the address saved in the fields next.
					uint z = find(curtoken->holder[y]->next);
					
					/* If (z > n) then the field next point probably to what
					was supposed to be after a single duplicate set; else
					the field "next" point to a token within the duptok set. */
					if (z > n) {
						// This will detect the last duplicate set in duptok.
						if (j) {
							/* I could have used the expression:
							k = (s1*(curtoken->repeat.count-(j+1)))+(z-m);
							which is used in the else case of the block if (z > n),
							but the expression below is faster. */
							k = s1*(curtoken->repeat.count-j);
							
							duptok[x]->next = duptok[k];
							
							/* I need to set the repeat field of the first token
							of a repeated set otherwise while processing it,
							a duplicate of what I am duplicating will be done again;
							but it is not supposed to happen.
							Keep in mind that a token that has its field seq set
							always has its field head set to null; I mention that
							because it is important for the field head to be null
							when not in use. */
							duptok[k]->repeat.seq = 0;
							
							// For speed, it not necessary to set the fields below.
							// duptok[k]->head = 0;
							// duptok[k]->repeat.szseq = 0;
							// duptok[k]->repeat.sel = 0;
							// duptok[k]->repeat.count = 0;
							
						} else duptok[x]->next = curtoken->holder[n]->next;
						
					} else {
						
						k = (s1*(curtoken->repeat.count-(j+1)))+(z-m);
						
						duptok[x]->next = duptok[k];
					}
					
					// I rewrite the field alt.
					if (curtoken->holder[y]->alt) {
						
						z = find(curtoken->holder[y]->alt);
						
						// k is calculated using z calculated above. I use the same formular as
						// the one used above to calculate the field next when z is not > n.
						k = (s1*(curtoken->repeat.count-(j+1)))+(z-m);
						
						duptok[x]->alt = duptok[k];
						
					} else duptok[x]->alt = 0;
					
					// I rewrite the field head.
					if (curtoken->holder[y]->head) {
						
						z = find(curtoken->holder[y]->head);
						
						// k is calculated using z calculated above. I use the same formular as
						// the one used above to calculate the field next when z is not > n.
						k = (s1*(curtoken->repeat.count-(j+1)))+(z-m);
						
						duptok[x]->head = duptok[k];
						
					} else duptok[x]->head = 0;
					
					i++;
				}
				
				j++;
			}
			
		} else {
			/* I create a token string which will do nothing.
			2*sizeof(pamsyntoken*) to account for the space needed
			to save the size of the tokenstring */
			duptok = mmalloc(2*sizeof(pamsyntoken*));
			duptok[0] = (pamsyntoken*)1; // The tokenstring has only 1 element.
			
			duptok++; // Position the pointer where the first token will be saved.
			
			// I allocate memory for the only token in the duplicate
			duptok[0] = mmalloc(sizeof(pamsyntoken));
			
			duptok[0]->holder = duptok;
			
			duptok[0]->str.ptr = 0;
			duptok[0]->str.sz = 0;
			
			duptok[0]->flags.isnot = 0;
			duptok[0]->flags.ishidden = 0;
			duptok[0]->flags.isfirst = 0;
			duptok[0]->flags.islast = 0;
			duptok[0]->flags.isnochar = 0;
			duptok[0]->flags.isany = 0;
			duptok[0]->flags.ischarset = 0;
			
			duptok[0]->strcmpid = 0;
			
			duptok[0]->alt = 0;
			
			duptok[0]->next = curtoken->holder[n]->next;
			
			// It is not necessary to set the field seq to zero because it will not
			// be checked once curtoken is set as the first token of the duplicate.
		}
		
		/* I need to save in the field head of the first token
		of the duplicate, the address of the token holding
		repeating informations. freedup() will use it to set curtoken
		so that the next duplicate can be done.
		The token holding repeating informations never has
		an alternate token attached to it. */
		
		/* I set the field head of the first token in the duplicate.
		I don't need to worry about any alternate token attached to
		that first token because there will never be one. */
		duptok[0]->head = curtoken;
		
		/* It is not necessary to set to zero the field seq of the first
		token in the duplicate; in fact when dup() is called, curtoken
		is set to the first token of the duplicate and that token will
		not see its field seq checked; so it is safe
		to comment out this line. */
		// duptok[0]->repeat.seq = 0;
		
		// For speed, I don't need to update this below.
		// duptok[0]->repeat.szseq = 0;
		// duptok[0]->repeat.sel = 0;
		// duptok[0]->repeat.count = 0;
		
		update();
		
		/* I set the field prev of the first token in the duplicate
		to the prev field of the token pointed by curtoken.
		In fact at this point duptok[0] is a duplicate of what is
		in curtoken and can be seen as one token instead of
		two token attached to each other. */
		duptok[0]->prev = curtoken->prev;
		
		// I set curtoken to the first token in duptok.
		curtoken = duptok[0];
		
		// I save the address of the duplicate.
		szduplicates++;
		duplicates = mmrealloc(duplicates, szduplicates*sizeof(pamsyntoken**));
		duplicates[szduplicates-1] = duptok;
		
		return 1;
	}
	
	// Free the duplicate set and set curtoken appropriatly.
	// Called only when curtoken point to the first token of a duplicate set.
	void freedup() {
		uint i;
		pamsyntoken** th;
		
		th = curtoken->holder;
		curtoken = curtoken->head;
		
		// Retrieve the size which is saved in the first slot
		// in the arrary of pointers to pamsyntoken.
		i = (uint)th[-1];
		
		do {
			i--;
			
			mmfree(th[i]);
			
		} while (i);
		
		mmfree(&th[-1]);
		
		szduplicates--;
		
		duplicates = mmrealloc(duplicates, szduplicates*sizeof(pamsyntoken**));
	}
	
	// Free all created duplicates.
	void freealldup() {
		uint i;
		
		while (szduplicates) {
			szduplicates--;
			// Retrieve the size which is saved in the first slot
			// in the arrary of pointers to pamsyntoken.
			i = (uint)duplicates[szduplicates][-1];
			do {
				i--;
				mmfree(duplicates[szduplicates][i]);
			} while (i);
			
			mmfree(&duplicates[szduplicates][-1]);
		}
		
		mmfree(duplicates);
		
		// Set duplicates to NULL.
		duplicates = 0;
		// szduplicates has already been set to null in the loop above.
	}
	
	/* This function reset all the fields count, sel and
	the size of the string associated with the token
	when used for variadic '?'. */
	void init() {
		uint i = (uint)t.tokenstring[-1];
		
		do {
			i--;
			
			if (t.tokenstring[i]->repeat.seq) {
				
				t.tokenstring[i]->repeat.sel = 0;
				t.tokenstring[i]->repeat.count = t.tokenstring[i]->repeat.seq[0];
				
			} else if (t.tokenstring[i]->flags.isany) t.tokenstring[i]->str.sz = 0;
			
		} while (i);
	}
	
	// This function gather all the matches found and
	// pass it to the callback function.
	uint send() {
		
		u8* p = curpos;
		
		pamsyntoken* t = curtoken;
		
		// Keep the address of the match currently being worked out.
		pamsynfound* m = 0;
		
		// Keep the address of a match previously completed
		// and is used to set the field next of the match in m.
		pamsynfound* n = 0;
		
		/* This structure is used to gather
		the match result from a stringcompare block.
		A stringcompare block is the portion of
		the patern found between "%{" and "}". */
		typedef struct strcmpobj {
			// Id that was associated with
			// the stringcompare block.
			uint id;
			
			// Structure used to store the match
			// result from the stringcompare block.
			struct {
				u8* ptr; // Pointer to the string.
				uint sz; // Size of the string.
			} str;
			
			// The last element of the linked list
			// has this field null.
			struct strcmpobj* next;
			
		} strcmpobj;
		
		/* This variable point to a linked list
		of strcmpobj for which the field str
		contain the complete match result from
		the stringcompare block for which
		it was created. */
		strcmpobj* complete = 0;
		
		/* This variable point to a linked list
		of strcmpobj for which the field str
		contain an incomplete match result from
		the stringcompare block for which
		it was created. */
		strcmpobj* incomplete = 0;
		
		// I Create the memory session that will be used
		// to regain any memory allocated by this function.
		mmsession memsession1 = mmsessionnew();
		
		gathermatch:
		
		if (t->head) {
			
			if (t->head->strcmpid) {
				/* If I get here, I create a new object
				for the stringcompare block and add it
				to the linked list pointed by incomplete. */
				
				strcmpobj* obj = mmalloc(sizeof(strcmpobj));
				
				obj->id = t->head->strcmpid;
				
				obj->str.ptr = 0;
				obj->str.sz = 0;
				
				obj->next = incomplete;
				
				incomplete = obj;
			}
			
		} else if (t->strcmpid) {
			// When I get here, the following is always true:
			// t->strcmpid == incomplete->id;
			
			strcmpobj* obj = complete;
			
			while (obj) {
				
				if (obj->id == incomplete->id) {
					/* This function compare the strings in
					obj->str and incomplete->str and return 1
					if they are the same otherwise return 0. */
					uint cmpstr() {
						u8* s1ptr = obj->str.ptr;
						u8* s2ptr = incomplete->str.ptr;
						
						uint s1sz = obj->str.sz;
						uint s2sz = incomplete->str.sz;
						
						// The size of both strings has to be equal to each other
						// to be the same string even if the sizes are null.
						if (s1sz != s2sz) return 0;
						
						// If s1sz is null then s2sz is also null since
						// I tested above s1sz != s2sz; and I return 1.
						if (!s1sz) return 1;
						
						u8* i = s1ptr + s1sz;
						
						while (s1ptr < i) {
							if (*s1ptr != *s2ptr) return 0;
							
							s1ptr++; s2ptr++;
						}
						
						return 1;
					}
					
					if (cmpstr()) {
						/* If I get here a stringcompare block
						with the same id was already processed and
						the result of its match in obj->str is
						the same as the match result in incomplete->str
						for the stringcompare block just processed. */
						
						// I can free the strcmpobj pointed by incomplete.
						
						u8* str = incomplete->str.ptr;
						
						if (str) mmfree(str);
						
						obj = incomplete->next;
						
						mmfree(incomplete);
						
						incomplete = obj;
						
						goto keeptokenprocessing;
						
					} else {
						/* If I get here, a stringcompare block with
						the same id was already processed but the result
						of its match in obj->str is not the same as
						the match result in incomplete->str for
						the stringcompare block just processed. */
						
						/* Since I have a stringcompare block mismatch,
						I free any memory allocated within this function
						and I return 2 to continue looking for matches. */
						
						mmsessionfree(memsession1, MMIGNORESUBSESSIONS);
						
						return 2;
					}
				}
				
				obj = obj->next;
			}
			
			/* If I get here, a stringcompare block with
			the same id had not been previously processed.
			I move the strcmpobj pointed by incomplete
			to the linked list pointed by complete. */
			
			obj = incomplete->next;
			
			incomplete->next = complete;
			
			complete = incomplete;
			
			incomplete = obj;
			
		/* Here I check whether I have a nochar or
		a token with a non-null size.
		Note that, nochar tokens are the only tokens with
		a null string size that are allowed to be used
		to create a pamsynfound. A token such as the one
		representing the metachar '<' will never be used
		to create a pamsynfound. */
		} else if (t->flags.isnochar || t->str.sz) {
			// I only add to m a token which is not hidden.
			if (t->flags.ishidden) {
				// If the token is hidden I set m to null
				// in order to create a new pamsynfound.
				if (m) {
					/* I adjust the field nochar if I did not have characters
					gathered in the pamsynfound. So even if I had encountered
					a nochar, it will be cancelled because it was
					among non-hidden characters. */
					if (m->start <= m->end) m->nochar = 0;
					else {
						/* If I get here I have a nochar.
						If (m->start == start) then (m->end == start-1) which
						would be incorrect; so I must set m->end to m->start
						and set the field nochar to 1 instead. */
						if (m->start == start) {
							
							m->end = m->start;
							m->nochar = 1;
							
						} else {
							
							m->start = m->end;
							m->nochar = 2;
						}
					}
					
					n = m; // Save in n what was the last completed pamsynfound.
					m = 0;
				}
				
			} else {
				if (m) m->start -= t->str.sz;
				else {
					m = mmalloc(sizeof(pamsynfound));
					m->end = p - 1;
					m->start = p - t->str.sz;
					m->next = n;
					
					/* I get here only if I had a non-null string or nochar.
					I set the the field nochar of the pamsynfound to 3 and
					it will be adjusted later to the correct value. */
					m->nochar = 3;
				}
				
				if (incomplete && t->str.sz) {
					/* This function prepend the string which has
					the size t->str.sz and which is at the address
					(p - t->str.sz), to the string in incomplete->str. */
					void insertstr() {
						u8* s1ptr = incomplete->str.ptr;
						u8* s2ptr = p - t->str.sz;
						
						uint s1sz = incomplete->str.sz;
						uint s2sz = t->str.sz;
						
						s1ptr = mmrealloc(s1ptr, s1sz + s2sz);
						
						// I move to the right, the block on the right of
						// the position where I am trying to do insertion.
						bytcpy(s1ptr + s2sz, s1ptr, s1sz);
						
						// Do the insertion.
						bytcpy(s1ptr, s2ptr, s2sz);
						
						// Set the new string in incomplete->str.
						incomplete->str.ptr = s1ptr;
						incomplete->str.sz = s1sz + s2sz;
					}
					
					insertstr();
				}
			}
			
			p -= t->str.sz;
		}
		
		keeptokenprocessing:
		
		if (t->prev) {
			t = t->prev;
			goto gathermatch;
		}
		
		// If m is null, I set it to a previously created
		// pamsynfound that was successfully completed.
		if (!m) m = n;
		
		// I adjust the field nochar of the pamsynfound only
		// if there was any and if it was not set before.
		if (m && m->nochar == 3) {
			/* I adjust the field nochar if I did not have characters
			gathered in the pamsynfound. So even if I had encountered
			a nochar it will be cancelled because it was among
			non-hidden characters. */
			if (m->start <= m->end) m->nochar = 0;
			else {
				/* If I get here I have a nochar.
				If (m->start == start) then (m->end == start-1) which
				would be incorrect; So I must set m->end to m->start
				and set the field nochar to 1 instead. */
				if (m->start == start) {
					
					m->end = m->start;
					m->nochar = 1;
					
				} else {
					m->start = m->end;
					m->nochar = 2;
				}
			}
		}
		
		// When I get here, all tokens making up
		// the match have been processed.
		
		// I free all strcmpobj created in
		// the linked list pointed by complete.
		while (complete) {
			
			strcmpobj* obj = complete;
			
			complete = complete->next;
			
			u8* str = obj->str.ptr;
			
			if (str) mmfree(str);
			
			mmfree(obj);
		}
		
		// I free all strcmpobj created in
		// the linked list pointed by incomplete.
		while (incomplete) {
			
			strcmpobj* obj = incomplete;
			
			incomplete = incomplete->next;
			
			u8* str = obj->str.ptr;
			
			if (str) mmfree(str);
			
			mmfree(obj);
		}
		
		// I Create the memory session that will be used to preserve
		// any memory allocated within the callback function.
		mmsession memsession2 = mmsessionnew();
		
		/* When 0 is returned by the callback function,
		I should stop looking for a match.
		When 1 is returned by the callback function,
		I should continue matching from curpos starting over
		from the first token in the tokenstring; I also
		call it regular look up.
		When >1 is returned by the callback function, I should
		continue as if I didn't find a match and I should look
		for alternate tokens; I also call it slow look up. */
		uint i = callback(m);
		
		// I free any memory allocated within this function.
		mmsessionfree(memsession1, MMIGNORESUBSESSIONS);
		
		// I can now discard the memory session that was used to preserve
		// any memory allocated within the callback function.
		mmsessioncancel(memsession2, MMIGNORESUBSESSIONS);
		
		if (!i || i > 1) return i;
		
		/* If I get here i == 1 and I return 1 unless p != curpos
		otherwise I should do a slow lookup.
		I will enter an infinite loop if it is not taken in consideration,
		because curpos will not see its value changed due to
		the fact that the fields sz of the strings associated
		to tokens will remain null. */
		
		if (p == curpos) return 2;
		
		return 1;
	}
	
	// The argument size cannot be null.
	if (!size) return;
	
	// Initialize with the address of
	// the beginning of the memory area.
	startpos = (u8*)start;
	
	setcurpos:
	
	curpos = startpos;
	
	resetcurtoken:
	
	// I save in curtoken the address of
	// the first token of the tokenstring.
	curtoken = t.tokenstring[0];
	
	// I initialize all state variables within tokens
	// which are necessary for repeatitions.
	init();
	
	checkrepeat:
	
	// If the seq field is not null,
	// duplicate the set of tokens enclosed.
	if (curtoken->repeat.seq) {
		
		if (!dup()) {
			/* I jump to findprev. Note that the first token
			of a repeat set do not have alternate tokens
			attached to it. */
			goto findprev;
		}
	}
	
	/* Make sure that I will not create a page fault and
	compare the token against what is in memory.
	Notice that I used <= because if a match is at the end
	of the text region where I am looking for a match,
	adding curtoken->str.sz to curpos will obviously put curpos
	at start+size which is a byte after the last character of
	the region where I am looking for a match.
	If I do not use <= , I would not be able to use those matches
	which are the end of the region where I am looking for a match. */
	compare:
	if ((curpos+curtoken->str.sz <= start+size) && cmp()) {
		
		// Update curpos.
		curpos += curtoken->str.sz;
		
		// Set the field prev of the next token
		// so that I can go back up.
		if (curtoken->next) {
			
			curtoken->next->prev = curtoken;
			// Select the next token.
			curtoken = curtoken->next;
			
			goto checkrepeat;
		}
		
		// If I get here, then I found a match.
		// I call send() to process the match.
		uint i = send();
		
		/* if i == 0 it mean that I should stop searching for matches.
		if i == 1 then continue with regular lookup.
		if i > 1 then continue with slow lookup. */
		
		if (i < 2) {
			/* I free created duplicates if any, since
			I will not go back up; so they are
			not needed anymore. */
			if (duplicates) freealldup();
			
			if (!i) return;
			
			// If I get here then i == 1 .
			
			// If curpos was at the end of the text area
			// that I am processing, I should stop.
			if (curpos == start+size) return;
			
			startpos = curpos;
			
			goto resetcurtoken;
		}
		
		// If I get here then i > 1 .
		
		curpos -= curtoken->str.sz;
		
		goto findalt;
	}
	
	// Look for an alternative token, but first
	// look at all variadic '?' possibilities.
	findalt:
	
	if (curtoken->flags.isany) {
		
		if (curpos+curtoken->str.sz > start+size || curtoken->str.sz == -1) {
			// Set the field sz of the string back to zero
			// so that if I come back I can re-use it.
			curtoken->str.sz = 0;
			
			/* I jump to findprev, because I am sure that
			there is no alternative case, because for
			a metacharacter '*', a single token is created for it
			and there is no alternate cases attached to it. */
			goto findprev;
		}
		
		curtoken->str.sz++;
		
		goto compare;
	}
	
	if (curtoken->alt) {
		// Set the field prev of the alternate token
		// so that I can go back up.
		(*curtoken->alt).prev = curtoken->prev;
		
		// Select the alternate token.
		curtoken = curtoken->alt;
		
		goto checkrepeat;
	}
	
	findprev:
	// Check if I am about to leave a duplicate branch
	if (curtoken->head) {
		
		if (curtoken->head->holder != curtoken->holder) {
			/* If I get here then I have reached the first token
			of a duplucate. I free the duplicate. curtoken will
			be set appropriatly by freedup().
			freedup() is only called when curtoken point to
			the first token of a duplicate set. */
			freedup();
			
			goto checkrepeat;
		}
	}
	
	// Go back up using the field prev until
	// I find a non null field alt.
	if (curtoken->prev) {
		// It is important for this below
		// to come before updating curpos.
		curtoken = curtoken->prev;
		
		// Update curpos
		curpos -= curtoken->str.sz;
		
		goto findalt;
	}
	
	/* If I get here then I went all the way back
	to the first token of tokenstring. If after
	incrementing startpos, I am going beyond
	boundaries, I should stop. */
	startpos++;
	
	if (startpos == start+size) {
		/* When I get here, I should not have any created
		duplicates, but the array of duplicates pointed by
		the variable duplicates may still contain an allocated
		memory block because while going up the linked list of
		tokens through their field prev, freedup() is called
		to free a duplicate set and use mmrealloc() to resize
		the array of duplicates pointed by the variable duplicates.
		So even when szduplicates is null, the variable duplicates
		may still point to an allocated memory block because
		mmrealloc() used with a size of zero do not free
		the memory block but simply internally mark it as having
		a size of zero and return a pointer to it. mmfree() is
		the only function which can completely free
		an allocated or re-allocated block of memory. */
		if (duplicates) mmfree(duplicates);
		
		return;
	}
	
	goto setcurpos;
}
