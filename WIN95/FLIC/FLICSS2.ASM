; FLICSS2.ASM - FLIC DELTA_FLC (SS2) chunk decoding for FLICFILE.C.
;
; Reconstructed from mk3w.exe (retail).

include win95.i

    BEGIN_CODE

	public _DecodeDeltaFLC
_DecodeDeltaFLC proc near
; u_char *DecodeDeltaFLC(u_char *src, u_char *pixels, int bpr, int width)
; DELTA_FLC: word-oriented line packets; an opcode word with the top bits
; 11 skips lines, 10 sets the last byte of the line

	push ebp
	mov ebp,esp
	push es
	push esi
	push edi
	push ebx
	push ecx
	push edx
	xor eax,eax
	mov esi,dword ptr 32[esp]
	mov edi,dword ptr 36[esp]
	mov ebp,dword ptr 40[esp]	; bpr
	lodsw						; lines
	mov edx,eax
	xor ecx,ecx
	jmp #opcode
#skiplines:
	push edx
	neg ax
	mul ebp
	pop edx
	add edi,eax
	xor eax,eax
#opcode:
	lodsw
	test ax,ax
	jns #packets
	cmp ah,0c0h
	jae #skiplines
	mov ebx,dword ptr 44[esp]	; width
	mov byte ptr es:[edi+ebx-1],al
	lodsw
	test ax,ax
	je #endline
#packets:
	mov bx,ax
	push edi
#packet:
	lodsw
	mov cl,al					; bytes to skip
	add edi,ecx
	test ah,ah
	js #run
	mov cl,ah					; words to copy
	rep movsw
	dec bx
	jne #packet
	pop edi
#endline:
	add edi,ebp
	dec edx
	jne #opcode
	jmp #done
#run:
	neg ah
	mov cl,ah
	lodsw
	rep stosw
	dec bx
	jne #packet
	pop edi
	add edi,ebp
	dec edx
	jne #opcode
#done:
	mov eax,esi
	pop edx
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop es
	pop ebp
	ret
_DecodeDeltaFLC endp


    END_CODE
    end
