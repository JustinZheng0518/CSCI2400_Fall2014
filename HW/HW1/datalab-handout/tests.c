/* Testing Code */

#include <limits.h>
//
// 15 problems, 40 points
// 2 - rating 1
// 3 - rating 2
// 7 - rating 3
// 3 - rating 4

// rating 1
int test_tmin(void) {
  return LONG_MIN;
}
int test_minusOne(void) {
  return -1;
}
// rating 2
int test_evenBits(void) {
  int result = 0;
  int i;
  for (i = 0; i < 32; i+=2)
    result |= 1<<i;
  return result;
}
int test_divpwr2(int x, int n)
{
    int p2n = 1<<n;
    return x/p2n;
    
}
int test_isEqual(int x, int y)
{
  return x == y; 
}
int test_negate(int x) {
  return -x;
}
int test_getByte(int x, int n)
{
  union  {
    int word;
    unsigned char bytes[4];
  } u;
  int test = 1;
  int littleEndian = (int) *(char *) &test;
  u.word = x;
  return littleEndian ? (unsigned) u.bytes[n] : (unsigned) u.bytes[3-n];
}
// rating 3
int test_isPositive(int x) {
  return x > 0;
}

int test_addOK(int x, int y)
{
  int sum = x+y;
  return !(x < 0 && y < 0 && sum >= 0) && !(x > 0 && y > 0 && sum <= 0);
}
int test_isLessOrEqual(int x, int y)
{
  return x <= y;
}
int test_logicalShift(int x, int n) {
  unsigned u = (unsigned) x;
  unsigned shifted = u >> n;
  return (int) shifted;
}
// rating 4
int test_leastBitPos(int x) {
  int mask = 1;

  if (x == 0)
    return 0;
  while (!(mask & x)) {
    mask = mask << 1;
  }
  return mask;
}
int test_bitParity(int x) {
  int result = 0;
  int i;
  for (i = 0; i < 32; i++)
    result ^=  (x >> i) & 0x1;
  return result;
}
int test_isPower2(int x) {
  int i;
  for (i = 0; i < 31; i++) {
    if (x == 1<<i)
      return 1;
  }
  return 0;
}
int test_bang(int x)
{
  return !x;
}
