;	Name 1: David Chun
;	Name 2: Chioma Okorie
;	UT EID 1: dc37875
;	UT EID 2: coo279

	.ORIG x3000
		LEA R0, COUNT	;counter
		LDW R0, R0, #0
		LEA R2, MLOC	;mask for switching
		LDW R2, R2, #0
		LDW R2, R2, #0	;get actual mask
		LEA R3, MASK 	;isolate bits mask
		LDB R3, R3, #0
		LEA R1, OFFSET	;store to data offset
		LDW R1, R1, #0
		LEA R5, DATA 	;load from data address
		LDW R5, R5, #0	;load location
LOOP	ADD R0, R0, #-1	;loop 4 times
		BRz DONE
		AND R4, R2, R3	;isolate bits [1:0]
		ADD R4, R4, R5	;new offset to load from
		LDB R4, R4, #0	;load bytes to swap
		STB R4, R1, #0	;store into next spot
		RSHFL R2, R2, #2	;left shift by 2 to handle next
		ADD R1, R1, #1	;new data location to store to
		BR LOOP 		;branch unconditional to loop
DONE	TRAP x25		;done
MASK	.FILL #3
COUNT 	.FILL #5
DATA	.FILL x4000
MLOC	.FILL x4004
OFFSET	.FILL x4005
	.END