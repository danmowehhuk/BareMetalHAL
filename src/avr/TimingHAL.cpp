#include "TimingHAL.h"

namespace BareMetalHAL {
namespace detail {

volatile uint32_t millisCounter = 0;

}  // namespace detail
}  // namespace BareMetalHAL

ISR(TIMER0_COMPA_vect) {
  BareMetalHAL::detail::millisCounter++;
}
