// Proves the fix for the original bug: a translation unit that uses
// new/delete[] WITHOUT including BareMetalHAL.h at all (exactly
// Eventuino.cpp's real situation - it only includes its own
// EventuinoHal.h, which declares functions but never pulls in
// BareMetalHAL.h) must still link successfully against the global
// operator new/delete defined in MemoryHAL.cpp. This is the actual
// regression this whole category exists to prevent - a single-TU
// example that includes BareMetalHAL.h itself would never have caught
// the inline-emits-nothing-at -Os bug the final review found.

int* allocate_without_including_the_hal(int count) {
  int* arr = new int[count];
  for (int i = 0; i < count; i++) {
    arr[i] = i;
  }
  return arr;
}

void free_without_including_the_hal(int* arr) {
  delete[] arr;
}
