    INCLUDE "hardware/custom.i"
    INCLUDE "hardware/cia.i"
	INCLUDE "devices/input.i"
	INCLUDE "devices/inputevent.i"
	INCLUDE "resources/potgo.i"
	INCLUDE "lvo/exec_lib.i"
	INCLUDE "lvo/potgo_lib.i"


* Entered with:       A0 == scratch (execpt for highest pri vertb server)
*  D0 == scratch      A1 == is_Data
*  D1 == scratch      A5 == vector to interrupt code (scratch)
*                     A6 == scratch
*
    SECTION CODE

	; Pin 5 (mouse button 3) is connected to an interrupt enabled pin on the Arduino
	; Toggling this generates an interrupt and the Arduino writes the scroll wheel 
	; values to right/middle mopuse buttons
	XDEF    _VertBServer
_VertBServer:

	; If the right mouse button being held down do nothing!
	LEA		_custom,A0
	MOVE.W	potinp(A0), D0
	AND.W	#$400, D0
	BEQ		exit

	; If the left mouse button being held down do nothing!
	BTST	#6, $BFE001
	BEQ		exit

	MOVEM.L A2, -(SP)

	; Alternatively could we ignore it?
	; BTST	#6, $BFE001
	; BNE		bla
	; moveq #1,d5
	; MOVE.B $BFE001, 4(A1)
; bla:
	

	MOVE.L	A1, A2					 ; is_Data

	; Output enable right & middle mouse.  Write 1 to right and 0 to middle
	MOVE.L	14(A2),A6
	MOVE.L	#$00000E00,D0
	MOVE.L	#$00000F00,D1
	JSR		_LVOWritePotgo(A6)		; WritePotgo(word,mask)(d0/d1)

	; Wait a bit.
	; The Arduino needs to catch the interrupt and reply on the right/middle mouse buttons
	LEA		_custom,A0
Delay:
	MOVEQ	#11,D1					; Needs testing on a slower Amiga!
.wait1	
	MOVE.B	vhposr+1(A0),D0			; Bits 7-0     H8-H1 (horizontal position)
.wait2	
	CMP.B	vhposr+1(A0),D0
	BEQ.B	.wait2
	DBF	D1,	.wait1
	
	; Save regs for C code
	MOVE.W	potinp(A0),(A2)			; Middle/Right Mouse
	MOVE.W	joy0dat(A0),2(A2)		; Mouse Counters (not used yet)
	
	; cmp.b #0, D5
	; BNE skiplmouse
	MOVE.B 	$BFE001,4(A2)			; Left Mouse 
; skiplmouse:

	; Output enable right & middle mouse.  Write 1 to right and 1 to middle
	MOVE.L	#$00000F00,D0
	MOVE.L	#$00000F00,D1
	JSR		_LVOWritePotgo(A6)		; WritePotgo(word,mask)(d0/d1)
	
	;
	; Signal the main task
	;
	MOVE.L 4.W,A6
	MOVE.L 6(A1),D0					; Signals
	MOVE.L 10(A1),A1				; Task
	
	; FIXME: If values are zero, no need to signal
	JSR _LVOSignal(A6)
	MOVEM.L (SP)+,A2

exit:
	MOVE.L	$DFF000, A0				; if you install a vertical blank server at priority 10 or greater, you must place custom ($DFF000) in A0 before exiting
	MOVEQ.L #0,D0					; set Z flag to continue to process other vb-servers
	RTS

	
skip:
	MOVE.W	#$0f00, $dff180
	JMP		exit
