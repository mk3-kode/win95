/******************************************************************************
 SYS\TYPES.H - <sys/types.h> with the PlayStation's unsigned types.

 Reconstructed. The game (PSXCD.H) and OVERLAY.H include <sys/types.h> and
 use u_char, u_short and u_long, which the compiler's own sys/types.h does
 not have: the layer's include directory came first on the include path.
******************************************************************************/
#ifndef __EMU_SYS_TYPES_H
#define __EMU_SYS_TYPES_H

#ifdef __GNUC__
#include_next <sys/types.h>
#else
/* Visual C++ 2.0's sys/types.h (sys/stat.h needs all of it) */
#ifndef _INC_TYPES
#define _INC_TYPES
#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif
#ifndef _INO_T_DEFINED
typedef unsigned short _ino_t;
#if !__STDC__
typedef unsigned short ino_t;
#endif
#define _INO_T_DEFINED
#endif
#ifndef _DEV_T_DEFINED
typedef unsigned int _dev_t;
#if !__STDC__
typedef unsigned int dev_t;
#endif
#define _DEV_T_DEFINED
#endif
#ifndef _OFF_T_DEFINED
typedef long _off_t;
#if !__STDC__
typedef long off_t;
#endif
#define _OFF_T_DEFINED
#endif
#endif /* _INC_TYPES */
#endif

#ifndef _UCHAR_T
#define _UCHAR_T
typedef unsigned char	u_char;
#endif
#ifndef _USHORT_T
#define _USHORT_T
typedef unsigned short	u_short;
#endif
#ifndef _UINT_T
#define _UINT_T
typedef unsigned int	u_int;
#endif
#ifndef _ULONG_T
#define _ULONG_T
typedef unsigned long	u_long;
#endif

#endif /* __EMU_SYS_TYPES_H */
