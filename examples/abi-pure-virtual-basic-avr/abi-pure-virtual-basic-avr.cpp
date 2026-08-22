// examples/abi-pure-virtual-basic-avr/abi-pure-virtual-basic-avr.cpp
//
// Proves __cxa_pure_virtual actually resolves at link time AND that
// virtual dispatch through a base-class pointer to an abstract class
// works correctly at runtime (not just "it linked") - a base class with
// a pure virtual method, a concrete override, called through a Base*.

#include "BareMetalHAL.h"

using namespace BareMetalHAL;

class Base {
  public:
    virtual ~Base();   // declaration only - out-of-line definition below
                        // is Base's key function, which is what forces
                        // the compiler to actually emit Base's vtable
                        // (and thus a reference to __cxa_pure_virtual)
    virtual int value() const = 0;
};

class Derived : public Base {
  public:
    int value() const override { return 42; }
};

Base::~Base() {}

int main() {
  Uart0::begin(9600);

  Derived d;
  Base* b = &d;

  Uart0::print("value=");
  Uart0::print((int)b->value());
  Uart0::println("");

  while (true) {}
}
