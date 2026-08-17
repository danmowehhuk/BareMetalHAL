// Bare-metal AVR example proving BareMetalHAL's global operator
// new/delete actually link and actually work, not just compile - see
// Task 2 of .claudework/plans/2026-08-17-dynamic-memory-hal.md for the
// full rationale. The motivating problem this whole category exists to
// fix was a real failed link (undefined reference to
// 'operator new(...)') - proving a real link succeeds is the point of
// this example, same as gpio-basic-avr/timing-basic-avr before it.
//
// Expected serial output (9600 baud), once at boot then repeating
// every ~2s:
//   free before: <N>
//   sum: 45
//   free after: <M>
// "sum: 45" proves the allocated array is real, writable memory (0+1+
// ...+9 = 45), not just a non-null pointer. Whether <M> equals <N> is
// what Task 3 observes and reports - don't assume it matches without
// checking; this exercises avr-libc's own malloc/free, not code this
// category wrote.

#include <util/delay.h>
#include "BareMetalHAL.h"

using namespace BareMetalHAL;

int main() {
  Uart0::begin(9600);

  while (true) {
    int before = freeMemory();
    Uart0::print("free before: ");
    Uart0::println(before);

    int* arr = new int[10];
    for (int i = 0; i < 10; i++) {
      arr[i] = i;
    }
    int sum = 0;
    for (int i = 0; i < 10; i++) {
      sum += arr[i];
    }
    Uart0::print("sum: ");
    Uart0::println(sum);

    delete[] arr;

    int after = freeMemory();
    Uart0::print("free after: ");
    Uart0::println(after);

    _delay_ms(2000);
  }
}
