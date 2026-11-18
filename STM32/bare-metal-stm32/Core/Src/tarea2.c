Main · C
/*
 * =====================================================================================
 *  CONTROL DE LED RGB Y SALIDA ANALOGICA CON ENCODER ROTATIVO Y MAQUINA DE ESTADOS
 * =====================================================================================
 *
 *  Universidad Nacional de Colombia
 *  Curso:        [Nombre del curso]
 *  Profesor:     [Nombre del profesor]
 *  Equipo:       Juan Pablo [Apellido]
 *                [Integrante 2]
 *                [Integrante 3]
 *  Fecha:        [dd/mm/aaaa]
 *  Version:      1.0
 *
 * -------------------------------------------------------------------------------------
 *  1. DESCRIPCION GENERAL
 * -------------------------------------------------------------------------------------
 *  El programa permite controlar con un solo encoder rotativo:
 *    - El brillo de cada color de un LED RGB (rojo, verde y azul) por separado.
 *    - El brillo de los tres colores a la vez (modo blanco).
 *    - Un voltaje de salida de 0 a 3.3 V, generado con PWM y un filtro RC.
 *
 *  Girar el encoder sube o baja el valor del modo activo. Presionar el pulsador del
 *  encoder (SW) pasa al siguiente modo, en este orden:
 *
 *      ROJO -> VERDE -> AZUL -> BLANCO -> VOLTAJE -> ROJO ...
 *
 *  El sistema arranca en modo BLANCO, con los tres LED al 25 % y la salida en 0 V.
 *  Un LED de estado parpadea a 4 Hz para indicar que el programa esta corriendo.
 *
 * -------------------------------------------------------------------------------------
 *  2. HARDWARE UTILIZADO
 * -------------------------------------------------------------------------------------
 *  Tarjeta:      NUCLEO-F411RE (microcontrolador STM32F411RE, ARM Cortex-M4)
 *  Reloj:        HSI interno de 16 MHz (valor por defecto, sin PLL).
 *                APB1 y APB2 a 16 MHz, por lo que todos los timers cuentan a 16 MHz.
 *
 *  Componentes externos:
 *    - LED RGB de catodo comun, con una resistencia limitadora por color
 *      (sugerido: 150 ohm para el rojo y 68 ohm para verde y azul a 3.3 V).
 *    - Encoder rotativo de 5 pines (CLK, DT, SW, +, GND), tipo KY-040,
 *      alimentado a 3.3 V.
 *    - Filtro RC pasa-bajos en la salida analogica: R = 10 kohm en serie y
 *      C = 1 uF a GND. Frecuencia de corte fc = 1/(2*pi*R*C) = 16 Hz.
 *    - LED de estado externo con resistencia de 220-330 ohm a GND.
 *
 *  Perifericos del microcontrolador:
 *    - TIM2  (32 bits): modo encoder x4. Cuenta los flancos de CLK y DT con signo.
 *    - TIM3:            PWM de 1 kHz para los tres LED (CH2, CH3 y CH4).
 *                       PSC = 15, ARR = 999 -> CCR de 0 a 1000 equivale a 0-100 %.
 *    - TIM10:           PWM de 4.85 kHz para la salida analogica (CH1).
 *                       PSC = 0, ARR = 3299 -> el CCR equivale a milivoltios.
 *    - TIM4:            base de tiempo de 1 ms por interrupcion (blinky y antirrebote).
 *    - EXTI4:           interrupcion por flanco de bajada del pulsador.
 *    - GPIO:            salida digital del LED de estado.
 *
 * -------------------------------------------------------------------------------------
 *  3. ASIGNACION DE PINES
 * -------------------------------------------------------------------------------------
 *   Pin   Funcion            Uso                        Conector en la NUCLEO
 *   ----  -----------------  -------------------------  ---------------------
 *   PB0   TIM3_CH3  (AF2)    LED rojo                   A3  / CN8-4
 *   PB5   TIM3_CH2  (AF2)    LED verde                  D4  / CN9-5
 *   PB1   TIM3_CH4  (AF2)    LED azul                   CN10-24 (Morpho)
 *   PB8   TIM10_CH1 (AF3)    Salida analogica (filtro)  D15 / CN5-10
 *   PA0   TIM2_CH1  (AF1)    Encoder CLK (canal A)      A0  / CN8-1
 *   PA1   TIM2_CH2  (AF1)    Encoder DT  (canal B)      A1  / CN8-2
 *   PA4   EXTI4              Pulsador SW (pull-up)      A2  / CN8-3
 *   PH1   Salida GPIO        LED de estado (blinky)     CN7-31 (Morpho)
 *   3V3   -                  Alimentacion del encoder   CN6-4
 *   GND   -                  Tierra comun               CN6-6 / CN6-7
 *
 *  Nota: PH1 es el pin OSC_OUT. Verificar en el manual UM1724 que los puentes de
 *  soldadura de la tarjeta lo conecten a CN7-31.
 *
 * -------------------------------------------------------------------------------------
 *  4. FUNCIONAMIENTO DEL SISTEMA
 * -------------------------------------------------------------------------------------
 *  El programa usa una maquina de estados finitos (FSM) guiada por eventos:
 *
 *    a) Las interrupciones solo levantan banderas y no hacen trabajo pesado:
 *         - TIM4 (cada 1 ms): cuenta 125 ms para el blinky y termina el antirrebote.
 *         - EXTI4: detecta el primer flanco del pulsador, silencia la linea y
 *           arranca una espera de 30 ms. Si al terminarla el boton sigue
 *           presionado, se confirma la pulsacion.
 *       El encoder no usa interrupcion: el TIM2 cuenta solo, por hardware.
 *
 *    b) En el lazo principal, el estado de reposo (STATE_NULL) llama a
 *       get_next_event(), que convierte banderas y conteo del encoder en un evento
 *       con esta prioridad: pulsador -> encoder -> blinky.
 *
 *    c) Segun el evento, la FSM pasa a un estado que ejecuta una sola accion y
 *       regresa al reposo:
 *         EV_MODE    -> STATE_CAMBIO_MODO: pasa al siguiente modo de edicion.
 *         EV_ENCODER -> STATE_ROJO / VERDE / AZUL / BLANCO / VOLTAJE (segun el modo):
 *                       lee los pasos del encoder, ajusta el valor con saturacion
 *                       entre 0 y el maximo y lo escribe en el CCR correspondiente.
 *         EV_BLINKY  -> STATE_BLINKY: invierte el LED de estado.
 *
 *    d) Cada paso del encoder (4 flancos) cambia un LED en 1 % (10 cuentas de CCR)
 *       o el voltaje en 10 mV. El voltaje esta limitado por MAX_MV_VOLTAJE.
 *
 *    e) El voltaje de salida es el valor medio del PWM, que el filtro RC entrega:
 *           Vout = 3.3 V * CCR / (ARR + 1) = CCR en mV
 *
 * =====================================================================================
 */

#include <stdint.h>
#include "stm32f411xe.h"
#include "stm32f4xx.h"

// CONSTANTES
#define RESOLUCION_ENCODER 4    // Cuenta 4 flancos por cada paso mecanico del encoder

// PWM de los LED (TIM3): 16 MHz / (15 + 1) = 1 MHz; 1 MHz / (999 + 1) = 1 kHz
#define LED_PSC 15              // Prescaler del TIM3
#define LED_ARR 999             // ARR del TIM3 (1000 cuentas por periodo)
#define LED_CCR_MAX (LED_ARR + 1) // Tope del CCR de los LED. ARR + 1 = 100 %
#define INIT_CCR_LED 250        // Rojo, verde y azul inician en 25 % (blanco tenue)
#define LED_STEP 10             // Lo que suma o resta el CCR con cada paso del encoder (1 %)

// PWM de la salida analogica (TIM10): 16 MHz / (3299 + 1) = 4.85 kHz
// Con ARR = 3299, el CCR equivale directamente a milivoltios (3300 -> 3.3 V)
#define VOLT_PSC 0              // Prescaler del TIM10
#define VOLT_ARR 3299           // ARR del TIM10 (3300 cuentas por periodo)
#define INIT_MV_VOLTAJE 0       // Inicia en 0 V
#define MAX_MV_VOLTAJE 3300      // 0.33 V para pruebas. Subir hasta 3300 para llegar a 3.3 V
#define VOLT_STEP_MV 10         // Lo que suma o resta el voltaje con cada paso del encoder (10 mV)

#define BLINKY_SEMIPERIODO_MS 125 // Cambia cada 125 ms (4 Hz)
#define ANTIRREBOTE_MS 30       // Espera antes de confirmar que el boton sigue presionado

// MAQUINA DE ESTADOS FINITOS (FSM)
// Modos de edicion
typedef enum{
    MODE_ROJO,    // El encoder cambia el LED rojo
    MODE_VERDE,   // El encoder cambia el LED verde
    MODE_AZUL,    // El encoder cambia el LED azul
    MODE_BLANCO,  // El encoder cambia los tres LED a la vez (brillo del blanco)
    MODE_VOLTAJE  // El encoder cambia el voltaje de salida
}edit_mode_t;

// Estados
typedef enum{
    STATE_NULL,        // No hace nada, espera un evento
    STATE_ROJO,        // Aplica los pasos del encoder al LED rojo
    STATE_VERDE,       // Aplica los pasos del encoder al LED verde
    STATE_AZUL,        // Aplica los pasos del encoder al LED azul
    STATE_BLANCO,      // Aplica los pasos del encoder a los tres LED
    STATE_VOLTAJE,     // Aplica los pasos del encoder a la salida analogica
    STATE_CAMBIO_MODO, // Atiende el cambio de modo
    STATE_BLINKY,      // Atiende el parpadeo del blinky
    STATE_ERROR,       // Lleva la maquina de estados a STATE_NULL si se sale del rango
    NUM_STATE          // CENTINELA: avisa que se salio del rango
}state_t;

// Eventos
typedef enum{
    EV_NULL,     // No hay ningun evento
    EV_ENCODER,  // El encoder avanzo o retrocedio al menos un paso
    EV_MODE,     // Se presiono el pulsador para pasar al siguiente modo
    EV_BLINKY,   // Se cumplio el semiperiodo del blinky
    NUM_EVENTS   // CENTINELA: avisa si se sale del rango
}evento_t;

/* VARIABLES */
static state_t estado_actual = STATE_NULL; // Empezar en el estado de reposo
static uint8_t state_entry_flag = 1;       // Vale 1 al llegar a un estado para que su accion solo se ejecute una vez

volatile uint8_t boton_flag = 0;      // Bandera que deja el antirrebote cuando confirma la pulsacion
volatile uint8_t blinky_flag = 0;     // Bandera que deja el TIM4 cuando se cumple el semiperiodo
volatile uint8_t boton_espera_ms = 0; // Milisegundos que faltan para confirmar el boton (0 = sin espera)

static edit_mode_t edit_mode = MODE_BLANCO;       // Iniciar en modo BLANCO
static int32_t ccr_rojo = INIT_CCR_LED;           // CAMBIO: CCR3 del TIM3 (rojo), antes CCR1
static int32_t ccr_verde = INIT_CCR_LED;          // CCR2 del TIM3 (verde)
static int32_t ccr_azul = INIT_CCR_LED;           // CCR4 del TIM3 (azul)
static int32_t mv_voltaje = INIT_MV_VOLTAJE;      // CCR1 del TIM10, en milivoltios

// FUNCIONES DE LA FSM
static evento_t get_next_event(void);      // Lee y limpia las banderas y el encoder y lo convierte en evento
static state_t estado_del_modo(void);      // Elige el estado de ajuste segun el modo
static int32_t encoder_steps(void);        // Devuelve los pasos acumulados con signo
static int32_t ajustar_valor(int32_t valor, int32_t steps,
                             int32_t paso, int32_t maximo); // Suma pasos y satura entre 0 y maximo
static void mode(void);                    // Pasa al siguiente modo de edicion
static void pulso(void);                   // Cambia el estado del blinky

// INICIALIZACION DEL HARDWARE
static void init_GPIO(void);          // Pin del blinky
static void init_TIM4(void);          // Base de tiempo de 1 ms (blinky y antirrebote)
static void init_tim3_PWM(void);      // PWM de los LED: CH3 (rojo), CH2 (verde) y CH4 (azul)
static void init_tim10_PWM(void);     // PWM de la salida analogica
static void init_tim2_ENCODER(void);  // PA0 + PA1 + TIM2 en modo encoder
static void init_EXTI4(void);         // CAMBIO: antes init_EXTI0. Pulsador en PA4, flanco de bajada

int main(void){

    evento_t evento_actual = EV_NULL; // Comienza sin eventos

    init_GPIO();
    init_tim3_PWM();
    init_tim10_PWM();
    init_tim2_ENCODER();
    init_EXTI4();        
    init_TIM4();

    while(1){
        if(estado_actual == STATE_NULL){
            evento_actual = get_next_event(); // Solo el estado de reposo consulta eventos
        }
        else{
            evento_actual = EV_NULL;
        }

        // Validar que el estado y el evento esten en rango antes de usarlos
        if((estado_actual >= NUM_STATE) || (evento_actual >= NUM_EVENTS)){
            estado_actual = STATE_ERROR;
            state_entry_flag = 1;
        }

        switch (estado_actual) {
            case STATE_NULL: // Estado de reposo
                if(state_entry_flag == 1){
                    state_entry_flag = 0;
                }

                // TRANSICIONES
                if(evento_actual == EV_ENCODER){
                    estado_actual = estado_del_modo();
                    state_entry_flag = 1;
                }
                else if(evento_actual == EV_MODE){
                    estado_actual = STATE_CAMBIO_MODO;
                    state_entry_flag = 1;
                }
                else if(evento_actual == EV_BLINKY){
                    estado_actual = STATE_BLINKY;
                    state_entry_flag = 1;
                }
                break;

            // Cada estado de color actua solo sobre su canal
            case STATE_ROJO:
                if(state_entry_flag == 1){
                    ccr_rojo = ajustar_valor(ccr_rojo, encoder_steps(), LED_STEP, LED_CCR_MAX);
                    TIM3->CCR3 = ccr_rojo; // CAMBIO: antes CCR1
                    state_entry_flag = 0;
                }
                estado_actual = STATE_NULL; // Transicion incondicional
                state_entry_flag = 1;
                break;

            case STATE_VERDE:
                if(state_entry_flag == 1){
                    ccr_verde = ajustar_valor(ccr_verde, encoder_steps(), LED_STEP, LED_CCR_MAX);
                    TIM3->CCR2 = ccr_verde;
                    state_entry_flag = 0;
                }
                estado_actual = STATE_NULL;
                state_entry_flag = 1;
                break;

            case STATE_AZUL:
                if(state_entry_flag == 1){
                    ccr_azul = ajustar_valor(ccr_azul, encoder_steps(), LED_STEP, LED_CCR_MAX);
                    TIM3->CCR4 = ccr_azul;
                    state_entry_flag = 0;
                }
                estado_actual = STATE_NULL;
                state_entry_flag = 1;
                break;

            case STATE_BLANCO:
                if(state_entry_flag == 1){
                    // Los pasos se leen una sola vez y se aplican a los tres canales
                    int32_t steps = encoder_steps();
                    ccr_rojo  = ajustar_valor(ccr_rojo,  steps, LED_STEP, LED_CCR_MAX);
                    ccr_verde = ajustar_valor(ccr_verde, steps, LED_STEP, LED_CCR_MAX);
                    ccr_azul  = ajustar_valor(ccr_azul,  steps, LED_STEP, LED_CCR_MAX);
                    TIM3->CCR3 = ccr_rojo; // CAMBIO: antes CCR1
                    TIM3->CCR2 = ccr_verde;
                    TIM3->CCR4 = ccr_azul;
                    state_entry_flag = 0;
                }
                estado_actual = STATE_NULL;
                state_entry_flag = 1;
                break;

            case STATE_VOLTAJE:
                if(state_entry_flag == 1){
                    mv_voltaje = ajustar_valor(mv_voltaje, encoder_steps(), VOLT_STEP_MV, MAX_MV_VOLTAJE);
                    TIM10->CCR1 = mv_voltaje;
                    state_entry_flag = 0;
                }
                estado_actual = STATE_NULL;
                state_entry_flag = 1;
                break;

            case STATE_CAMBIO_MODO:
                if(state_entry_flag == 1){
                    mode();
                    state_entry_flag = 0;
                }
                estado_actual = STATE_NULL;
                state_entry_flag = 1;
                break;

            case STATE_BLINKY:
                if(state_entry_flag == 1){
                    pulso();
                    state_entry_flag = 0;
                }
                estado_actual = STATE_NULL;
                state_entry_flag = 1;
                break;

            case STATE_ERROR:
            default:
                estado_actual = STATE_NULL; // Vuelve al reposo sin hacer nada
                state_entry_flag = 1;
                break;
        }
    }

    return 0;
}

// FUNCIONES DRIVER DE LA FSM
// Convierte las banderas de ISR y el encoder en eventos. Prioridad: pulsador -> encoder -> blinky
static evento_t get_next_event(void){
    if(boton_flag == 1){
        boton_flag = 0;
        return EV_MODE;
    }

    int32_t count = (int32_t) TIM2->CNT; // Con signo: flancos acumulados en el encoder
    if(count >= RESOLUCION_ENCODER || count <= -RESOLUCION_ENCODER){
        return EV_ENCODER; // La bandera se "limpia" en encoder_steps()
    }

    if(blinky_flag == 1){
        blinky_flag = 0;
        return EV_BLINKY;
    }
    return EV_NULL;
}

// Traduce el modo de edicion al estado que debe atender el encoder
static state_t estado_del_modo(void){
    switch(edit_mode){
        case MODE_ROJO:    return STATE_ROJO;
        case MODE_VERDE:   return STATE_VERDE;
        case MODE_AZUL:    return STATE_AZUL;
        case MODE_BLANCO:  return STATE_BLANCO;
        case MODE_VOLTAJE: return STATE_VOLTAJE;
        default:           return STATE_ERROR;
    }
}

// Devuelve los pasos completos acumulados y los descuenta del TIM2
static int32_t encoder_steps(void){
    int32_t steps = (int32_t) TIM2->CNT / RESOLUCION_ENCODER; // Pasos enteros
    TIM2->CNT -= steps * RESOLUCION_ENCODER; // Solo queda el residuo
    return steps;
}

// Suma steps x paso al valor, lo satura entre 0 y maximo y devuelve el resultado.
// Quien la llama guarda el resultado en su variable y lo escribe en su CCR
static int32_t ajustar_valor(int32_t valor, int32_t steps,
                             int32_t paso, int32_t maximo){
    valor += steps * paso;
    if(valor < 0){
        valor = 0; // No puede bajar de cero
    }
    if(valor > maximo){
        valor = maximo; // No puede pasar del maximo
    }
    return valor;
}

// Pasa al siguiente modo: ROJO -> VERDE -> AZUL -> BLANCO -> VOLTAJE -> ROJO
static void mode(void){
    switch(edit_mode){
        case MODE_ROJO:    edit_mode = MODE_VERDE;   break;
        case MODE_VERDE:   edit_mode = MODE_AZUL;    break;
        case MODE_AZUL:    edit_mode = MODE_BLANCO;  break;
        case MODE_BLANCO:  edit_mode = MODE_VOLTAJE; break;
        case MODE_VOLTAJE: edit_mode = MODE_ROJO;    break;
        default:           edit_mode = MODE_BLANCO;  break; // Recupera un modo invalido
    }
}

// Funcion de parpadeo
static void pulso(void){
    GPIOH->ODR ^= GPIO_ODR_OD1; // Cambiar el estado del LED PH1
}

// INICIALIZACION DEL HARDWARE
static void init_GPIO(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
    GPIOH->MODER |= GPIO_MODER_MODE1_0; // PH1 como salida
}

// Base de tiempo de 1 ms: 16 MHz / 16 = 1 MHz; 1 MHz / 1000 = 1 kHz
static void init_TIM4(void){
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    TIM4->PSC = 15;
    TIM4->ARR = 999;

    TIM4->DIER |= TIM_DIER_UIE;  // Interrupcion por actualizacion
    NVIC_EnableIRQ(TIM4_IRQn);   // Permiso del NVIC

    TIM4->CR1 = TIM_CR1_CEN;
}

// PWM de los LED en TIM3
// CAMBIO: el rojo paso de PB4 (CH1) a PB0 (CH3). Ya no se usa el CH1
static void init_tim3_PWM(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // PB0 (CH3), PB5 (CH2) y PB1 (CH4) en funcion alternativa AF2
    GPIOB->MODER |= (GPIO_MODER_MODE0_1 | GPIO_MODER_MODE5_1 | GPIO_MODER_MODE1_1);
    GPIOB->AFR[0] |= (GPIO_AFRL_AFSEL0_1 | GPIO_AFRL_AFSEL5_1 | GPIO_AFRL_AFSEL1_1);

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->PSC = LED_PSC;
    TIM3->ARR = LED_ARR;
    TIM3->CCR3 = ccr_rojo;   
    TIM3->CCR2 = ccr_verde;
    TIM3->CCR4 = ccr_azul;

    // CH2 vive en CCMR1; CH3 y CH4 viven en CCMR2. PWM modo 1 (110) con preload
    TIM3->CCMR1 |= (TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE);
    TIM3->CCMR2 |= (TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3PE); // CAMBIO: CH3 (antes CH1 en CCMR1)
    TIM3->CCMR2 |= (TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4PE);

    TIM3->CCER |= (TIM_CCER_CC3E | TIM_CCER_CC2E | TIM_CCER_CC4E); // CAMBIO: CC3E en vez de CC1E

    TIM3->CR1 |= TIM_CR1_ARPE; // Preload del ARR
    TIM3->EGR = TIM_EGR_UG;    // Carga ARR y CCR en los registros sombra (una sola vez)
    TIM3->CR1 |= TIM_CR1_CEN; // contar
}

// PWM de la salida analogica en TIM10_CH1 (PB8, AF3)
// TIM10 cuelga de APB2 (16 MHz con HSI) y no necesita el bit MOE como el TIM1
static void init_tim10_PWM(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    GPIOB->MODER |= GPIO_MODER_MODE8_1;                         // PB8 en funcion alternativa
    GPIOB->AFR[1] |= (GPIO_AFRH_AFSEL8_0 | GPIO_AFRH_AFSEL8_1); // AF3 = 0011 para TIM10

    RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;

    TIM10->PSC = VOLT_PSC;
    TIM10->ARR = VOLT_ARR;
    TIM10->CCR1 = mv_voltaje;

    TIM10->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE); // PWM modo 1 con preload
    TIM10->CCER |= TIM_CCER_CC1E;

    TIM10->CR1 |= TIM_CR1_ARPE; // Preload del ARR
    TIM10->EGR = TIM_EGR_UG; // Carga ARR y CCR en los registros sombra (una sola vez)
    TIM10->CR1 |= TIM_CR1_CEN; // Contar
}

static void init_tim2_ENCODER(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER |= (GPIO_MODER_MODE0_1 | GPIO_MODER_MODE1_1);   // PA0 y PA1 en funcion alterna
    GPIOA->AFR[0] |= (GPIO_AFRL_AFSEL0_0 | GPIO_AFRL_AFSEL1_0); // AF1 para TIM2
    // CAMBIO: nuevo. Pull-up en PA0 y PA1 por si el encoder no trae resistencias (encoder suelto).
    // Con un modulo KY-040 tambien funciona: quedan en paralelo con las del modulo
    GPIOA->PUPDR |= (GPIO_PUPDR_PUPD0_0 | GPIO_PUPDR_PUPD1_0);

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->ARR = 0xFFFFFFFF;                                // ARR al maximo (TIM2 es de 32 bits)
    TIM2->CCMR1 |= (TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0);   // CH1 lee TI1 (PA0) y CH2 lee TI2 (PA1)
    TIM2->CCMR1 |= (TIM_CCMR1_IC1F | TIM_CCMR1_IC2F);       // Filtro en ambas entradas

    TIM2->SMCR |= (TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1);        // Modo encoder 3 (x4)
    TIM2->CR1 |= TIM_CR1_CEN;
}

// CAMBIO: antes init_EXTI0 con PB0. Pulsador SW en PA4 por EXTI4
static void init_EXTI4(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD4_0; // 01 = pull-up (PA4 queda como entrada por defecto)

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[1] &= ~SYSCFG_EXTICR2_EXTI4; // 0000 = puerto A para EXTI4

    EXTI->FTSR |= EXTI_FTSR_TR4; // Flanco de bajada
    EXTI->IMR |= EXTI_IMR_MR4;
    NVIC_EnableIRQ(EXTI4_IRQn);
}

// INTERRUPCIONES
// TIM4: cada 1 ms. Genera el blinky y termina el antirrebote del pulsador
void TIM4_IRQHandler(void){
    static uint16_t contador_blinky = 0;

    TIM4->SR = ~TIM_SR_UIF; // Bajar la bandera sin borrar otras

    contador_blinky++;
    if(contador_blinky >= BLINKY_SEMIPERIODO_MS){
        contador_blinky = 0;
        blinky_flag = 1;
    }

    // Antirrebote del pulsador
    if(boton_espera_ms > 0){
        boton_espera_ms--;
        if(boton_espera_ms == 0){
            if((GPIOA->IDR & GPIO_IDR_ID4) == 0){ // CAMBIO: antes GPIOB / ID0. Sigue presionado (activo en bajo)
                boton_flag = 1;
            }
            EXTI->PR = EXTI_PR_PR4;    // CAMBIO: antes PR0. Descarta los rebotes que quedaron pendientes
            EXTI->IMR |= EXTI_IMR_MR4; // CAMBIO: antes MR0. Vuelve a escuchar el pulsador
        }
    }
}

// CAMBIO: antes EXTI0_IRQHandler. Primer flanco de bajada del pulsador en PA4
void EXTI4_IRQHandler(void){
    EXTI->PR = EXTI_PR_PR4;      // Bajar la bandera de interrupcion
    // Silencia el EXTI4 y arranca la espera del antirrebote
    EXTI->IMR &= ~EXTI_IMR_MR4;
    boton_espera_ms = ANTIRREBOTE_MS;
}





