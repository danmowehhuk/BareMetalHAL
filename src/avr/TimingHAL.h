#ifndef BAREMETALHAL_AVR_TIMINGHAL_H
#define BAREMETALHAL_AVR_TIMINGHAL_H

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Timer0-backed millisecond counter for HAL_AVR. Claims Timer0
// exclusively - a HAL_AVR consumer wanting hardware PWM on pins 4/13
// (TIMER0B/TIMER0A on the ATmega2560 - verified against the real
// pins_arduino.h, not assumed from the Uno's different pin layout)
// cannot use both. No current consumer uses PWM, so this is a
// documented limitation, not a live conflict.
//
// Arduino's own core uses ISR(TIMER0_OVF_vect) for its millis() -
// verified against the real wiring.c, not assumed. This design uses
// ISR(TIMER0_COMPA_vect) (CTC mode, Compare Match A) instead - a
// different vector, incidental rather than a safety feature being
// relied upon: BareMetalHAL.h's own #error guard (fires if included
// without NO_ARDUINO defined) already fully prevents an Arduino-branch
// build from ever reaching a state where this file could be linked
// alongside Arduino's own Timer0 usage.

namespace BareMetalHAL {

namespace detail {

// Storage for the millisecond counter lives in TimingHAL.cpp - the ISR
// that increments it must have external linkage and exactly one
// definition, which an inline header function can't provide (unlike
// the rest of this HAL, which is header-only). Namespaced (BareMetalHAL::
// detail, matching GpioHAL.h/UartHAL.h's convention for internals not
// part of the public contract) rather than a leading-underscore global -
// a leading underscore at global-namespace scope is reserved to the
// implementation, and nothing about the ISR() macro (which itself always
// expands to a global-scope function, regardless of the namespace of
// the variable it touches) forces this counter out of a namespace.
extern volatile uint32_t millisCounter;

}  // namespace detail

// Configures Timer0 for a 1ms tick and enables global interrupts. Must
// be called once, explicitly, before millis() is used anywhere - no
// HAL category in this library auto-initializes hardware (UartHAL's
// begin() is caller-owned for the same reason).
//
// Templated on Cpu (defaulting to F_CPU) rather than a plain function
// so the OCR0A derivation - and its static_asserts - only get
// instantiated, and only fire, when a consumer actually calls
// timingInit(). If this were file-scope or a plain (non-template)
// function body, the static_asserts would fire the instant
// <BareMetalHAL.h> is included at all, even by a consumer that only
// wants UartHAL/GpioHAL and never touches Timing - breaking the
// umbrella header for every consumer on any F_CPU this formula
// doesn't support, not just for Timing's own users. Don't "simplify"
// this back to a plain function.
template <uint32_t Cpu = F_CPU>
inline void timingInit() {
  // F_CPU must divide evenly by 64000 (prescaler 64 x desired 1000Hz
  // tick) for an exact 1ms tick with no drift - a different F_CPU
  // needs its own prescaler/OCR0A derivation, not this formula assumed
  // blindly.
  static_assert(Cpu % 64000UL == 0,
                "F_CPU not evenly divisible by 64000 - this prescaler/tick-rate combination doesn't produce an exact 1ms tick for this F_CPU");

  // uint16_t intermediate deliberately, not uint8_t - the static_assert
  // below must catch an oversized computed value before it can silently
  // truncate. Computing directly into a uint8_t would let e.g. a raw
  // value of 300 wrap to 44 before any check could catch it.
  constexpr uint16_t ocr0aRaw = (Cpu / 64000UL) - 1;

  static_assert(ocr0aRaw <= 255,
                "Computed OCR0A exceeds uint8_t range for this F_CPU at /64 prescale - needs a larger prescaler, this formula doesn't generalize past that point");

  constexpr uint8_t ocr0aFor1ms = (uint8_t)ocr0aRaw;

  TCCR0A = (1 << WGM01);               // CTC mode
  TCCR0B = (1 << CS01) | (1 << CS00);  // prescaler /64
  OCR0A = ocr0aFor1ms;
  TIMSK0 = (1 << OCIE0A);
  sei();
}

inline uint32_t millis() {
  // 32-bit read of a value the ISR writes non-atomically - read with
  // interrupts disabled to avoid a torn read on this 8-bit core. Same
  // idiom as GpioHAL's pinMode/digitalWrite critical sections.
  uint8_t oldSREG = SREG;
  cli();
  uint32_t m = detail::millisCounter;
  SREG = oldSREG;
  return m;
}

// Busy-waits until at least `ms` milliseconds have elapsed, matching
// Arduino's own delay() semantics. Built entirely on top of millis() -
// no new register access, no new hardware dependency - so it needs
// nothing beyond what timingInit() already provides. Same caller-owned
// precondition as millis(): timingInit() must already have been called.
inline void delay(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) { }
}

// Microsecond-granularity busy-wait. Cycle-exact when `us` is a
// compile-time constant at the call site - GCC constant-folds the
// tick computation through this inlined wrapper into avr-libc's own
// _delay_us(). Requires optimizations enabled (any -O level except
// -O0): avr-libc's own fast path is gated on the compiler-defined
// __OPTIMIZE__ macro, which only -O0 leaves unset. Every build in
// this project already builds with optimizations on.
inline void delayMicroseconds(uint16_t us) {
  _delay_us(us);
}

}  // namespace BareMetalHAL

#endif
