
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


#ifndef LIBMACROS
#define LIBMACROS


// RoundDown to power of two.
#define ROUNDDOWNTOPOWEROFTWO(VALUE,POWEROFTWO) \
	(VALUE & (-POWEROFTWO))


// RoundUp to power of two.
#define ROUNDUPTOPOWEROFTWO(VALUE,POWEROFTWO) \
	(((VALUE-1) & (-POWEROFTWO)) + POWEROFTWO)


// Left bit-rotation.
#define BITROL(VALUE, COUNT) \
	(COUNT &= ((8*sizeof(typeof(VALUE)))-1), \
	((VALUE << COUNT) | (VALUE >> ((8*sizeof(typeof(VALUE)))-COUNT))))


// Right bit-rotation.
#define BITROR(VALUE, COUNT) \
	(COUNT &= ((8*sizeof(typeof(VALUE)))-1), \
	((VALUE >> COUNT) | (VALUE << ((8*sizeof(typeof(VALUE)))-COUNT))))


// Hash macro which return a digest
// from a single hashing interation.
// It is meant to be used iteratively
// whereby the seed is the previously
// returned digest.
#define HASH(data, seed) \
	(BITROR(seed, 1) + data)


// Add OBJECT to bottom of the circular
// linkedlist pointed by LINKEDLIST.
// FIELDPREV and FIELDNEXT are respectively
// the names of the fields prev and next
// of an object in the circular linkedlist.
// OBJECT and LINKEDLIST must be of pointer type.
// LINKEDLIST value get modified only if null.
#define LINKEDLISTCIRCULARADDTOBOTTOM(FIELDPREV, FIELDNEXT, OBJECT, LINKEDLIST) \
{ \
	/* These two lines are optimizations mainly \
	 * to prevent re-dereferencing everywhere in case \
	 * the arguments were dereference expressions. */ \
	typeof(OBJECT) savedobject = OBJECT; \
	typeof(OBJECT) savedlinkedlist = (typeof(OBJECT))LINKEDLIST; \
	\
	if (savedlinkedlist) { \
		\
		savedobject->FIELDNEXT = savedlinkedlist; \
		savedobject->FIELDPREV = savedlinkedlist->FIELDPREV; \
		savedlinkedlist->FIELDPREV->FIELDNEXT = savedobject; \
		savedlinkedlist->FIELDPREV = savedobject; \
		\
	} else { \
		\
		savedobject->FIELDPREV = savedobject; \
		savedobject->FIELDNEXT = savedobject; \
		LINKEDLIST = (typeof(LINKEDLIST))savedobject; \
	} \
}


// Add OBJECT to top of the circular
// linkedlist pointed by LINKEDLIST.
// The only difference is that
// the last expression was moved
// out of the second if() alternate.
// FIELDPREV and FIELDNEXT are respectively
// the names of the fields prev and next
// of an object in the circular linkedlist.
// OBJECT and LINKEDLIST must be of pointer type.
// LINKEDLIST value always get modified and set to OBJECT.
#define LINKEDLISTCIRCULARADDTOTOP(FIELDPREV, FIELDNEXT, OBJECT, LINKEDLIST) \
{ \
	/* These two lines are optimizations mainly \
	 * to prevent re-dereferencing everywhere in case \
	 * the arguments were dereference expressions. */ \
	typeof(OBJECT) savedobject = OBJECT; \
	typeof(OBJECT) savedlinkedlist = (typeof(OBJECT))LINKEDLIST; \
	\
	if (savedlinkedlist) { \
		\
		savedobject->FIELDNEXT = savedlinkedlist; \
		savedobject->FIELDPREV = savedlinkedlist->FIELDPREV; \
		savedlinkedlist->FIELDPREV->FIELDNEXT = savedobject; \
		savedlinkedlist->FIELDPREV = savedobject; \
		\
	} else { \
		\
		savedobject->FIELDPREV = savedobject; \
		savedobject->FIELDNEXT = savedobject; \
	} \
	\
	LINKEDLIST = (typeof(LINKEDLIST))savedobject; \
}


// Substitute OLDOBJECT with NEWOBJECT
// in its circular linkedlist
// FIELDPREV and FIELDNEXT are respectively
// the names of the fields prev and next
// of an object in the circular linkedlist.
// OLDOBJECT and NEWOBJECT must be of pointer type.
#define LINKEDLISTCIRCULARSUBSTITUTE(FIELDPREV, FIELDNEXT, OLDOBJECT, NEWOBJECT) \
{ \
	/* These two lines are optimizations mainly \
	 * to prevent re-dereferencing everywhere in case \
	 * the arguments were dereference expressions. */ \
	typeof(OLDOBJECT) savedoldobject = OLDOBJECT; \
	typeof(NEWOBJECT) savednewobject = NEWOBJECT; \
	\
	typeof(OLDOBJECT) savedoldobjectprev = savedoldobject->FIELDPREV; \
	typeof(OLDOBJECT) savedoldobjectnext = savedoldobject->FIELDNEXT; \
	\
	savednewobject->FIELDPREV = savedoldobjectprev; \
	savednewobject->FIELDNEXT = savedoldobjectnext; \
	savedoldobjectprev->FIELDNEXT = savednewobject; \
	savedoldobjectnext->FIELDPREV = savednewobject; \
}


// Remove OBJECT from the circular
// linkedlist pointed by LINKEDLIST.
// FIELDPREV and FIELDNEXT are respectively
// the names of the fields prev and next
// of an object in the circular linkedlist.
// If LINKEDLIST == OBJECT, and
// OBJECT is not the remaining
// object of the linkedlist,
// LINKEDLIST get set to OBJECT->FIELDNEXT .
// OBJECT and LINKEDLIST must be of pointer type.
// LINKEDLIST can be passed as null,
// if it is known that OBJECT is not
// the top of the circular linkedlist.
#define LINKEDLISTCIRCULARREMOVE(FIELDPREV, FIELDNEXT, OBJECT, LINKEDLIST) \
{ \
	/* These two lines are optimizations mainly \
	 * to prevent re-dereferencing everywhere in case \
	 * the arguments were dereference expressions. */ \
	typeof(OBJECT) savedobject = OBJECT; \
	typeof(OBJECT) savedlinkedlist = (typeof(OBJECT))LINKEDLIST; \
	\
	typeof(OBJECT) objectnext = savedobject->FIELDNEXT; \
	\
	if (objectnext != savedobject) { \
		\
		if (savedlinkedlist == savedobject) LINKEDLIST = objectnext; \
		\
		typeof(OBJECT) objectprev = savedobject->FIELDPREV; \
		\
		objectprev->FIELDNEXT = objectnext; \
		\
		objectnext->FIELDPREV = objectprev; \
		\
	} else LINKEDLIST = (typeof(OBJECT))0; \
}


#endif
