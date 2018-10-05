#include <stdio.h>
#include "44b.h"
#include "leds.h"
#include "utils.h"
#include "D8Led.h"
#include "intcontroller.h"
#include "timer.h"
#include "gpio.h"
#include "keyboard.h"

#define BUT1 0x1
#define BUT2 0x2

struct RLstat {
	int moving;
	int speed;
	int direction;
	int position;
};

static struct RLstat RL = {
	.moving = 0,
	.speed = 5,
	.direction = 0,
	.position = 0,
};
/**
 * Registrar  las  ISRs,  es  decir,
 *  añadir  la  dirección  de  la  ISRs  a  la  tabla  de
 * direcciones  de  las  subrutinas  de  tratamiento  de  interrupción
 * */
void timer_ISR (void) __attribute__ ((interrupt ("IRQ")));
void button_ISR (void) __attribute__ ((interrupt ("IRQ")));
void keyboard_ISR (void) __attribute__ ((interrupt ("IRQ")));

void timer_ISR(void)
{
	//COMPLETAR: cada vez que el TIMER0 interrumpe el led rotante se mueve
	// si el giro es horario position se incrementa si su valor es <5 si no escribe un 0
	// si el giro es antihorario position se decrementa si su valor es >0 si no se escribe un 5


	if(RL.direction == 1){
		if(RL.position < 5)
			RL.position++;
		else
			RL.position = 0;
	}
	else if(RL.direction == 0){
		if(RL.position > 0)
			RL.position--;
		else
			RL.position = 5;
	}

	D8Led_segment(RL.position);
	ic_cleanflag(INT_TIMER0);
}

void button_ISR(void)
{
	unsigned int whichint = rEXTINTPND;
	unsigned int buttons = (whichint >> 2) & 0x3;

	//COMPLETAR: BUT1 cambia el estado de LED1 y cambia dirección de
	//movimiento del led rotante
	//Hay que añadir lo otro

	if (buttons == BUT1) {
		led1_switch();
		if(RL.direction==0)
			RL.direction = 1;
		else
			RL.direction = 0;
	}

	//COMPLETAR: BUT2 cambia el estado de LED2
	//activa movimiento de led rotante si parado
	//o lo para si está en movimiento (actuar sobre rutinas del TIMER)

	if (buttons == BUT2) {
		led2_switch();
		if(RL.moving == 0){
			RL.moving = 1;
			tmr_update(TIMER0);
			tmr_start(TIMER0);

		}
		else{
			RL.moving = 0;
			tmr_stop(TIMER0);
		}
	}



	// eliminamos rebotes
	Delay(2000);

	//COMPLETAR: debemos borrar las peticiones de interrupción correspondientes a los pulsadores en rEXTINTPND
	if (rEXTINTPND & 0x4){
		rEXTINTPND |= (0x1 << 2);
	}
	else if(rEXTINTPND & 0x8){
		rEXTINTPND |= (0x1 << 3);
	}
	ic_cleanflag(INT_EINT4567);
}
void keyboard_ISR(void){
	int sc = kb_scan();
	if(sc!=-1){
		D8Led_digit(sc);
		switch(sc){
		case 0:
			tmr_set_divider(TIMER0, D1_8);
			tmr_set_count(TIMER0, 62500, 62495);
			tmr_update(TIMER0);
			break;
		case 1:
			tmr_set_divider(TIMER0, D1_8);
			tmr_set_count(TIMER0, 31250, 31195);
			tmr_update(TIMER0);
			break;
		case 2:
			tmr_set_divider(TIMER0, D1_8);
			tmr_set_count(TIMER0, 15625, 15600);
			tmr_update(TIMER0);
			break;
		case 3:
			tmr_set_divider(TIMER0, D1_4);
			tmr_set_count(TIMER0, 15625, 15600);
			tmr_update(TIMER0);
			break;
		}
		//esperar a que la tecla se suelte
		if((rPDATG & 0x2) == 0){}
	}
	Delay(2000);
	ic_cleanflag(INT_EINT1);
}
int setup(void)
{
	leds_init();
	D8Led_init();
	D8Led_segment(RL.position);

	/* Port G:*/
	//COMPLETAR: utilizando el interfaz para el puerto G definido en gpio.h
	//		configurar los pines 6 y 7 del puerto G para poder generar interrupciones
	//		configurar que las interrupciones generadas por los pines 6 y 7 sean por flanco de bajada
	//		activar las correspondientes resistencias de pull-up.

	portG_conf(6, EINT);
	portG_conf(7, EINT);
	portG_conf(1, EINT);//Interrupción del teclado

	portG_eint_trig(6, FALLING);
	portG_eint_trig(7, FALLING);
	portG_eint_trig(1, FALLING);

	portG_conf_pup(1, ENABLE);
	portG_conf_pup(6, ENABLE);
	portG_conf_pup(7, ENABLE);



	/********************************************************************/

	/* Configuracion del timer */

	//COMPLETAR: utilizando el interfaz para los timers definido en timer.h
	//		configurar el timer 0:
	//      valor de prescalado a 255
	//      valor del divisor 1/8
	//      valor de cuenta 62500 y cualquier valor de comparacion entre 1 y 62499
	//      actualizar el contador con estos valores (update)
	//      poner el contador en modo RELOAD
	//      dejar el contador parado

	tmr_set_prescaler(0,255);
	tmr_set_divider(0, D1_8);
	tmr_set_count(TIMER0, 62500, 62495);
	tmr_update(TIMER0);
	tmr_set_mode(0, RELOAD);
	tmr_stop(TIMER0);

	if (RL.moving)
		tmr_start(TIMER0);
	/***************************/
	/**
	 *para registrarla como rutina de tratamiento de interrupción
	 **/
	pISR_TIMER0 = (unsigned) timer_ISR;
	pISR_EINT4567 = (unsigned) button_ISR;
	pISR_EINT1 = (unsigned) keyboard_ISR;
	/* Configuración del controlador de interrupciones */

	ic_init();
	//COMPLETAR: utilizando el interfaz definido en intcontroller.h
	//		habilitar la línea IRQ en modo no vectorizado
	//		deshabilitar la línea FIQ
	//		configurar la línea INT_TIMER0 en modo IRQ
	//		configurar la línea INT_EINT4567 en modo IRQ
	//		habilitar la línea INT_TIMER0
	//		habilitar la línea INT_EINT4567
	ic_conf_irq(ENABLE, VEC);
	ic_conf_fiq(DISABLE);
	ic_conf_line(INT_TIMER0, IRQ);
	ic_conf_line(INT_EINT4567, IRQ);
	ic_enable(INT_TIMER0);
	ic_enable(INT_EINT4567);
	ic_enable(INT_EINT1);



	/***************************************************/

	Delay(0);
	return 0;

}

int main(void)
{
	setup();

	while (1) {
		//return 0;
	}
	return 0;
}
