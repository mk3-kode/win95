; FLICBRUN.ASM - FLIC BYTE_RUN chunk decoding for FLICFILE.C.
;
; Reconstructed from mk3w.exe (retail).

include win95.i

    BEGIN_CODE

	public _DecodeBrun
_DecodeBrun proc near
; u_char *DecodeBrun(FlicRaster *r, u_char *src, int x, int y, int w, int h)
; BYTE_RUN: positive count = run of one byte, negative = literal bytes

	push ebp
	mov ebp,esp
	push es
	push esi
	push edi
	push ebx
	push ecx
	push edx
	mov ebx,dword ptr 8[ebp]	; raster
	mov edx,dword ptr 12[ebx]	; bytes per row
	mov eax,dword ptr 20[ebp]	; y
	imul eax,edx
	add eax,dword ptr 16[ebp]	; x
	add eax,dword ptr 8[ebx]	; pixels
	mov ebx,eax
	add ebx,dword ptr 24[ebp]	; ebx = end of the row
	mov esi,dword ptr 12[ebp]
	xor ecx,ecx
	xor eax,eax
	cld
	sub ebx,edx
#line:
	add ebx,edx
	mov edi,ebx
	sub edi,dword ptr 24[ebp]
	inc esi						; packet count, unused
#packet:
	lodsb
	test al,al
	jns #run
	neg al
	mov cx,ax
	rep movsb
	cmp edi,ebx
	jb #packet
	dec dword ptr 28[ebp]
	jne #line
	jmp short #done
#run:
	mov cx,ax
	lodsb
	rep stosb
	cmp edi,ebx
	jb #packet
	dec dword ptr 28[ebp]
	jne #line
#done:
	mov eax,esi
	pop edx
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop es
	leave
	ret
_DecodeBrun endp


    END_CODE
    end
