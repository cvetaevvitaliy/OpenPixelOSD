/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32g4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32g4xx_it.h"

extern PCD_HandleTypeDef hpcd_USB_FS;

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/

void NMI_Handler(void)
{
   while (1)
  {
  }

}

void HardFault_Handler(void) __attribute__((naked));
void HardFault_Handler(void) {
    __asm volatile (
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "b HardFault_Handler_C\n"
    );
}

void HardFault_Handler_C(uint32_t *stacked_regs) {
    //volatile uint32_t r0  = stacked_regs[0];
    //volatile uint32_t r1  = stacked_regs[1];
    //volatile uint32_t r2  = stacked_regs[2];
    //volatile uint32_t r3  = stacked_regs[3];
    //volatile uint32_t r12 = stacked_regs[4];
    volatile uint32_t lr  = stacked_regs[5];
    volatile uint32_t pc  = stacked_regs[6];
    //volatile uint32_t psr = stacked_regs[7];

    volatile uint32_t cfsr  = SCB->CFSR;
    //volatile uint32_t hfsr  = SCB->HFSR;
    //volatile uint32_t mmfar = SCB->MMFAR;
    //volatile uint32_t bfar  = SCB->BFAR;

    UNUSED(lr);
    UNUSED(pc);
    UNUSED(cfsr);

    TRACE_FATAL("HardFault!\nPC = 0x%08lX\nLR = 0x%08lX\nCFSR = 0x%08lX\n", pc, lr, cfsr);

    while (1); 
}


void MemManage_Handler(void)
{

  while (1)
  {

  }
}

void BusFault_Handler(void)
{

  while (1)
  {

  }
}

void UsageFault_Handler(void)
{

  while (1)
  {

  }
}

void SVC_Handler(void)
{

}

void DebugMon_Handler(void)
{

}

void PendSV_Handler(void)
{

}


void SysTick_Handler(void)
{
    HAL_IncTick();
}

/******************************************************************************/
/* STM32G4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g4xx.s).                    */
/******************************************************************************/

void USB_LP_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

