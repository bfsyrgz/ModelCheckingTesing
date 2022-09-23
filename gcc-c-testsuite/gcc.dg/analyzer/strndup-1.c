/* { dg-skip-if "no strndup in libc" { *-*-darwin[789]* *-*-darwin10* *-*-mingw* *-*-vxworks* } } */

#include <string.h>
#include <stdlib.h>

extern void requires_nonnull (void *ptr)
  __attribute__((nonnull));

void test_1 (const char *s)
{
  char *p = strndup (s, 42); /* { dg-message "allocated here" } */
} /* { dg-warning "leak of 'p'" } */

void test_2 (const char *s)
{
  char *p = strndup (s, 42);
  free (p);
}
void test_3 (const char *s)
{
  char *p = strndup (s, 42); /* { dg-message "this call could return NULL" } */
  requires_nonnull (p); /* { dg-warning "use of possibly-NULL 'p'" } */
}
