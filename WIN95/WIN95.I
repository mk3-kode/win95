; WIN95.I - assembler setup for the Windows 95 build.
;
; Reconstructed. The routines that use it end up in mk3w.exe's "code" section.

	.386p
	.model flat

BEGIN_CODE	macro
code	segment dword public use32 'CODE'
	assume cs:code, ds:FLAT, es:FLAT, ss:FLAT
	endm

END_CODE	macro
code	ends
	endm
