/* STDSTUFF.H - the layer's copy (\mk3p\emulator\inc\stdstuff.h, which
   SOUND\BUILDSND.C includes as ../emulator/inc/stdstuff.h). Reconstructed. LONG
   is unsigned, as the retail game code has it on the PlayStation (MKSONY.H):
   randu() in MKUTIL.C divides with divl. */
#ifndef __STDSTUFF_H
#define __STDSTUFF_H

// where I put my standard library types and basic macros

#ifdef __cplusplus
	extern "C" {
#endif // __cplusplus

#include <stddef.h> // system standard definitions

#ifndef INT_MAX
	#include <limits.h> // compiler dependent data type sizes
#endif

#ifdef __cplusplus
	}
#endif // __cplusplus

// defines type of int

#if (LONG_MAX == INT_MAX && ULONG_MAX == UINT_MAX)
	#define INT_IS_LONG 1
#else
	#define INT_IS_LONG 0
#endif

#if (SHRT_MAX == INT_MAX && USHRT_MAX == UINT_MAX)
	#define INT_IS_SHORT 1
#else
	#define INT_IS_SHORT 0
#endif

#ifndef CDECL
	#define CDECL cdecl
#endif

// null pointers usually defined in stddef.h

#ifndef NULL
	#define NULL ((void *)0)
#endif

#define NODATA  NULL

#ifndef NOFUNC
	#define NOFUNC NULL 
#endif


typedef int Errcode;	/* >= 0 if ok, < 0 otherwise see errcodes.h */
#define Success ((int)0)
#define Failure -1

typedef int Boolean;
#define TRUE 1
#define FALSE 0

/* BYTE, UBYTE, etc is to keep code portable between different compilers
   where especially 'int' can mean different things. */

typedef int    			INT;
typedef unsigned int    UINT;
typedef signed char     SBYTE;
typedef unsigned char   UBYTE;
typedef signed char     CHAR;
typedef unsigned char   UCHAR;
typedef signed short    SHORT;
typedef unsigned short  USHORT;
typedef unsigned long	LONG;
typedef unsigned long   ULONG;
typedef float   		FLOAT;
typedef double			DOUBLE;

/* these are sometimes handy, put your own here if you like */

#define Min(a,b) ((a)<(b)?(a):(b))
#define Max(a,b) ((a)>(b)?(a):(b))
#define Absval(x) ((x)<0?(-(x)):(x))
#define ArrayCount(a) (sizeof(a)/sizeof(a[0]))

// offset a pointer byte address wise 
#define OPTR(base,offset) ((void*)((UBYTE*)((void *)base)+(offset))) 

// gives size of memory given pointer to start and one byte beyond end 
#define SIZE(beg,end) \
	(unsigned)((UBYTE*)((void *)(end))-(UBYTE*)((void *)(beg)))

// gives inclusive size between two fields in a structure
#define INCLUSIVE_SIZE(f0,f1) SIZE(&(f0),&(&(f1))[1])

// offset generating macros.  Note: these will not work with classes
// that have any non public members or and base classes

// returns syntax reference to a field within a given struct name;
// useful for sizeof(MEMBER(s,f)) or sizeof(*MEMBER(s,f))

#define MEMBER(Class,Field) \
	((Class*)NULL)->Field

// returns offset of field within a given struct name,
// and field name ie: OFFSET(struct sname,fieldname)

#define OFFSET(Class,Field) \
	(USHORT)((ULONG)((UBYTE*)&MEMBER(Class,Field)-(UBYTE*)NULL))

// return pointer to class given pointer to field in class
#define TOCLASS(Class,Field,Fieldptr) \
	 ((Class *)OPTR(Fieldptr,-OFFSET(Class,Field))) 

#ifdef __cplusplus

	#define deletez(x) { delete x; x = NULL; }

	#include <iostream.h>
	#define BOX  cout << dec << __FILE__ << " " << __LINE__ << ' '
	#define DBOX  cout << dec << __FILE__ << " " << __LINE__ << endl;
#else
	#include <stdio.h>
	#define DBOX  printf("%s %d\n", __FILE__, __LINE__ );
#endif


#endif // __STDSTUFF_H -- leave at end of file

