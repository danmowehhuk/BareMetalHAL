#include "UartHAL.h"

// Instance 2 only ever exists on multi-UART chips (already gated by
// UartHAL.h's own #ifdef UDR2), and every such chip (avr/iomxx0_1.h)
// names its RX vector USART2_RX_vect - no single-UART-chip fallback is
// needed here the way instance 0 needs one.
#ifdef UDR2
#include "UartHAL_rx_impl.h"
#define BAREMETALHAL_UART2_RX_VECT USART2_RX_vect
BAREMETALHAL_DEFINE_UART_RX_IMPL(2)
#undef BAREMETALHAL_UART2_RX_VECT
#endif
