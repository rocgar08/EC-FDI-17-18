/*-------------------------------------------------------------------
**
**  Fichero:
**    main.c  11/2/2016
**
**  Notas de diseño:
**    - El buffer de memoria del LCD esta en 0x0c200000 (es programable)
**      y es de tamaño 0x9600B (320x240px y con 2px/B)
**    - Las 36 imagenes a visualizar consecutivamente en el LCD están
**      ubicadas a partir de la dirección 0x0c400000 cada 0x10000B,
**      cada una ocupa 0x9600B y se indican en el fichero load_img.text
**
**-----------------------------------------------------------------*/


#include "44b.h"
#include "44blib.h"
#include <stdio.h>
#include "option.h"
#include "dma.h"
#include "uart.h"
#include "utils.h"
#include "intcontroller.h"
#include "timer.h"
#include "gpio.h"
#include "button.h"

/*
** El buffer de memoria del LCD esta en 0x0c200000 y es de tamaño 0x9600B
** (El LCD tiene 320x240px y en memoria cada byte contiene 2px)
**
*/

void timer0_isr( void ) __attribute__ ((interrupt ("IRQ")));



void sys_init( void )
{
	rWTCON  = 0;		// deshabilita el watchdog
	rINTMSK = ~0;		// enmascara todas las interrupciones

	// Configuracion del gestor de reloj
	rLOCKTIME = 0xfff;	// estabilizacion del PLL = 512 us
	rPLLCON   = 0x38021;	// MCLK = 64MHz
	rCLKSLOW  = 0x8;		// MCLK_SLOW = 500 KHz
	rCLKCON   = 0x7ff8;	// modo NORMAL y reloj distribuido a todos los controladores

	// Configuracion del arbitro de bus
	// prioridad fija por defecto LCD > ZDMA > BDMA > IRQ
     rSBUSCON = 0x8000001b;	

	// Configuracion de cache
	rSYSCFG = 0;				// deshabilitada

	// Configuracion del controlador de interrupciones
	rI_PMST = 0x1f1b;	// prioridades fijas por defecto
	rI_PSLV = 0x1b1b1b1b;
	
	rEXTINTPND = ~0;			// borra las interrupciones externas pendientes por linea EINT7..4
    rI_ISPC = ~0;			// borra todas las interrupciones pendientes
}	

void lcd_init( void )
{      
	rDITHMODE = 0x12210;
	rDP1_2    = 0xa5a5;
	rDP4_7    = 0xba5da65;
	rDP3_5    = 0xa5a5f;
	rDP2_3    = 0xd6b;
	rDP5_7    = 0xeb7b5ed;
	rDP3_4    = 0x7dbe;
	rDP4_5    = 0x7ebdf;
	rDP6_7    = 0x7fdfbfe;
	
	rREDLUT   = 0;
	rGREENLUT = 0;
	rBLUELUT  = 0;

	// indica al controlador de LCD que el buffer LCD esta en la dirección 0x0c200000 y es de tamaño 0x9600B;
	rLCDSADDR1 = 0x16100000;
	rLCDSADDR2 = 0x20104B00;
	rLCDSADDR3 = 0x50;

	rLCDCON2  = 0x13cef;
	rLCDCON3  = 0;
	rLCDCON1   = 0xc021; // refresco a 120 Hz
}


void putImageNoDMA( unsigned int imgDir )
{
	unsigned int i;
	unsigned char *src, *dst;

	src = (unsigned char *) imgDir;
	dst = (unsigned char *) 0x0c200000;

	for( i=0; i<0x9600; i++ )
		dst[i] = src[i];
}
unsigned int video = 0;
void timer0_isr(void){
	/* rPDATG registro de datos que permite escribir o leer del puerto.*/
	rPDATG =(0x1<<7);
	if((rPDATG &(0x1<<7)) == 0)
		putImageNoDMA(0x0C400000+ video*0x10000);
	//video*0x10000
	//video*0x0c200000
	else putImageDMA(0x0C400000+video*0x10000);//creo que esto se va a salir del "array"
 /**
  * las 36 imágenes y con el script que permite cargarlas en memoria del ARM
  *  a partir de la dirección 0xC400000.
  **/
	video = (video +1)%36;
	ic_cleanflag(INT_TIMER0);
}

int setup(void)
{
    sys_init();
    uart0_init();
    lcd_init();
    zdma0_init();

/* Configurar los puertos, el controlador de interrupciones y el timer0 para que genere interrupciones vectorizadas por la línea IRQ cada 0,1segundos. */
    portG_conf(7,INPUT);
    portG_eint_trig(7,FALLING);
    portG_conf_pup(7,1);

    rPCOND = 0xAAAA;
    rPUPD = 0x0;

    //Igual que en la practica 4
    ic_conf_irq(1, VEC);
	ic_conf_fiq(0);
	ic_conf_line(INT_TIMER0, IRQ);
	ic_enable(INT_TIMER0);

	tmr_set_prescaler(0,255);
	tmr_set_divider(0, D1_4);
	tmr_set_count(TIMER0, 6250, 255);

	tmr_update(TIMER0);
	tmr_set_mode(0, RELOAD);
	tmr_start(TIMER0);

	pISR_TIMER0 = (unsigned) timer0_isr;
    return 0;
}


void main( void )
{
	unsigned int num = 0;

	setup();

	while( 1 )
		{
		uart0_putint( num++ );
		uart0_putchar( '\n' );
		}

}


