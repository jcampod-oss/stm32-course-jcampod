#include <stdint.h>
#include "stm32f411xe.h"
#include "stm32f4xx.h"
 
// Declarando que funciones de configuracion para el micro
void init_GPIO(void); 
void init_TIMER(void);
void init_TIMER2(void);
void init_EXTI1(void);

// Definicion de la funcion dibujar en el display
void DIBUJAR(uint8_t valor);
 
// Definicion  de las funciones para cada tipo de numero
void init_CERO(void);
void init_UNO(void);
void init_DOS(void);
void init_TRES(void);
void init_CUATRO(void);
void init_CINCO(void);
void init_SEIS(void);
void init_SIETE(void);
void init_OCHO(void);
void init_NUEVE(void);
 
 
// Definicion de variables que mencionan que hubo interrupcion
volatile uint8_t cambio = 0;
volatile uint8_t  Csensor1 = 0;
volatile uint8_t Csensor2 = 0;
 
// Definicion de estados digito
typedef enum{
 
    DigitoUno,
    DigitoDos,
    DigitoTres,
    DigitoCuatro,
 
} TipoDigito;
 
TipoDigito digito = DigitoCuatro; // Establecer el estado inicial

volatile uint16_t conteo = 0;  //iniciando el conteo en cero
uint8_t cifras[4] = {0}; // arreglo para guardar las cifras del conteo
 
// iniciando el programa principal
int main(void){

    // iniciando funciones principales
    init_GPIO();
    init_TIMER();
    init_TIMER2();
    init_EXTI1();
 
    //Iniciando en 0
    


    // crear variable de cambio
    uint16_t copia = 0;
 
    while(1){

        // Actualizar la lista de cifras
        if(Csensor1 == 1 || Csensor2 == 1){

            if(Csensor1 == 1 && conteo < 9999){
                conteo++; // suma en 1 al conteo
            }
    
            if(Csensor2 == 1 && conteo > 0){
                conteo--; // resta en 1 al conteo
            }
            
            Csensor1 = 0; // bajamos el aviso del sensor 1
            Csensor2 = 0; // bajamos el aviso del sensor 2

            copia = conteo; // Hacer la copia para no afectar el conteo original 

            for (uint8_t i = 0; i < 4; i++){
                cifras[i] = copia % 10;   // toma la última cifra
                copia = copia / 10;      // la elimina
            }

        }
        
        // Aplicamos las interrupciones para cambiar los digitos
        if (cambio == 1){
            cambio = 0;
 
            switch(digito){
                case DigitoCuatro:
    
                    GPIOA->ODR |= GPIO_ODR_OD0; // Apagar el led D3
                    GPIOC->ODR |= GPIO_ODR_OD0; // Apagar el led D2
                    GPIOB->ODR |= GPIO_ODR_OD1; // Apagar el led D1
                    DIBUJAR(cifras[0]); // Dibujando el numero en el led
                    GPIOA->ODR &= ~(GPIO_ODR_OD8); // Encender el led D4
                    digito = DigitoTres;
                    break;
 
                case DigitoTres:

                    GPIOA->ODR |= GPIO_ODR_OD8; // Apagar el led D4
                    GPIOC->ODR |= GPIO_ODR_OD0; // Apagar el led D2
                    GPIOB->ODR |= GPIO_ODR_OD1; // Apagar el led D1
                    DIBUJAR(cifras[1]); // Dibujando el numero en el led
                    GPIOA->ODR &= ~(GPIO_ODR_OD0); // Encender el led D3
                    digito = DigitoDos;
                    break;
 
                case DigitoDos:
                    
                    GPIOA->ODR |= GPIO_ODR_OD8; // Apagar el led D4
                    GPIOA->ODR |= GPIO_ODR_OD0; // Apagar el led D3
                    GPIOB->ODR |= GPIO_ODR_OD1; // Apagar el led D1
                    DIBUJAR(cifras[2]); // Dibujando el numero en el led
                    GPIOC->ODR &= ~(GPIO_ODR_OD0); // Encender el led D2
                    digito = DigitoUno;
                    break;
 
                case DigitoUno:
    
                    GPIOA->ODR |= GPIO_ODR_OD8; // Apagar el led D4
                    GPIOA->ODR |= GPIO_ODR_OD0; // Apagar el led D3
                    GPIOC->ODR |= GPIO_ODR_OD0; // Apagar el led D2
                    DIBUJAR(cifras[3]); // Dibujando el numero en el led
                    GPIOB->ODR &= ~(GPIO_ODR_OD1); // Encender el led D1
                    digito = DigitoCuatro;
                    break;
 
            }
        }
       
    }
}
 
// Configurando todos los pines del micro 
void init_GPIO(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Configurando la señal de reloj
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Activando la señal de reloj en GPIOB
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;// activando la señal de reloj en GPIOC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN; 
    
// Configurando los pines de los segmentos
    
 
// Configurando el pin PA4 para segemento a
    GPIOA->MODER |= GPIO_MODER_MODE4_0; // Configruando el PC0 en salida
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT4); // Configurando el pin como push pull
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR4_1; // Configurando la  velociadad
    GPIOA->ODR |= GPIO_ODR_OD4; // Inicial en 3.3V
 
// Configurando el pin PB0 para segmento b
    GPIOB->MODER |= GPIO_MODER_MODE0_0; // Configruando el PC1 en salida
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT0); // Configurando el pin como push pull
    GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR0_1; // Configurando la  velociadad
    GPIOB->ODR |= GPIO_ODR_OD0; // Inicial en 3.3V
 
// Configurando el pin PB14 para segemento c
    GPIOB->MODER |= GPIO_MODER_MODE14_0; // Configruando el PC1 en salida
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT14); // Configurando el pin como push pull
    GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR14_1; // Configurando la  velociadad
    GPIOB->ODR |= GPIO_ODR_OD14; // Inicial en 3.3V
 
    // Configurando el pin PC3 para segmento d
    GPIOB->MODER |= GPIO_MODER_MODE10_0; // Configruando el PC3 en salida
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT10); // Configurando el pin como push pull
    GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10_1; // Configurando la  velociadad
    GPIOB->ODR |= GPIO_ODR_OD10; // Inicial en 3.3V
 
// Configurando PC4 para segmento e
    GPIOC->MODER |= GPIO_MODER_MODE4_0; // Configruando el PC4 en salida
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT4); // Configurando el pin como push pull
    GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR4_1; // Configurando la  velociadad
    GPIOC->ODR |= GPIO_ODR_OD4; // Inicial en 3.3V
 
// Configurando el pin PB5 para segemento f
    GPIOB->MODER |= GPIO_MODER_MODE5_0; // Configruando el PC5 en salida
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT5); // Configurando el pin como push pull
    GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR5_1; // Configurando la  velociadad
    GPIOB->ODR |= GPIO_ODR_OD5; // Inicial en 3.3V
 
// Configurando el pin PB15 para segmento g
    GPIOB->MODER |= GPIO_MODER_MODE15_0; // Configruando el PC6 en salida
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT15); // Configurando el pin como push pull
    GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR15_1; // Configurando la  velociadad
    GPIOB->ODR |= GPIO_ODR_OD15; // Inicial en 3.3V
 
//Configurando pines para los digitos
 
// Configurando el led D1
    GPIOB->MODER |= GPIO_MODER_MODE1_0; // Configruando el PB12 en salida
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT1); // Configurando el pin como push pull
    GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR1_1; // Configurando la  velociadad
    GPIOB->ODR |= GPIO_ODR_OD1; // Inicial en 3.3V
 
// Configurando el led D2
    GPIOC->MODER |= GPIO_MODER_MODE0_0; // Configruando el PB12 en salida
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT0); // Configurando el pin como push pull
    GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR0_1; // Configurando la  velociadad
    GPIOC->ODR |= GPIO_ODR_OD0; // Inicial en 3.3V
 
// Configurando el led D3
    GPIOA->MODER |= GPIO_MODER_MODE0_0; // Configruando el PB14 en salida
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT0); // Configurando el pin como push pull
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR0_1; // Configurando la  velociadad
    GPIOA->ODR |= GPIO_ODR_OD0; // Inicial en 3.3V
 
// Configurando el led D4
    GPIOA->MODER |= GPIO_MODER_MODE8_0; // Configruando el PB15 en salida
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT8); // Configurando el pin como push pull
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8_1; // Configurando la  velociadad
    GPIOA->ODR |= GPIO_ODR_OD8; // Inicial en 3.3V
 
// Configurando los pines de los sensores   
 
// Congifurando el pin GPIO del sensor 1
    GPIOB->MODER &= ~(GPIO_MODER_MODE9_Msk); // Configruando el PA0 en entrada
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT9); // Configurando el pin como push pull
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD9_Msk); // Sin pull up
 
// Congifurando el pin GPIO del sensor 2
    GPIOB->MODER &= ~(GPIO_MODER_MODE12_Msk); // Configruando el PA1 en entrada
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT12); // Configurando el pin como push pull
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD12_Msk); // Sin pull up
    

// Configurando el pin para el led de vida

GPIOH->MODER |= (GPIO_MODER_MODE1_0); // Configurando el PH1 como salida
GPIOH->OTYPER &= ~(GPIO_OTYPER_OT1); // Congifurando el pin como push pull
GPIOH->PUPDR &= ~(GPIO_PUPDR_PUPD1_Msk); // Sin pull up

GPIOH->ODR |= GPIO_ODR_OD1; // Encendiendo el led de la board de desarrollo
}
 
//Configurando la interrupcion del timer para display
void init_TIMER(void){
    
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  // Activar las eñal de reloj para el timer2
 
    TIM2->CNT = 0; //Reiniciando contador;
    TIM2->PSC = 1600 - 1; // divisor por  1600
    TIM2->ARR = 30 - 1; // Configurando el reload
 
    TIM2->CR1 &= ~(TIM_CR1_DIR); // Contar hacia arriba
    TIM2->CR1 &= ~(TIM_CR1_CMS_Msk); // contar desde abajo 
    TIM2->CR1 &= ~(TIM_CR1_ARPE); 
    
    TIM2->DIER |= TIM_DIER_UIE; // Habilitando la interrupcion del tim2-paso 1
 
    NVIC_EnableIRQ(TIM2_IRQn); // Hbilitando la interrupcion del tim2- paso 2
 
    TIM2->CR1 |= TIM_CR1_CEN; // Contando
}
 
// Funcion de interrupcion automatica del display
void TIM2_IRQHandler(void){
    // Verificando hubo interrupcion 
    if(TIM2->SR & TIM_SR_UIF){
        TIM2->SR &= ~(TIM_SR_UIF); // Bajamos la bandera
 
        cambio = 1; // Avisamos del cambio
    }
 
}

// Configurando la interrupcion automatica para el led
void init_TIMER2(void){

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;  // Activar las eñal de reloj para el timer3

    TIM3->CNT = 0; //Reiniciando contador;
    TIM3->PSC = 1600 - 1; // divisor por  1600
    TIM3->ARR = 2500 - 1; // Configurando el reload
 
    TIM3->CR1 &= ~(TIM_CR1_DIR); // Contar hacia arriba
    TIM3->CR1 &= ~(TIM_CR1_CMS_Msk); // contar desde abajo 
    TIM3->CR1 &= ~(TIM_CR1_ARPE); 
    
    TIM3->DIER |= TIM_DIER_UIE; // Habilitando la interrupcion del tim3-paso 1
 
    NVIC_EnableIRQ(TIM3_IRQn); // Hbilitando la interrupcion del tim3- paso 2
 
    TIM3->CR1 |= TIM_CR1_CEN; // Contando
}

// Funcion de interrupcion automatica del led de vida
void TIM3_IRQHandler(void){
    // Verificando hubo interrupcion 
    if(TIM3->SR & TIM_SR_UIF){
        TIM3->SR &= ~(TIM_SR_UIF); // Bajamos la bandera
 
        GPIOH->ODR ^= GPIO_ODR_OD1; // Cambiando el estado del led de vida
    }
 
}
 
// Configurando las interrupciones de los sensores
void init_EXTI1(void){

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Habilitando el reloj de SYSCFG

   
    SYSCFG->EXTICR[2] &= ~(SYSCFG_EXTICR3_EXTI9_Msk); // Limpiar el campo de la linea 9
    SYSCFG->EXTICR[2] |=  SYSCFG_EXTICR3_EXTI9_PB;    // Linea 9 escucha al puerto B
    EXTI->RTSR |= EXTI_FTSR_TR9;                      // Flanco de SUBIDA
    EXTI->IMR  |= EXTI_IMR_IM9;                       // Desenmascarar la linea 9

    
    SYSCFG->EXTICR[3] &= ~(SYSCFG_EXTICR4_EXTI12_Msk); // Limpiar el campo de la linea 12
    SYSCFG->EXTICR[3] |=  SYSCFG_EXTICR4_EXTI12_PB;    // Linea 12 escucha al puerto B
    EXTI->FTSR |= EXTI_RTSR_TR12;                      // Flanco de BAJADA
    EXTI->IMR  |= EXTI_IMR_IM12;                       // Desenmascarar la linea 12

    NVIC_EnableIRQ(EXTI9_5_IRQn);    // Lineas 5 a 9 comparten esta interrupcion
    NVIC_EnableIRQ(EXTI15_10_IRQn);  // Lineas 10 a 15 comparten esta interrupcion
}

void EXTI9_5_IRQHandler(void){
    // Comprobar que fue la linea 9 (sensor 1) y no otra de la 5 a la 9
    if (EXTI->PR & EXTI_PR_PR9){
        EXTI->PR = EXTI_PR_PR9;   // Bajar la bandera escribiendo 1
        Csensor1 = 1;             // Aviso: sumar
    }
}

void EXTI15_10_IRQHandler(void){
    // Comprobar que fue la linea 12 (sensor 2) y no otra de la 10 a la 15
    if (EXTI->PR & EXTI_PR_PR12){
        EXTI->PR = EXTI_PR_PR12;  // Bajar la bandera escribiendo 1
        Csensor2 = 1;             // Aviso: restar
    }
}
 
// Definiendo que hace la funcion cero en el display
void init_CERO(void){
    GPIOA->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR &= ~(GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR &= ~(GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR &= ~(GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR |= (GPIO_ODR_OD15); // Encendiendo el led g
 
}
 
// Definiendo que hace la funcion uno en el display
void init_UNO(void){
    GPIOA->ODR |= (GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR &= ~(GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR |= (GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR |= (GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR |= (GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR |= (GPIO_ODR_OD15); // Encendiendo el led g
 
}
 
// Definiendo que hace la funcion dos en el display
void init_DOS(void){
    GPIOA->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR &= ~(GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR |= (GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR &= ~(GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR |= (GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR &= ~(GPIO_ODR_OD15); // Encendiendo el led g
 
}
 
// Definiendo que hace la funcion tres en el display
void init_TRES(void){
    GPIOA->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR &= ~(GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR &= ~(GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR |= (GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR |= (GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR &= ~(GPIO_ODR_OD15); // Encendiendo el led g
 
}
 
// Definiendo que hace la funcion cuatro en el display
void init_CUATRO(void){
    GPIOA->ODR |= (GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR &= ~(GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR |= (GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR |= (GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR &= ~(GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR &= ~(GPIO_ODR_OD15); // Encendiendo el led g
 
}
 
// Definiendo que hace la funcion cinco en el display
void init_CINCO(void){
    GPIOA->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR |= (GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR &= ~(GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR |= (GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR &= ~(GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR &= ~(GPIO_ODR_OD15); // Encendiendo el led g
 
}
 
// Definiendo que hace la funcion seis en el display
void init_SEIS(void){
    GPIOA->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR |= (GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR &= ~(GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR &= ~(GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR &= ~(GPIO_ODR_OD15); // Encendiendo el led g
 
}
 
// Definiendo que hace la funcion siete en el display
void init_SIETE(void){
    GPIOA->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR &= ~(GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR |= (GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR |= (GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR |= (GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR |= (GPIO_ODR_OD15); // Encendiendo el led g
 
}
 
// Definiendo que hace la funcion ocho en el display
void init_OCHO(void){
    GPIOA->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR &= ~(GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR &= ~(GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR &= ~(GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR &= ~(GPIO_ODR_OD15); // Encendiendo el led g
}
 
// Definiendo que hace la funcion nueve en el display
void init_NUEVE(void){
    GPIOA->ODR &= ~(GPIO_ODR_OD4); // Encendiendo el led a
    GPIOB->ODR &= ~(GPIO_ODR_OD0); // Encendiendo el led b
    GPIOB->ODR &= ~(GPIO_ODR_OD14); // Encendiendo el led c
    GPIOB->ODR &= ~(GPIO_ODR_OD10); // Encendiendo el led d
    GPIOC->ODR |= (GPIO_ODR_OD4); // Encendiendo el led e
    GPIOB->ODR &= ~(GPIO_ODR_OD5); // Encendiendo el led f
    GPIOB->ODR &= ~(GPIO_ODR_OD15); // Encendiendo el led g
 
}

// Definiendo la funcion dibujar para saber cual se esta prendiendo en cada digito
void DIBUJAR(uint8_t valor){
    switch (valor) {

        //casos de numeros
        case 0:
            init_CERO(); // Dibuja un 0
            break;

        case 1:
            init_UNO(); // Dibuja un 1
            break;

        case 2:
            init_DOS(); // Dibuja un 2
            break;

        case 3:
            init_TRES(); // Dibuja un 3
            break;

        case 4:
            init_CUATRO(); // Dibuja un 4
            break;

        case 5:
            init_CINCO(); // Dibuja un 5
            break;

        case 6:
            init_SEIS(); // Dibuja un 6
            break;

        case 7:
            init_SIETE(); // Dibuja un 7
            break;

        case 8:
            init_OCHO(); // Dibuja un 8
            break;

        case 9:
            init_NUEVE(); // Dibuja un 9
            break;
    
    }
}