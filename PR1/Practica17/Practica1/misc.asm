
.equ COEF1, 3483
.equ COEF2, 11718
.equ COEF3, 1183


.text
.global div16384
.global rgb2gray

rgb2gray:

	//Prologo
	push {r4-r6,lr}

	//CUERPO*
	mov r3,#1 //AUX1
	mov r4,#1 //AUX2

	ldr r5, = COEF1
	ldrb r6,[r0]//pixel R
	mul r3,r5,r6

	ldr r5, = COEF2
	ldrb r6,[r0,#1]//pixel G
	mul r4,r5,r6
	add r3,r3,r4

	ldr r5, = COEF3
	ldrb r6,[r0,#2] //pixel B
	mul r4,r5,r6
	add r3,r4,r3

	mov r0,r3

	bl div16384
	pop {r4-r6,lr}
	mov pc,lr
div16384:
	lsr r0,r0,#14
	mov pc,lr
