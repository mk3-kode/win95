/******************************************************************************
 MEMFILL.C - word fill and long copy.

 Reconstructed from mk3w.exe (retail). In the original
 these leave eax as it was (assembly in a C function).
******************************************************************************/
#include "emulator.h"

/* at least one long is copied */
void CopyLongs(void *dst, void *src, int bytes)
{
#ifdef _MSC_VER
	__asm {
		push	eax
		push	ecx
		push	esi
		push	edi
		cld
		mov		edi, dst
		mov		esi, src
		mov		ecx, edi
		add		ecx, bytes
		mov		eax, [esi]
	copy:
		stosd
		add		esi, 4
		cmp		edi, ecx
		mov		eax, [esi]
		jb		copy
		pop		edi
		pop		esi
		pop		ecx
		pop		eax
	}
#else
	u_long *d = dst, *s = src;
	u_long *end = (u_long *)((u_char *)dst + bytes);

	do
		*d++ = *s++;
	while (d < end);
#endif
}

void FillWords(void *dst, u_short value, u_int bytes)
{
#ifdef _MSC_VER
	__asm {
		push	eax
		push	ecx
		push	edi
		cld
		mov		edi, dst
		mov		ecx, bytes
		shr		ecx, 1
		mov		ax, value
		repne stosw				/* sic: F2, works as rep */
		pop		edi
		pop		ecx
		pop		eax
	}
#else
	u_short *d = dst;

	for (bytes >>= 1; bytes != 0; bytes--)
		*d++ = value;
#endif
}
