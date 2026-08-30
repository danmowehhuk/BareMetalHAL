#include "UartHAL.h"
#include "UartHAL_rx_impl.h"

// Instance 0 exists on every AVR chip this library targets, but the RX
// vector's name is NOT consistent across them - verified against the
// real per-chip headers, not assumed:
// - Multi-UART chips (avr/iomxx0_1.h, e.g. the ATmega2560) name it
//   USART0_RX_vect, matching instances 1-3's own USART<N>_RX_vect
//   convention.
// - Single-UART chips (avr/iom328p.h, e.g. the ATmega328P) only have one
//   UART and don't number it - it's USART_RX_vect, no "0". Hardcoding
//   USART0_RX_vect would silently fail to match any real vector on this
//   chip family: ISR() would still compile (only a -Wmisspelled-isr
//   warning, not an error), but the RX_vect it emits doesn't correspond
//   to any real interrupt, so incoming RX bytes would vector to the weak
//   __bad_interrupt default handler - the same reset-loop hazard this
//   whole enableRx() design exists to avoid.
#if defined(USART0_RX_vect)
#define BAREMETALHAL_UART0_RX_VECT USART0_RX_vect
#elif defined(USART_RX_vect)
#define BAREMETALHAL_UART0_RX_VECT USART_RX_vect
#else
#error "No USART0/USART RX vector found for this MCU - BareMetalHAL UartHAL RX needs this defined"
#endif

BAREMETALHAL_DEFINE_UART_RX_IMPL(0)

#undef BAREMETALHAL_UART0_RX_VECT
