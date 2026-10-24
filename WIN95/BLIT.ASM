; BLIT.ASM - span copies for the PSX emulation (PSXEMU.C, POLYSCAL.C).
;
; Reconstructed from mk3w.exe (retail).
; Texels are 8-bit CLUT indices, colour 0 is transparent; pixels are 16-bit.

include win95.i

EXTERN _Emu

    BEGIN_CODE


	public _Blit8
_Blit8 proc near
; void Blit8(int h, int w, u_short *clut, u_char *src, u_char *dst, int dstskip, int srcskip)

	push ebp
	mov ebp,esp
	push esi
	push edi
	push ebx
	push ecx
	mov esi,dword ptr 20[ebp]	; src
	mov edi,dword ptr 24[ebp]	; dst
	mov ebx,dword ptr 16[ebp]	; clut
	xor eax,eax
#row:
	mov ecx,dword ptr 12[ebp]	; ecx = end of the source row
	add ecx,esi
#texel:
	lodsb
	test eax,eax
	je short #clear
	mov dx,word ptr [ebx+eax*2]
	mov word ptr [edi],dx
#clear:
	add edi,2
	cmp esi,ecx
	jb #texel
	add esi,dword ptr 32[ebp]	; srcskip
	add edi,dword ptr 28[ebp]	; dstskip
	dec dword ptr 8[ebp]
	jg #row
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
_Blit8 endp


	public _Blit8Wide
_Blit8Wide proc near
; every texel written to two pixels

	push ebp
	mov ebp,esp
	push esi
	push edi
	push ebx
	push ecx
	mov esi,dword ptr 20[ebp]
	mov edi,dword ptr 24[ebp]
	mov ebx,dword ptr 16[ebp]
	xor eax,eax
#row:
	mov ecx,dword ptr 12[ebp]
	add ecx,esi
#texel:
	lodsb
	test eax,eax
	je short #clear
	mov dx,word ptr [ebx+eax*2]
	mov word ptr [edi],dx
	mov word ptr 2[edi],dx
#clear:
	add edi,4
	cmp esi,ecx
	jb #texel
	add esi,dword ptr 32[ebp]
	add edi,dword ptr 28[ebp]
	dec dword ptr 8[ebp]
	jg #row
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
_Blit8Wide endp


	public _Blit8Mirror
_Blit8Mirror proc near
; dst is the rightmost pixel; the row is written right to left

	push ebp
	mov ebp,esp
	push esi
	push edi
	push ebx
	push ecx
	mov esi,dword ptr 20[ebp]
	mov edi,dword ptr 24[ebp]
	mov ebx,dword ptr 16[ebp]
	xor eax,eax
#row:
	mov ecx,dword ptr 12[ebp]
	add ecx,esi
#texel:
	lodsb
	test eax,eax
	je short #clear
	mov dx,word ptr [ebx+eax*2]
	mov word ptr [edi],dx
#clear:
	sub edi,2
	cmp esi,ecx
	jb #texel
	add esi,dword ptr 32[ebp]
	add edi,dword ptr 28[ebp]
	dec dword ptr 8[ebp]
	jg #row
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
_Blit8Mirror endp


	public _Blit8WideMirror
_Blit8WideMirror proc near

	push ebp
	mov ebp,esp
	push esi
	push edi
	push ebx
	push ecx
	mov esi,dword ptr 20[ebp]
	mov edi,dword ptr 24[ebp]
	mov ebx,dword ptr 16[ebp]
	xor eax,eax
#row:
	mov ecx,dword ptr 12[ebp]
	add ecx,esi
#texel:
	lodsb
	test eax,eax
	je short #clear
	mov dx,word ptr [ebx+eax*2]
	mov word ptr [edi],dx
	mov word ptr 2[edi],dx
#clear:
	sub edi,4
	cmp esi,ecx
	jb #texel
	add esi,dword ptr 32[ebp]
	add edi,dword ptr 28[ebp]
	dec dword ptr 8[ebp]
	jg #row
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
_Blit8WideMirror endp


	public _VramLoad16
_VramLoad16 proc near
; void VramLoad16(u_char *src, int x, int y, int wbytes, int h)
; copy into the 1024x512 16-bit VRAM at Emu.vram16

	push ebp
	mov ebp,esp
	push esi
	push edi
	push ebx
	push ecx
	mov esi,dword ptr 8[ebp]
	test esi,esi
	je #done
	mov edi,dword ptr 16[ebp]	; y
	shl edi,10
	add edi,dword ptr 12[ebp]	; x
	shl edi,1
	add edi,dword ptr _Emu
	mov ebx,dword ptr 24[ebp]	; h
	shl ebx,11
	add ebx,edi					; ebx = end
	mov edx,dword ptr 20[ebp]	; wbytes
	mov eax,800h
	sub eax,edx					; eax = to the next row
#row:
	mov ecx,edx
	shr ecx,2
	repne movsd
	mov cl,dl
	and cl,3
	repne movsb
	add edi,eax
	cmp edi,ebx
	jb #row
#done:
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
_VramLoad16 endp


	public _VramFill16
_VramFill16 proc near
; void VramFill16(u_short color, int x, int y, int w, int h)

	push ebp
	mov ebp,esp
	push esi
	push edi
	push ebx
	push ecx
	mov edi,dword ptr 16[ebp]
	shl edi,10
	add edi,dword ptr 12[ebp]
	shl edi,1
	add edi,dword ptr _Emu
	mov esi,dword ptr 24[ebp]
	shl esi,11
	add esi,edi					; esi = end
	mov edx,dword ptr 20[ebp]	; w
	mov ecx,edx
	shl ecx,1
	mov ebx,800h
	sub ebx,ecx
	mov eax,dword ptr 8[ebp]
	shl eax,16
	mov ax,word ptr 8[ebp]
#row:
	mov ecx,edx
	shr ecx,1
	repne stosd
	mov cl,dl
	and cl,1
	repne stosw
	add edi,ebx
	cmp edi,esi
	jb #row
	pop ecx
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
_VramFill16 endp


	public _ScaleSpan
_ScaleSpan proc near
; void ScaleSpan(int row, SCALESPAN *ss)	called by ScaleRows (POLYSCAL.C)
; one screen row of a scaled quad: texel row 'row' of ss->src, columns from
; the ss->ucols table

	push ebp
	push ecx
	push esi
	push edi
	push ebx
	mov ebp,dword ptr 28[esp]	; ss
	mov esi,dword ptr 24[esp]	; row
	shl esi,8
	add esi,dword ptr 0[ebp]	; ss->src
	mov ecx,dword ptr 12[ebp]	; ss->clut
	mov edi,dword ptr 4[ebp]	; ss->dst
	mov ebx,dword ptr 8[ebp]	; ss->ucols
	mov eax,dword ptr 20[ebp]	; ss->wbytes
	mov ebp,edi
	add ebp,eax					; ebp = end of the row
#pixel:
	movzx eax,word ptr [ebx]
	add ebx,2
	movzx eax,byte ptr [esi+eax]
	test eax,eax
	je short #clear
	mov ax,word ptr [ecx+eax*2]
	stosw
	cmp edi,ebp
	jb #pixel
	mov ebp,dword ptr 28[esp]
	add edi,dword ptr 28[ebp]	; ss->dstskip
	mov dword ptr 4[ebp],edi
	pop ebx
	pop edi
	pop esi
	pop ecx
	pop ebp
	ret
#clear:
	add edi,2
	cmp edi,ebp
	jb #pixel
	mov ebp,dword ptr 28[esp]
	add edi,dword ptr 28[ebp]
	mov dword ptr 4[ebp],edi
	pop ebx
	pop edi
	pop esi
	pop ecx
	pop ebp
	ret
_ScaleSpan endp


	public _DoubleScanlines
_DoubleScanlines proc near
; void DoubleScanlines(u_char *frame)	640x480: copy each even row to the
; odd row below it

	push esi
	push ecx
	push edi
	push ebx
	mov esi,dword ptr 20[esp]
	mov edi,esi
	mov ebx,edi
	add ebx,4afffh
#row:
	add edi,500h
	mov ecx,140h
	repne movsd
	add esi,500h
	cmp edi,ebx
	jb #row
	pop ebx
	pop edi
	pop ecx
	pop esi
	ret
_DoubleScanlines endp


    END_CODE
    end
