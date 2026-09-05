/* Includes ------------------------------------------------------------------*/
//#include <stm32f103xb.h>
#include <stdio.h>
#include "stm32g4xx_hal.h"

#ifdef USE_SWO
//signed int fputs(const char *pStr, FILE *pStream)
int _write(int file, char *ptr, int len)
{
    (void)file;
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++)
    {
      ITM_SendChar(*ptr++);
    }
    return len;
}
#endif


uint32_t _ITMPort = 0; // The stimulus port from which SWO data is received and displayed.


void SWO_Init() {
  
uint32_t SWOPrescaler = 28 ; // baudrate in Hz, note that cpuCoreFreqHz is expected to match the CPU core clock
 
	CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk; 		// Debug Exception and Monitor Control Register (DEMCR): enable trace in core debug
	DBGMCU->CR	= 0x00000027u ;							// DBGMCU_CR : TRACE_IOEN DBG_STANDBY DBG_STOP 	DBG_SLEEP
	TPI->SPPR	= 0x00000002u ;							// Selected PIN Protocol Register: Select which protocol to use for trace output (2: SWO)
	TPI->ACPR	= SWOPrescaler ;						// Async Clock Prescaler Register: Scale the baud rate of the asynchronous output
	ITM->LAR	= 0xC5ACCE55u ;							// ITM Lock Access Register: C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC
	ITM->TCR	= 0x0001000Du ;							// ITM Trace Control Register
	ITM->TPR	= ITM_TPR_PRIVMASK_Msk ;				// ITM Trace Privilege Register: All stimulus ports
	ITM->TER	= 0x01;							// ITM Trace Enable Register: Enabled tracing on stimulus ports. One bit per stimulus port.
	DWT->CTRL	= 0x400003FEu ;							// Data Watchpoint and Trace Register
	TPI->FFCR	= 0x00000100u ;

}

