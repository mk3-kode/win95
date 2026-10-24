; FLICCOL.ASM - FLIC COLOR_256 and COLOR_64 chunks decoding for FLICFILE.C.
;
; Reconstructed from mk3w.exe (retail).

include win95.i

    BEGIN_CODE

	public _DecodeColor256
_DecodeColor256 proc near
; void DecodeColor256(void *chunk, FlicRGB *palette)	COLOR_256: 8-bit RGB

	push ebp
	mov ebp,esp
	push esi
	push edi
	push ecx
	push ebx
	xor eax,eax
	mov esi,dword ptr 8[ebp]
	mov edi,dword ptr 12[ebp]
	lodsw						; packet count
	mov ebx,eax
	test ebx,ebx
	jmp #next
#packet:
	lodsb						; colours to skip
	add edi,eax
	add edi,eax
	add edi,eax
	lodsb						; colours to copy, 0 = 256
	or al,al
	jne #count
	mov eax,100h
#count:
	mov ecx,eax
	add ecx,eax
	add ecx,eax
	rep movsb
	dec ebx
#next:
	jne #packet
	pop ebx
	pop ecx
	pop edi
	pop esi
	pop ebp
	ret
_DecodeColor256 endp


	public _DecodeColor64
_DecodeColor64 proc near
; void DecodeColor64(void *chunk, FlicRGB *palette)	COLOR_64: 6-bit RGB

	push ebp
	mov ebp,esp
	push esi
	push edi
	push ecx
	push ebx
	xor eax,eax
	mov esi,dword ptr 8[ebp]
	mov edi,dword ptr 12[ebp]
	lodsw
	mov ebx,eax
	test ebx,ebx
	jmp #next
#packet:
	lodsb
	add edi,eax
	add edi,eax
	add edi,eax
	lodsb
	or al,al
	jne #count
	mov eax,100h
#count:
	mov ecx,eax
	add ecx,eax
	add ecx,eax
#byte:
	lodsb
	shl al,2
	stosb
	loop #byte
	dec ebx
#next:
	jne #packet
	pop ebx
	pop ecx
	pop edi
	pop esi
	pop ebp
	ret
_DecodeColor64 endp


    END_CODE
    end
