// Autor: Ignacio Piñeyro
// Fecha:

#include "board.h"						//Incluir siempre
#include "arm_math.h"					//Incluir si se trabaja con DSP de CMSIS
#include "FreeRTOS.h"					//Incluir siempre
#include "task.h"						//Incluir siempre
#include "queue.h"						//Incluir si se trabaja con colas
#include "semphr.h"						//Incluir si se trabaja con semáforos

#define PORT(n)		((uint8_t) n)
#define PIN(n)		((uint8_t) n)
#define GRUPO(n)	((uint8_t) n)

#define    PORT_TEC1    ((uint8_t) 0)
#define    PIN_TEC1     ((uint8_t) 4)
#define    PORT_TEC2    ((uint8_t) 0)
#define    PIN_TEC2     ((uint8_t) 8)
#define    PORT_TEC3    ((uint8_t) 0)
#define    PIN_TEC3     ((uint8_t) 9)
#define    PORT_TEC4    ((uint8_t) 1)
#define    PIN_TEC4     ((uint8_t) 9)

#define    PORT_LED1    ((uint8_t) 0)
#define    PIN_LED1     ((uint8_t) 14)
#define    PORT_LED2    ((uint8_t) 1)
#define    PIN_LED2     ((uint8_t) 11)
#define    PORT_LED3    ((uint8_t) 1)
#define    PIN_LED3     ((uint8_t) 12)
#define    PORT_LED0_R  ((uint8_t) 5)
#define    PIN_LED0_R   ((uint8_t) 0)
#define    PORT_LED0_G  ((uint8_t) 5)
#define    PIN_LED0_G   ((uint8_t) 1)
#define    PORT_LED0_B  ((uint8_t) 5)
#define    PIN_LED0_B   ((uint8_t) 2)

#define ADC_0		0
#define CHANNEL_0	0

#define ORDEN_FILTRO	8

arm_fir_instance_f32 Filtro_FIR;			//Se define Filtro_FIR como un FIR de punto flotante dentro de math.h

QueueHandle_t Dato_Salida;					//Se define el tipo de dato Queue para Dato_Salida de Filtro manual
QueueHandle_t Dato_Salida_DSP;				//Se define el tipo de dato Queue para Dato_Salida_DSP de Filtro DSP

SemaphoreHandle_t Semphr_ADC;				//Se define el tipo de dato Semaphore para Semphr_ADC para el ADC

QueueHandle_t Contador_UP;					//Se define el tipo de dato Queue para Contador_UP de Pulsador_UP
QueueHandle_t Contador_DOWN;				//Se define el tipo de dato Queue para Contador_DOWN de Pulsador_DOWN

SemaphoreHandle_t  Semaforo_T3T1;			//Se define el tipo de dato Semaphore para Semaforo_T3T1 para Pulsador_3
SemaphoreHandle_t  Semaforo_T3T2;			//Se define el tipo de dato Semaphore para Semaforo_T3T2 para Pulsador_3
SemaphoreHandle_t  Semaforo_T3Muestra;		//Se define el tipo de dato Semaphore para Semaforo_T3Muestra para Pulsador_3



//Handler del ADC0
void ADC0_IRQHandler (void){												//La interrupción siempre retorna void y tiene argumento void

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( Semphr_ADC, &xHigherPriorityTaskWoken );			//En cada entrada de dato al ADC, la interrupción libera el semáforo para la tarea Procesa_Datos
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

//Configuración de Hardware
static void prvSetupHardware(void){

	SystemCoreClockUpdate();

	Chip_GPIO_Init(LPC_GPIO_PORT);

    Chip_SCU_PinMux( GRUPO(1) , PIN(0) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_PULLUP , SCU_MODE_FUNC0 );		//CFG TEC_1
    Chip_SCU_PinMux( GRUPO(1) , PIN(1) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_PULLUP , SCU_MODE_FUNC0 );		//CFG TEC_2
    Chip_SCU_PinMux( GRUPO(1) , PIN(2) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_PULLUP , SCU_MODE_FUNC0 );		//CFG TEC_3
    Chip_SCU_PinMux( GRUPO(1) , PIN(6) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_PULLUP , SCU_MODE_FUNC0 );		//CFG TEC_4
    Chip_SCU_PinMux( GRUPO(2) , PIN(10) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS , SCU_MODE_FUNC0 );						//CFG LED1
    Chip_SCU_PinMux( GRUPO(2) , PIN(11) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS , SCU_MODE_FUNC0 );						//CFG LED2
    Chip_SCU_PinMux( GRUPO(2) , PIN(12) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS , SCU_MODE_FUNC0 );						//CFG LED3
    Chip_SCU_PinMux( GRUPO(2) , PIN(0) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS , SCU_MODE_FUNC4 );							//CFG LED0_R
    Chip_SCU_PinMux( GRUPO(2) , PIN(1) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS , SCU_MODE_FUNC4 );							//CFG LED0_G
    Chip_SCU_PinMux( GRUPO(2) , PIN(2) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS , SCU_MODE_FUNC4 );							//CFG LED0_B

	Chip_SCU_PinMux( GRUPO(4) , PIN(3) , SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS , SCU_MODE_FUNC0 );							//CFG ADC0_0
	Chip_SCU_ADC_Channel_Config (ADC_0 , CHANNEL_0);																						//Como el ADC está en FUNC0 hay que aclarar que funciona el ADC

    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, PORT(PORT_TEC1), PIN(PIN_TEC1));																//INPUT TEC_1
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, PORT(PORT_TEC2), PIN(PIN_TEC2));																//INPUT TEC_2
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, PORT(PORT_TEC3), PIN(PIN_TEC3));																//INPUT TEC_3
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, PORT(PORT_TEC4), PIN(PIN_TEC4));																//INPUT TEC_4

    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PORT(PORT_LED1), PIN(PIN_LED1));																//OUTPUT LED1
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PORT(PORT_LED2), PIN(PIN_LED2));																//OUTPUT LED2
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PORT(PORT_LED3), PIN(PIN_LED3));																//OUTPUT LED3
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PORT(PORT_LED0_R), PIN(PIN_LED0_R));															//OUTPUT LED0_R
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PORT(PORT_LED0_G), PIN(PIN_LED0_G));															//OUTPUT LED0_G
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PORT(PORT_LED0_B), PIN(PIN_LED0_B));															//OUTPUT LED0_B

    Chip_GPIO_SetPinState(LPC_GPIO_PORT, PORT(PORT_LED1), PIN(PIN_LED1), (bool) false);														//CLR LED1
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, PORT(PORT_LED2), PIN(PIN_LED2), (bool) false);														//CLR LED2
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, PORT(PORT_LED3), PIN(PIN_LED3), (bool) false);														//CLR LED3
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, PORT(PORT_LED0_R), PIN(PIN_LED0_R), (bool) false);													//CLR LED0_R
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, PORT(PORT_LED0_G), PIN(PIN_LED0_G), (bool) false);													//CLR LED0_G
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, PORT(PORT_LED0_B), PIN(PIN_LED0_B), (bool) false);													//CLR LED0_B
}

//Función de fantasía para simular lectura de ADC
unsigned int Leer_ADC (void){

	static unsigned int i = 0;
	unsigned int Valores_Cicliclos [8] = {5,6,4,5,6,4,7,3};

	return (Valores_Cicliclos [(i++) % ORDEN_FILTRO]);				//Retorna los valores de Valores_Ciclicos cambiando de posición en cada ejecución y sin pasarse del último gracias al módulo %
}

//Función de fantasía para simular salida por el DAC
void DAC_Output (unsigned int b){

	b ++;
}


//Procesamiento de datos
static void Procesa_Datos (void *pvParameters){

	static float Datos_ADC [ORDEN_FILTRO];
	static float b [ORDEN_FILTRO] = {0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125};		//Coeficientes del filtro FIR. 1/ORDEN_FILTRO para hacer una ponderación nivelada
	//static float a[ORDEN_FILTRO] = {1, 2, 3, 4, 5, 6, 7};											//Coeficientes de Feedback para filtro IIR
	static float Salida;
	//static float FIR[ORDEN_FILTRO], Feedback[(ORDEN_FILTRO-1)];									//Terminos para filtro IIR
	static float Salida_DSP;

	static unsigned int Indice = 0;
	static unsigned int i = 0;

	arm_fir_init_f32 (&Filtro_FIR, ORDEN_FILTRO, b, Datos_ADC, 1);				//Inicializa el FIR DSP

	while (1){																	//Todas las tareas de FREERTOS van en un loop (excepto que se la borre antes de salir)
		//xSemaphoreTake (Semphr_ADC , portMAX_DELAY);							//COMENTAR PARA DEBUGEAR. Como no hay ADC, el Handler nunca va a dar el semáforo que se toma acá.

		if (Indice < ORDEN_FILTRO){												//Comienza la lectura del ADC y la carga de esos datos en el vector Datos_ADC
			Datos_ADC [Indice] = (float) Leer_ADC ();							//Leer_ADC es una función de fantasía (previamente definida)
			Indice ++;
		}

		if (Indice == ORDEN_FILTRO)	{											//Si ya está el vector de muestras completo, aplica el filtro a las muestras
			for (i = 0; i < (ORDEN_FILTRO - 1); i++)							//Realiza un corrimiento en el vector de datos para leer el siguiente valor que entre al ADC
				Datos_ADC [i] = Datos_ADC [i + 1];

			Datos_ADC [ORDEN_FILTRO - 1] = (float) Leer_ADC ();
			Salida = 0;															//Setea la salida en 0

			for (i = 0; i < ORDEN_FILTRO; i++)									//Para las 8 posiciones del vector de datos:
				Salida += Datos_ADC [i] * b[i];									//Aplica el filtro FIR manual

			//for (i = 0; i < ORDEN_FILTRO; i++){								//Aplica el filtro IIR manual
			//	FIR[i] = Datos_ADC [i] * b[i];
			//	Feedback[i] = Datos_ADC [i+1] * a[i];
			//  Salida += FIR[i] + Feedback[i] ;
			//}

			xQueueSend(Dato_Salida, &Salida, portMAX_DELAY);					//Toma el dato de Salida con FIR manual aplicado y lo escribe en la cola Dato_Salida

			arm_fir_f32 (&Filtro_FIR, Datos_ADC, &Salida_DSP, 1);				//Aplica el filtro FIR DSP
			xQueueSend(Dato_Salida_DSP, &Salida_DSP, portMAX_DELAY);			//Toma el dato de Salida con FIR DSP aplicado y lo escribe en la cola Dato_Salida
		}

	}
}


//Salida de datos
static void Salida_Datos (void *pvParameters){

	static float Salida_Datos, Salida_Datos_DSP;								//Genera las variables para las salida de dataos del filtro manual y DSP

	while (1){																	//Todas las tareas de FREERTOS van en un loop (excepto que se la borre antes de salir)
		xQueueReceive (Dato_Salida, &Salida_Datos, 0);							//Recibe el dato saliente del procesamiento FIR manual (FIFO) y lo guarda en Salida_Datos
		xQueueReceive (Dato_Salida, &Salida_Datos_DSP, 0);						//Recibe el dato saliente del procesamiento FIR DSP (FIFO) y lo guarda en Salida_Datos
		DAC_Output ( (unsigned int) Salida_Datos);								//Castea los datos de salida del FIR manual para usarlos de argumento en la funcion de fantasía que saca valores por el DAC
		DAC_Output ( (unsigned int) Salida_Datos_DSP);						    //Castea los datos de salida del FIR DSP para usarlos de argumento en la funcion de fantasía que saca valores por el DAC
	}
}

//Pulsador UP
static void Pulsador_UP(void *pvParameters) {

	static unsigned int Cuenta = 0;
	static unsigned int Buffer;
	static unsigned int Estado_Anterior;

	while (1){

		xSemaphoreTake (Semaforo_T3T1, portMAX_DELAY); 													//Toma el Semaforo_T3T1

		if(Chip_GPIO_GetPinState (LPC_GPIO_PORT, PORT_TEC1, PIN_TEC1) == 0 && Estado_Anterior !=0){		//Si TEC1 está pulsada
			Estado_Anterior = 0;
			Cuenta ++;																					//Se incrementa el número de veces a blinkear LED
		}

		else{
			if(Chip_GPIO_GetPinState (LPC_GPIO_PORT, PORT_TEC1, PIN_TEC1)==1)							//Si TEC1 no está pulsada
			Estado_Anterior = 1;
		}


		vTaskDelay(20/portTICK_RATE_MS);																//Delay de 20ms
		if (uxQueueMessagesWaiting (Contador_UP) > 0)													//Chequea si hay valores para leer en la cola Contador_UP
			xQueueReceive (Contador_UP,&Buffer,0);														//Lee el valor de cuenta en la cola Contador_UP
		xQueueSend(Contador_UP, &Cuenta, portTICK_RATE_MS);												//Escribe el valor de Cuenta en la cola Contador_UP
	}
}

//Pulsador DOWN
static void Pulsador_DOWN(void *pvParameters){

	static unsigned int Cuenta = 0;
	static unsigned int Buffer;
	static unsigned int Estado_Anterior;

	while (1){

		xSemaphoreTake (Semaforo_T3T2, portMAX_DELAY);													//Toma el Semaforo_T3T2

		if(Chip_GPIO_GetPinState  (LPC_GPIO_PORT, PORT_TEC2, PIN_TEC2) == 0 && Estado_Anterior !=0){ 	//Si TEC2 está pulsada
			Estado_Anterior = 0;
			if (Cuenta > 0)																				//Se asegura que el numero de veces a blinkear el LED sea positivo
				Cuenta --;																				//Se decrementa el número de veces a blinkear LED
		}

		else{
			if(Chip_GPIO_GetPinState (LPC_GPIO_PORT, PORT_TEC2, PIN_TEC2)==1)							//Si TEC2 no está pulsada
				Estado_Anterior = 1;
		}
		vTaskDelay(20/portTICK_RATE_MS);																//Delay de 20ms
		if (uxQueueMessagesWaiting (Contador_DOWN) > 0)													//Chequea si hay valores para leer en la cola Contador_DOWN
			xQueueReceive (Contador_DOWN, &Buffer, 0);													//Lee el valor de Buffer en la cola Contador_DOWN
		xQueueSend(Contador_DOWN, &Cuenta, portMAX_DELAY); 												//Escribe el valor de Cuenta en la cola Contador_DOWN

	}
}

//Pulsador para detener la cuenta de veces a blinkear
static void Pulsador_3(void *pvParameters){

	while (1){
		if(Chip_GPIO_GetPinState (LPC_GPIO_PORT, PORT_TEC3, PIN_TEC3) == 1){							//Si TEC3 no está pulsada
			xSemaphoreGive(Semaforo_T3T1);																//Habilita el Semaforo_T3T1 para Pulsador_UP
			xSemaphoreGive(Semaforo_T3T2);																//Habilita el Semaforo_T3T2 para Pulsador_DOWN
		}

		else{																							//Si TEC3 está pulsada
			xSemaphoreGive(Semaforo_T3Muestra);															//Habilita el Semaforo_T3Muestra para vTask_4 (Blinkear LED)
			vTaskDelay(2000/portTICK_RATE_MS); 															//Delay de 2000ms
		}
	}
}

//Blinkeo de LED3 segun Cuenta
static void vTask_4(void *pvParameters) {

	static unsigned int Parpadeos;
	static unsigned int i;

	while (1){
		xSemaphoreTake (Semaforo_T3Muestra, portMAX_DELAY); 											//Toma Semaforo_T3Muestra
		xQueueReceive (Contador_UP, &Parpadeos, portMAX_DELAY);											//Lee el valor Parpadeos de la cola Contador_UP

		for (i=0; i<(2*Parpadeos); i++){																//Tooglea LED3 la cantidad de veces que diga Parpadeos
			Chip_GPIO_SetPinToggle (LPC_GPIO_PORT, PORT(PORT_LED3),PIN(PIN_LED3));
			vTaskDelay(300/portTICK_RATE_MS);															//Delay 300ms
		}
	}
}

//Blinkeo de LED1 automático
static void vLEDTask1(void *pvParameters){

	while (1){
		Chip_GPIO_SetPinToggle (LPC_GPIO_PORT, PORT(PORT_LED1) , PIN(PIN_LED1) );
		vTaskDelay(500 / portTICK_RATE_MS);
	}
}

//Main
int main(void)
{
	prvSetupHardware();

	vSemaphoreCreateBinary(Semphr_ADC);								//Crea el Semphr_ADC

	Dato_Salida = xQueueCreate(100, sizeof (float));				//Crea una cola de 100 items máximo de tamaño float cada uno para la salida del filtro FIR manual
	Dato_Salida_DSP = xQueueCreate(1, sizeof (float));				//Crea una cola de 100 items máximo de tamaño float cada uno para la salida del filtro FIR DSP

	xSemaphoreTake(Semphr_ADC, portMAX_DELAY);						//Inicializa Semphr_ADC como tomado

	xTaskCreate(Procesa_Datos, "Procesamiento", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);	//Crea tarea de procesamiento de datos con parámetro NULL
	xTaskCreate(Salida_Datos, "Output", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);			//Crea tarea de salida de datos con parámetro NULL

	vSemaphoreCreateBinary(Semaforo_T3T1);							//Crea el Semaforo_T3T1
	vSemaphoreCreateBinary(Semaforo_T3T2);							//Crea el Semaforo_T3T2
	vSemaphoreCreateBinary(Semaforo_T3Muestra);						//Crea el Semaforo_T3Muestra

	Contador_UP = xQueueCreate(1, sizeof(unsigned int)); 			//Crea una cola de 1 item máximo de tamaño uint para el Contador_UP
	Contador_DOWN = xQueueCreate(1, sizeof(unsigned int));			//Crea una cola de 1 item máximo de tamaño uint para el Contador_DOWN

	xSemaphoreTake(Semaforo_T3T1, portMAX_DELAY);					//Inicializa Semaforo_T3T1 como tomado
	xSemaphoreTake(Semaforo_T3T2, portMAX_DELAY);					//Inicializa Semaforo_T3T2 como tomado
	xSemaphoreTake(Semaforo_T3Muestra, portMAX_DELAY);				//Inicializa Semaforo_T3Muestra como tomado

	xTaskCreate(Pulsador_UP, "Tarea_UP", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);			//Crea tarea para Pulsador_UP con parámetro NULL
	xTaskCreate(Pulsador_DOWN, "Tarea_DOWN", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);		//Crea tarea para Pulsador_DOWN con parámetro NULL
	xTaskCreate(Pulsador_3, "Tarea_T3", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);			//Crea tarea para Pulsador_3 con parámetro NULL
	xTaskCreate(vTask_4, "Tarea_Muestra", configMINIMAL_STACK_SIZE,	NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);			//Crea tarea para vTask4 con parámetro NULL

	xTaskCreate(vLEDTask1, "vTaskLed1", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);			// Crea tarea para vTaskLed1 con parámetro NULL

	vTaskStartScheduler();			 								//Inicializa el Scheduler y el SO toma posesión del microcontrolado

	return 1;														//No debería llegar nunca acá
}
