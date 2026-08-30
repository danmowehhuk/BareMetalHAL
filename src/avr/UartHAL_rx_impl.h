#ifndef BAREMETALHAL_AVR_UARTHAL_RX_IMPL_H
#define BAREMETALHAL_AVR_UARTHAL_RX_IMPL_H

// Implementation detail, not part of the public API - see UartHAL.h's own
// "detail::" namespace for the same convention. Included only by this
// library's own UartHAL_rx<N>.cpp files, one invocation per instance.
//
// Split into one translation unit per UART instance (UartHAL_rx0.cpp
// .. UartHAL_rx3.cpp) rather than one shared UartHAL.cpp, specifically so
// static-archive linking can pull in exactly the instances a consumer
// actually references. Archive linking resolves per-.o-file, not per
// symbol within a .o file - so as long as each instance's ring buffer,
// enableRx(), and ISR live in their own .o, a consumer that only ever
// names (e.g.) Uart2's symbols never drags in Uart0/Uart1/Uart3's
// storage or ISRs at all.
//
// enableRx() is a real (non-inline) function, declared in UartHAL.h and
// defined here. That's the crux of the fix: an inline-only design (this
// library's previous approach) never emits an unresolved reference into
// any object file, so the linker has nothing forcing it to pull an
// archive member in - the ISR silently isn't linked, RXCIE0 still gets
// set some other way, and the interrupt vectors to the weak
// __bad_interrupt default handler forever. A real function call is a
// strong undefined-symbol reference: the linker must resolve it, which
// forces this whole translation unit (ring buffer + ISR included) out of
// the archive. A consumer who never calls enableRx() (nor
// available()/read(), which reference the same rxDetail externs) has no
// reference into this file at all, so nothing about RX is ever linked in
// for them - RXCIE0 is never set for their instance, and their behavior
// is byte-for-byte identical to a build of this library from before RX
// existed.
#define BAREMETALHAL_DEFINE_UART_RX_IMPL(N) \
namespace BareMetalHAL { \
namespace Uart##N { \
namespace rxDetail { \
  volatile uint8_t buffer[64]; \
  volatile uint8_t head = 0; \
  volatile uint8_t tail = 0; \
} \
void enableRx() { \
  UCSR##N##B |= (1 << RXCIE0); \
  rxDetail::head = 0; \
  rxDetail::tail = 0; \
  sei(); \
} \
} \
} \
ISR(BAREMETALHAL_UART##N##_RX_VECT) { \
  uint8_t b = UDR##N; \
  uint8_t next = (BareMetalHAL::Uart##N::rxDetail::tail + 1) & 63; \
  if (next != BareMetalHAL::Uart##N::rxDetail::head) { \
    BareMetalHAL::Uart##N::rxDetail::buffer[BareMetalHAL::Uart##N::rxDetail::tail] = b; \
    BareMetalHAL::Uart##N::rxDetail::tail = next; \
  } \
  /* else: buffer full, drop the incoming byte, keep existing data */ \
}

#endif
