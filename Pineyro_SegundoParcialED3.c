// Autor: Ignacio Piñeyro
// Fecha: 08 Julio 2020

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

#define ORDEN_FILTRO	4

arm_fir_instance_f32 Filtro_FIR;				//Se define Filtro_FIR como un FIR de punto flotante dentro de math.h

QueueHandle_t Dato_Salida_FIR;					//Se define el tipo de dato Queue para Dato_Salida de Filtro FIR DSP
QueueHandle_t Dato_Salida_IIR;					//Se define el tipo de dato Queue para Dato_Salida_DSP de Filtro IIR

SemaphoreHandle_t Semphr_ADC_FIR;				//Se define el tipo de dato Semaphore para Semphr_ADC para el FIR DSP
SemaphoreHandle_t Semphr_ADC_IIR;				//Se define el tipo de dato Semaphore para Semphr_ADC para el IIR

static int i=0;

//Handler del ADC0
void ADC0_IRQHandler (void){

	if (i % 2 == 0){																//Para muestras pares, se habilita el semáforo para la tarea que utiliza el FIR DSP
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(Semphr_ADC_FIR, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken );
	}

	else{																			//Para muestras impares, se habilita el semáforo para la tarea que utiliza el IIR
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(Semphr_ADC_IIR, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken );
	}

	i++;
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
	return i;
}

//Función de fantasía para simular salida por el DAC del filtro FIR
void DAC_FIR_Output (float a){
	//return a;
}

//Función de fantasía para simular salida por el DAC del filtro IIR
void DAC_IIR_Output (float b){
	//return b;
}

//Procesamiento de datos pares con FIR DSP
static void Filtrado_FIR (void *pvParameters){

	static float Datos_ADC [ORDEN_FILTRO];
	static float b [ORDEN_FILTRO] = {0.25, 0.5, 1, 2};							//Coeficientes b del filtro FIR
	static float Salida_FIR;
	static unsigned int Indice = 0;
	static unsigned int i = 0;

	arm_fir_init_f32 (&Filtro_FIR, ORDEN_FILTRO, b, Datos_ADC, 1);				//Inicializa el FIR DSP

	while (1){																	//Todas las tareas de FREERTOS van en un loop (excepto que se la borre antes de salir)
		xSemaphoreTake (Semphr_ADC_FIR , portMAX_DELAY);						//COMENTAR PARA DEBUGEAR. Si hubiese ADC, cada 2 interrupciones se tomaría el semáforo que habilita el ADC_Handler.

		if (Indice < ORDEN_FILTRO){												//Comienza la lectura del ADC y la carga de esos datos en el vector Datos_ADC
			Datos_ADC [Indice] = (float) Leer_ADC ();							//Leer_ADC es una función de fantasía (previamente definida)
			Indice ++;
		}

		if (Indice == ORDEN_FILTRO)	{											//Si ya está el vector de muestras completo, aplica el filtro a las muestras
			for (i = 0; i < (ORDEN_FILTRO - 1); i++)							//Realiza un corrimiento en el vector de datos para leer el siguiente valor que entre al ADC
				Datos_ADC [i] = Datos_ADC [i + 1];

			Datos_ADC [ORDEN_FILTRO - 1] = (float) Leer_ADC ();
			Salida_FIR = 0;														//Setea la Salida_FIR en 0

			arm_fir_f32 (&Filtro_FIR, Datos_ADC, &Salida_FIR, 1);				//Aplica el filtro FIR DSP
			xQueueSend(Dato_Salida_FIR, &Salida_FIR, portMAX_DELAY);			//Toma el dato de Salida_FIR con FIR DSP aplicado y lo escribe en la cola Dato_Salida_FIR
		}

	}
}

//Procesamiento de datos impares con IIR
static void Filtrado_IIR (void *pvParameters){

	static float Datos_ADC [ORDEN_FILTRO];
	static float b [ORDEN_FILTRO] = {2.0, 1.45, 2.10, 0.85};					//Coeficientes FIR del filtro IIR
	static float a[ORDEN_FILTRO] = {3.25, 1.0, 4.15, 5.20};						//Coeficientes de feedback del filtro IIR
	static float Salida_IIR;
	static float FIR[ORDEN_FILTRO], Feedback[(ORDEN_FILTRO-1)];				    //Terminos para filtro IIR
	static unsigned int Indice = 0;
	static unsigned int i = 0;

	while (1){																	//Todas las tareas de FREERTOS van en un loop (excepto que se la borre antes de salir)
		xSemaphoreTake (Semphr_ADC_IIR , portMAX_DELAY);						//COMENTAR PARA DEBUGEAR. Si hubiese ADC, cada 2 interrupciones se tomaría el semáforo que habilita el ADC_Handler.

		if (Indice < ORDEN_FILTRO){												//Comienza la lectura del ADC y la carga de esos datos en el vector Datos_ADC
			Datos_ADC [Indice] = (float) Leer_ADC ();							//Leer_ADC es una función de fantasía (previamente definida)
			Indice ++;
		}

		if (Indice == ORDEN_FILTRO)	{											//Si ya está el vector de muestras completo, aplica el filtro a las muestras
			for (i = 0; i < (ORDEN_FILTRO - 1); i++)							//Realiza un corrimiento en el vector de datos para leer el siguiente valor que entre al ADC
				Datos_ADC [i] = Datos_ADC [i + 1];

			Datos_ADC [ORDEN_FILTRO - 1] = (float) Leer_ADC ();
			Salida_IIR = 0;														//Setea la Salida_IIR en 0

			for (i = 0; i < ORDEN_FILTRO; i++){									//Aplica el filtro IIR manual
				FIR[i] = Datos_ADC [i] * b[i];
				if (i == 0)
					Feedback[i] = 0;
				if (i > 0)
					Feedback[i] = FIR[i-1] * a[i-1];
			 Salida_IIR += FIR[i] + Feedback[i];

			 xQueueSend(Dato_Salida_IIR, &Salida_IIR, portMAX_DELAY);			//Toma el dato de Salida_IIR con IIR manual aplicado y lo escribe en la cola Dato_Salida_IIR
			}
		}

	}
}

//Salida de datos del FIR DSP
static void Salida_Datos_FIR (void *pvParameters){

	static float Salida_Datos_FIR;												//Genera la variable para las salida de datos del filtro FIR DSP

	while (1){																	//Todas las tareas de FREERTOS van en un loop (excepto que se la borre antes de salir)
		xQueueReceive (Dato_Salida_FIR, &Salida_Datos_FIR, 0);					//Recibe el dato saliente del procesamiento FIR DSP (FIFO) y lo guarda en Dato_Salida_FIR
		DAC_FIR_Output ( (float) Salida_Datos_FIR);								//Castea los datos de salida del FIR DSP para usarlos de argumento en la funcion de fantasía que saca valores por el DAC
	}
}

//Salida de datos del IIR
static void Salida_Datos_IIR (void *pvParameters){

	static float Salida_Datos_IIR;												//Genera la variable para las salida de datos del filtro IIR

	while (1){																	//Todas las tareas de FREERTOS van en un loop (excepto que se la borre antes de salir)
		xQueueReceive (Dato_Salida_IIR, &Salida_Datos_IIR, 0);					//Recibe el dato saliente del procesamiento IIR (FIFO) y lo guarda en Salida_Datos_IIR
		//DAC_IIR_Output ( (float) Salida_Datos_IIR);								//Castea los datos de salida del IIR para usarlos de argumento en la funcion de fantasía que saca valores por el DAC
	}
}


//Main
int main(void)
{
	prvSetupHardware();

	vSemaphoreCreateBinary(Semphr_ADC_FIR);							//Crea el Semphr_ADC_FIR
	vSemaphoreCreateBinary(Semphr_ADC_FIR);							//Crea el Semphr_ADC_FIR

	Dato_Salida_FIR = xQueueCreate(100, sizeof (float));			//Crea una cola de 100 items máximo de tamaño float cada uno para la salida del filtro FIR DSP
	Dato_Salida_IIR = xQueueCreate(100, sizeof (float));			//Crea una cola de 100 items máximo de tamaño float cada uno para la salida del filtro IIR

	xSemaphoreTake(Semphr_ADC_FIR, portMAX_DELAY);					//Inicializa Semphr_ADC_FIR como tomado
	xSemaphoreTake(Semphr_ADC_IIR, portMAX_DELAY);					//Inicializa Semphr_ADC_IIR como tomado

	xTaskCreate(Filtrado_FIR, "Filtrado_FIR", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);			//Crea tarea de procesamiento de datos con FIR DSP
	xTaskCreate(Filtrado_IIR, "Filtrado_IIR", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);			//Crea tarea de procesamiento de datos con IIR
	xTaskCreate(Salida_Datos_FIR, "Output_FIR", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);		//Crea tarea de salida de datos FIR DSP
	xTaskCreate(Salida_Datos_IIR, "Output_IIR", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);		//Crea tarea de salida de datos IIR

	vTaskStartScheduler();			 								//Inicializa el Scheduler y el SO toma posesión del microcontrolador

	return 1;														//No debería llegar nunca acá
}
