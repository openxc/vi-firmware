#ifndef __BSP_BLUEBOARDBASE__
#define __BSP_BLUEBOARDBASE__

#if defined(__LPC17XX__)
    #undef BOARD
    #define BOARD BOARD_LPCXpressoBase_RevB
	#include "../LPCXpressoBase_RevB/bsp_LPCXpressoBase_RevB.h"
    #undef USB_CONNECT_GPIO_PORT_NUM
    #define USB_CONNECT_GPIO_PORT_NUM 2
    #undef USB_CONNECT_GPIO_BIT_NUM
    #define USB_CONNECT_GPIO_BIT_NUM 9
#endif

#endif
