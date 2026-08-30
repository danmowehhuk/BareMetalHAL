#include "UartHAL.h"

// Instance 3 only ever exists on multi-UART chips (already gated by
// UartHAL.h's own #ifdef UDR3), and every such chip (avr/iomxx0_1.h)
// names its RX vector USART3_RX_vect - no single-UART-chip fallback is
// needed here the way instance 0 needs one.
#ifdef UDR3
#include "UartHAL_rx_impl.h"
#define BAREMETALHAL_UART3_RX_VECT USART3_RX_vect
BAREMETALHAL_DEFINE_UART_RX_IMPL(3)
#undef BAREMETALHAL_UART3_RX_VECT
#endif
