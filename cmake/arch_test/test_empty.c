/*
 * Used to test support for certain assembly instruction types.
 */

extern int test_avx() asm("_test_avx");

int main() {
  if (test_avx() != 0) {
    return -1;
  }

  return 0;
}
