.686
.MMX
.K3D
.XMM
.model flat, stdcall


_DATA SEGMENT PARA PUBLIC USE32 "Data"

; Muß DATA sein, wegen selbstmanipulierendem Code!

ALIGN 16


; YUV 16-235
; Cr=V, Cb=U
; Y = 0,257*R + 0,504*G + 0,098*B + 16
; V = 0,439*R - 0,368*G - 0,071*B + 128
; U =-0,148*R - 0,291*G + 0,439*B + 128
  _get_Y		DW   13, 64, 33,  0, 13, 64, 33,  0
  _get_Cr		DW   -9,-47, 56,  0, -9,-47, 56,  0
  _get_Cb       DW   56,-37,-19,  0, 56,-37,-19,  0
  _add_to_Y		DD   16, 16, 16, 16
  _add_to_CrCb	DD  128,128,128,128

; YUV 0-255
; Y = 0,299*R + 0,587*G + 0,114*B
; V = 0,500*R - 0,419*G - 0,0813*B + 128
; U =-0,169*R - 0,332*G + 0,500*B + 128
  _fr_get_Y		DW	 15, 74, 38,  0, 15, 74, 38,  0
  _fr_get_V     DW  -10,-53, 63,  0,-10,-54, 63,  0
  _fr_get_U     DW   63,-42,-21,  0, 63,-42,-21,  0
  _fr_add_to_Y		DD    0,  0,  0,  0
  _fr_add_to_CrCb	DD  128,128,128,128

  _table_dist   DD  _fr_get_Y - _get_Y

; YUY 16-235 -> RGB
; D:= Y - 16
; E= U - 128
; F:= V - 128
; B:= 1,164*D + 2,018*E
; G:= 1,164*D - 0,391*E - 0,813*F 
; R:= 1,164*D           - 1,596*F
;  _get_DEF		DW  -16,-128, -16,-128, -16,-128, -16,-128
  _get_DEF      DW  0,0,0,0,0,0,0,0
  _get_line1	DW  75,  25, 75,+102   ; +0.4*E,  1.6*F
  _get_line2	DW	75, 129, 75,  52   ;  2.0*E, +0.8*F
  _shufinp		DB 0D4h,0B1h,039h
  _pack_l2m     DW  0FFFFh,0,0FFFFh,0
  _final_sub	DW  17703, 10000,14246, 0
  _final_add    DW      0, 18761,    0, 0

; Masken für YUY2

ALIGN 16
  _mask_Y       DD  0FFh,0FFh,0FFh,0FFh

  _mask_YUY2_V  DW  000FFh,0,000FFh,0,0,0,0,0
  _mask_YUY2_U  DW  000FFh,0,000FFh,0,0,0,0,0

  RGBtoYUV422_Codes		DB  0,24,8,0, 8,16,0,0, 8,0,16,0

  RGBtoYCrCb			DD  0,0,0,0,0,offset RGBtoYCrCb_SSE2
  RGBtoYUV422			DD  0,0,0,0,0,offset RGBtoYUV422_SSE2

  CPU_plain				DD  0
  CPU_MMX				DD  1
  CPU_EnhMMX			DD  2
  CPU_3DNow				DD  3
  CPU_SSE				DD  4
  CPU_SSE2				DD  5

  current_CPU			DD  0

PUBLIC RGBtoYCrCb_SSE2
PUBLIC RGBtoYUV422_SSE2
PUBLIC YUV422toRGB_MMX

; dwFlags:
;   Bit  0: 0 = normal  1 = für Overlay
;   Bit 21: 00 = YUY2
;           01 = UYVY
;           10 = VYUY
;           11 = invalid

RGBtoYCrCb_SSE2 proc dwIn,dwOut,dwFlags,dwWidth,dwHeight,dwSPitch,dwDPitch
  pushad
  mov          esi, dwIn
  mov          edi, dwOut
  mov          eax, 0
  cmp          dwFlags, 1
  cmove        eax, _table_dist

  mov          ebx, dwHeight
@next_line:
  mov          ecx, dwWidth
  shr          ecx, 2
  push         esi
  push         edi

@next_pixels:
  pxor         xmm7, xmm7
  movdqa       xmm0, [esi]       ; Pixel:  RGB4 RGB3 RGB2 RGB1
  movdqa       xmm1, xmm0

  punpcklbw    xmm0, xmm7        ; Pixel:  RGB3 RGB1
  punpckhbw    xmm1, xmm7        ; Pixel:  RGB4 RGB2

  movdqa       xmm2, xmm0
  movdqa       xmm3, xmm1

  lea          edx, _get_Y
  movdqa       xmm6, [edx+eax]
  pmaddwd      xmm0, xmm6 
  pmaddwd      xmm1, xmm6
  pshufd       xmm4, xmm0, 0B1h
  pshufd       xmm5, xmm1, 0B1h
  paddd		   xmm0, xmm4
  paddd        xmm1, xmm5
  psrad        xmm0, 7
  psrad        xmm1, 7
  lea          edx,  _add_to_Y
  movdqa       xmm6, [edx+eax]

  packssdw     xmm0, xmm1
  paddd        xmm0, xmm6       ;    Y4    Y3    Y2    Y1
  pand         xmm0, _mask_Y


  movdqa       xmm7, xmm0

  lea          edx, _get_Cr
  movdqa       xmm6, [edx+eax]
  movdqa       xmm4, xmm2
  movdqa       xmm5, xmm3
  pmaddwd      xmm2, xmm6
  pmaddwd      xmm3, xmm6
  pshufd       xmm0, xmm2, 0B1h
  pshufd       xmm1, xmm3, 0B1h
  paddd        xmm0, xmm2
  paddd        xmm1, xmm3
  psrad        xmm0, 7
  psrad        xmm1, 7
  lea          edx,  _add_to_CrCb
  movdqa       xmm6, [edx+eax]
  packssdw     xmm0, xmm1
  paddd        xmm0, xmm6       ;    V4    V3    V2    V1
  pand         xmm0, _mask_Y
  pslld        xmm0, 8
  por          xmm7, xmm0

  lea          edx, _get_Cb
  movdqa       xmm6, [edx+eax]
  pmaddwd      xmm4, xmm6
  pmaddwd      xmm5, xmm6
  pshufd       xmm2, xmm4, 0B1h
  pshufd       xmm3, xmm5, 0B1h
  paddd        xmm4, xmm2
  paddd        xmm5, xmm3
  psrad        xmm4, 7
  psrad        xmm5, 7
  lea          edx,  _add_to_CrCb
  movdqa       xmm6, [edx+eax]
  packssdw     xmm4, xmm5
  paddd        xmm4, xmm6       ;    U4    U3    U2    U1
  pand         xmm4, _mask_Y
  pslld        xmm4, 16
  por          xmm7, xmm4

@store:
  movdqa       [edi], xmm7

  dec          ecx
  jnz          @next_pixels

  pop          edi
  pop          esi
  add          edi, dwDPitch
  add          esi, dwSPitch

  dec          ebx
  jnz          @next_line

  popad
  ret  

RGBtoYCrCb_SSE2 endp


RGBtoYUV422_SSE2 proc dwIn,dwOut,dwFlags,dwWidth,dwHeight,dwSPitch,dwDPitch
  pushad
  mov          eax, 0
  shr          dwFlags,1
  cmovc        eax, _table_dist
  mov          esi, dwFlags
  and          esi, 3
  lea          ebx, RGBtoYUV422_Codes

  mov          ecx, offset @shift_Y+4
  mov          dl, [ebx+4*esi]
  mov          [ecx], dl

  lea          ecx, @shift_V+4
  mov          dl, [ebx+4*esi+1]
  mov          [ecx], dl

  lea          ecx, @shift_U+4
  mov          dl, [ebx+4*esi+2]
  mov          [ecx], dl

  mov          esi, dwIn
  mov          edi, dwOut
  mov          ebx, dwHeight
@next_line:
  mov          ecx, dwWidth
  shr          ecx, 2
  push         esi
  push         edi

@next_pixels:
  pxor         xmm7, xmm7
  movdqa       xmm0, [esi]       ; Pixel:  RGB4 RGB3 RGB2 RGB1
  add          esi, 16
  movdqa       xmm1, xmm0

  punpcklbw    xmm0, xmm7        ; Pixel:  RGB2 RGB1
  punpckhbw    xmm1, xmm7        ; Pixel:  RGB4 RGB3

  movdqa       xmm2, xmm0
  movdqa       xmm3, xmm1

  lea          edx, _get_Y
  movdqa       xmm6, [edx+eax]
  pmaddwd      xmm0, xmm6 
  pmaddwd      xmm1, xmm6
  pshufd       xmm4, xmm0, 0B1h
  pshufd       xmm5, xmm1, 0B1h
  paddd		   xmm0, xmm4
  paddd        xmm1, xmm5
  psrad        xmm0, 7
  psrad        xmm1, 7
  lea          edx,  _add_to_Y
  movdqa       xmm6, [edx+eax]

  packssdw     xmm0, xmm1
  pand         xmm0, _mask_Y
  paddb        xmm0, xmm6       ;    Y4     Y3     Y2     Y1
  packssdw     xmm0, xmm7       ;
@shift_Y:
  pslld        xmm0, 0

  movdqa       xmm7, xmm0

  lea          edx, _get_Cr
  movdqa       xmm6, [edx+eax]
  movdqa       xmm4, xmm2
  movdqa       xmm5, xmm3
  pmaddwd      xmm2, xmm6
  pmaddwd      xmm3, xmm6
  pshufd       xmm0, xmm2, 0B1h
  pshufd       xmm1, xmm3, 0B1h
  paddd        xmm0, xmm2
  paddd        xmm1, xmm3
  psrad        xmm0, 7
  psrad        xmm1, 7
  lea          edx,  _add_to_CrCb
  movdqa       xmm6, [edx+eax]
  packssdw     xmm0, xmm1
  paddd        xmm0, xmm6       ;    V4    V3    V2    V1
  pand         xmm0, _mask_Y
  packssdw     xmm0, xmm0
  
  pshuflw      xmm1, xmm0, 0B1h ;    V3    V4    V1    V2
  pavgw        xmm1, xmm0
  pand         xmm1, _mask_YUY2_V
@shift_V:
  pslld        xmm1, 24
  por          xmm7, xmm1

  lea          edx, _get_Cb
  movdqa       xmm6, [edx+eax]
  pmaddwd      xmm4, xmm6
  pmaddwd      xmm5, xmm6
  pshufd       xmm2, xmm4, 0B1h
  pshufd       xmm3, xmm5, 0B1h
  paddd        xmm4, xmm2
  paddd        xmm5, xmm3
  psrad        xmm4, 7
  psrad        xmm5, 7
  lea          edx,  _add_to_CrCb
  movdqa       xmm6, [edx+eax]
  packssdw     xmm4, xmm5
  pand         xmm4, _mask_Y
  
  paddd        xmm4, xmm6       ;    U4    U3    U2    U1
  packssdw     xmm4, xmm5
  pshuflw      xmm5, xmm4, 0B1h ;    U3    U4    U1    U2
  pavgw        xmm5, xmm4
  pand         xmm5, _mask_YUY2_U
@shift_U:
  pslld        xmm5, 8
  por          xmm7, xmm5

@store:
  movlps       [edi], xmm7

  dec          ecx
  jnz          @next_pixels

  pop          edi
  pop          esi
  add          edi, dwDPitch
  add          esi, dwSPitch

  dec          ebx
  jnz          @next_line

  popad
  ret  
RGBtoYUV422_SSE2 endp




YUV422toRGB_MMX proc dwIn,dwOut,dwFlags,dwWidth,dwHeight,dwSPitch,dwDPitch
  pushad
  mov          esi, dwIn
  mov          edi, dwOut
  mov          ebx, dwHeight

@next_line:
  mov          ecx, dwWidth
  shr          ecx, 1
  push         esi
  push         edi
@next_pixels:
  
  pxor         mm7, mm7
  movd         mm0, [esi]
  add          esi, 4
@shift:
  ; noch nichts


  punpcklbw    mm0, mm7
  lea          edx, _get_DEF
  movq         mm1, [edx]
  paddw        mm0, mm1  
  movq         mm1, mm0

  lea          edx, _get_line1
  lea          eax, _get_line2
  movq         mm5, [edx]
  movq         mm6, [eax]

  movq         mm2, mm0
  movq         mm3, mm1
  pmullw       mm0, mm5
  pmullw       mm1, mm6
  
  pmulhw       mm2, mm5
  pmulhw       mm3, mm6

  movq         mm4, mm0
  movq         mm7, mm1
  punpcklwd    mm0, mm2
  punpcklwd    mm1, mm3
  punpckhwd    mm4, mm2
  punpckhwd    mm7, mm3
  packssdw     mm0, mm4
  packssdw     mm1, mm7

  movq         mm2, mm0
  psrlq        mm2, 32

  psrlq        mm1, 16
  lea          edx, _pack_l2m
  pand         mm1, [edx]
  movq         mm3, mm1
  psrlq        mm3, 16
  por          mm1, mm3

; MM0:  0/0  0/1
; MM1:  1/1  1/3
; MM2:  0/2  0/3
; MM3:  0/0  0/1
  movq         mm3, mm0
  pxor         mm7, mm7
  psubsw       mm7, mm1
  paddsw       mm1, mm0
  psrlq        mm1, 16
  movq         mm6, mm2
  psubsw       mm0, mm7
  psubsw       mm6, mm7  

; mm0, mm6: B1, B2
; mm1: GT
  movq         mm4, mm2
  psrlq        mm2, 16

; mm2: Add_R
; mm3, mm4: 0/0  0/2
  movq         mm5, mm3
  movq         mm7, mm4

  psubsw      mm3, mm1  ; G1
  psubsw      mm4, mm1  ; G2
  paddsw      mm5, mm2  ; R1
  paddsw      mm7, mm2  ; R2
  
  punpcklwd    mm0, mm3
  punpcklwd    mm6, mm4
  punpckldq    mm0, mm5
  punpckldq    mm6, mm7
  lea          edx, _final_add
  paddw        mm0, [edx]
  paddw        mm6, [edx]
  lea          edx, _final_sub
  psubusw      mm0, [edx]
  psubusw      mm6, [edx]

  psrlw        mm0, 6
  psrlw        mm6, 6

  packuswb     mm0, mm0
  movd         edx, mm0
  and          edx, 0FFFFFFh
  mov          [edi], edx
  packuswb     mm6, mm6
  movd         edx, mm6
  and          edx, 0FFFFFFh
  mov          [edi+4], edx
  add          edi, 8

  dec          ecx
  jnz          @next_pixels

  pop          edi
  pop          esi
  add          edi, dwDPitch
  add          esi, dwSPitch

  dec          ebx
  jnz          @next_line
  popad
  emms
  ret
YUV422toRGB_MMX endp

_DATA ends

END