#include "UartHAL.h"

// Instance 1 only ever exists on multi-UART chips (already gated by
// UartHAL.h's own #ifdef UDR1), and every such chip (avr/iomxx0_1.h)
// names its RX vector USART1_RX_vect - no single-UART-chip fallback is
// needed here the way instance 0 needs one.
#ifdef UDR1
#include "UartHAL_rx_impl.h"
#define BAREMETALHAL_UART1_RX_VECT USART1_RX_vect
BAREMETALHAL_DEFINE_UART_RX_IMPL(1)
#undef BAREMETALHAL_UART1_RX_VECT
#endif
