; FLICLC.ASM - FLIC DELTA_FLI (LC) chunk decoding for FLICFILE.C.
;
; Reconstructed from mk3w.exe (retail).

include win95.i

    BEGIN_CODE

	public _DecodeLC
_DecodeLC proc near
; u_char *DecodeLC(u_char *src, u_char *pixels, int bpr)
; DELTA_FLI: line skip, line count, then per line packets of
; (skip, count): positive count = literal bytes, negative = run of one byte

	push ebp
	mov ebp,esp
	sub esp,4
	push es
	push esi
	push edi
	push ebx
	push ecx
	push edx
	cld
	xor eax,eax
	mov esi,dword ptr 8[ebp]
	mov edi,dword ptr 12[ebp]
	lodsw						; lines to skip
	mul dword ptr 16[ebp]
	add edi,eax
	lodsw						; lines to decode
	mov dword ptr [ebp-4],eax
	mov edx,edi
	xor ah,ah
#line:
	mov edi,edx
	lodsb						; packets in this line
	mov bl,al
	test bl,bl
	jmp #next
#packet:
	lodsb
	add edi,eax
	lodsb
	test al,al
	js #run
	mov ecx,eax
	rep movsb
	dec bl
	jne #packet
	jmp #endline
#run:
	neg al
	mov ecx,eax
	lodsb
	rep stosb
	dec bl
#next:
	jne #packet
#endline:
	add edx,dword ptr 16[ebp]
	dec dword ptr [ebp-4]
	jne #line
	mov eax,esi
	pop edx
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop es
	mov esp,ebp
	pop ebp
	ret
_DecodeLC endp


    END_CODE
    end
