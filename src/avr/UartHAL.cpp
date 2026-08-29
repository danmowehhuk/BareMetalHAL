#include "UartHAL.h"

// Ring buffer storage and the RX-complete ISR need external linkage
// (the ISR must have exactly one definition, and this volatile state is
// read from both interrupt and mainline context) - same reason
// TimingHAL::detail::millisCounter lives in TimingHAL.cpp rather than
// TimingHAL.h. The ISR itself is defined at global scope, outside
// namespace BareMetalHAL, referencing the namespaced storage by its
// fully-qualified name - ISR() always expands to a global-scope
// function regardless of the namespace surrounding its invocation, and
// this mirrors TimingHAL.cpp's own ISR(TIMER0_COMPA_vect) exactly
// (global-scope ISR, fully-qualified reference into BareMetalHAL::
// detail, no using-namespace shortcut).
#define BAREMETALHAL_DEFINE_UART_RX(N) \
namespace BareMetalHAL { \
namespace Uart##N { \
namespace rxDetail { \
  volatile uint8_t buffer[64]; \
  volatile uint8_t head = 0; \
  volatile uint8_t tail = 0; \
} \
} \
} \
ISR(USART##N##_RX_vect) { \
  uint8_t b = UDR##N; \
  uint8_t next = (BareMetalHAL::Uart##N::rxDetail::tail + 1) & 63; \
  if (next != BareMetalHAL::Uart##N::rxDetail::head) { \
    BareMetalHAL::Uart##N::rxDetail::buffer[BareMetalHAL::Uart##N::rxDetail::tail] = b; \
    BareMetalHAL::Uart##N::rxDetail::tail = next; \
  } \
  /* else: buffer full, drop the incoming byte, keep existing data */ \
}

BAREMETALHAL_DEFINE_UART_RX(0)

#ifdef UDR1
BAREMETALHAL_DEFINE_UART_RX(1)
#endif

#ifdef UDR2
BAREMETALHAL_DEFINE_UART_RX(2)
#endif

#ifdef UDR3
BAREMETALHAL_DEFINE_UART_RX(3)
#endif

#undef BAREMETALHAL_DEFINE_UART_RX
