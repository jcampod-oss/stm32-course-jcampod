#include <stdint.h>
#include "stm32f411xe.h"
#include "stm32f4xx.h"

void init_hardware(void);


int main(void){
    
    init_hardware();

    while(1){

        if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk){
            GPIOB->ODR ^= (GPIO_ODR_OD8); // encendiendo el led
            GPIOB->ODR ^= (GPIO_ODR_OD9); // encendiendo el led
            GPIOB->ODR ^= (GPIO_ODR_OD6); // encendiendo el led
           // limpiando la bandera de interrupcion
        }

    }
}
// Verde
void init_hardware(void){
    //RCC->AHB1ENR &= ~(1<<0);
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN; // limpiando la posicion
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // activando la señal

    //configurando el pin 8
    GPIOB->MODER &= ~(GPIO_MODER_MODE8); // limpiando la posición
    GPIOB->MODER |= (GPIO_MODER_MODE8_0); // configurando como salida

    GPIOB->OTYPER &= ~GPIO_OTYPER_OT8; // configurando como push-pull

    GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED8); // LIMPIANDO LA POSICION
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED8_1); // configurando la velocidad

    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD8); // limpiando la posición

    GPIOB->ODR &= ~ (GPIO_ODR_OD8); // encendiendo el led
    GPIOB->ODR |= (GPIO_ODR_OD8); // apagando el led


//Amarillo


    //configurando el pin 9
    GPIOB->MODER &= ~(GPIO_MODER_MODE9); // limpiando la posición
    GPIOB->MODER |= (GPIO_MODER_MODE9_0); // configurando como salida

    GPIOB->OTYPER &= ~GPIO_OTYPER_OT9; // configurando como push-pull

    GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED9); // LIMPIANDO LA POSICION
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED9_1); // configurando la velocidad

    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD9); // limpiando la posición

    GPIOB->ODR &= ~ (GPIO_ODR_OD9); // encendiendo el led
    GPIOB->ODR |= (GPIO_ODR_OD9); // apagando el led

//Rojo

    //configurando el pin 6
    GPIOB->MODER &= ~(GPIO_MODER_MODE6); // limpiando la posición
    GPIOB->MODER |= (GPIO_MODER_MODE6_0); // configurando como salida

    GPIOB->OTYPER &= ~GPIO_OTYPER_OT6; // configurando como push-pull

    GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED6); // LIMPIANDO LA POSICION
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED6_1); // configurando la velocidad

    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD6); // limpiando la posición

    GPIOB->ODR &= ~ (GPIO_ODR_OD6); // apagando el led
    GPIOB->ODR |= (GPIO_ODR_OD6); // encendiendo el led

    SysTick->LOAD = 16000000 -1 ; // 1 segundo
    SysTick->VAL = 0; // limpiar el valor del contador
    SysTick->CTRL |= (SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk); // habilitar el systick    

}

