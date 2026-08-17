// Bare-metal AVR example proving BareMetalHAL's global operator
// new/delete actually link and actually work, not just compile. The
// motivating problem this whole category exists to fix was a real
// failed link (undefined reference to 'operator new(...)') - proving a
// real link succeeds is the point of this example, same as
// gpio-basic-avr/timing-basic-avr before it.
//
// Expected serial output (9600 baud), once at boot then repeating
// every ~2s:
//   free before: <N>
//   sum: 45
//   cross-TU sum: 10
//   free after: <M>
// "sum: 45" proves the allocated array is real, writable memory (0+1+
// ...+9 = 45), not just a non-null pointer. Whether <M> equals <N> is
// something to observe and report - don't assume it matches without
// checking; this exercises avr-libc's own malloc/free, not code this
// category wrote. "cross-TU sum: 10" (0+1+2+3+4 = 10) is the same proof
// for allocate_without_including_the_hal()/free_without_including_the_hal()
// below, which live in a separate translation unit that never includes
// BareMetalHAL.h or MemoryHAL.h at all.

#include <util/delay.h>
#include "BareMetalHAL.h"

using namespace BareMetalHAL;

// Declared without including a header for these - deliberately, not an
// oversight. These live in consumer_no_include.cpp, a translation unit
// that never includes BareMetalHAL.h or MemoryHAL.h, mirroring
// Eventuino.cpp's real situation. Calling them from here proves the
// cross-TU link against MemoryHAL.cpp's global operator new/delete
// actually works end-to-end, not just that consumer_no_include.cpp
// compiles standalone.
int* allocate_without_including_the_hal(int count);
void free_without_including_the_hal(int* arr);

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

    int* crossTuArr = allocate_without_including_the_hal(5);
    int crossTuSum = 0;
    for (int i = 0; i < 5; i++) {
      crossTuSum += crossTuArr[i];
    }
    Uart0::print("cross-TU sum: ");
    Uart0::println(crossTuSum);
    free_without_including_the_hal(crossTuArr);

    int after = freeMemory();
    Uart0::print("free after: ");
    Uart0::println(after);

    _delay_ms(2000);
  }
}
