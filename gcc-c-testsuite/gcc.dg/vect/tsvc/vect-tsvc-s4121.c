/*  This file is distributed under the University of Illinois Open Source
    License. See license.txt for details.  */

/* { dg-additional-options "--param vect-epilogues-nomask=0" } */
/* { dg-require-effective-target vect_float } */

#include "tsvc.h"

real_t f(real_t a, real_t b){
    return a*b;
}

real_t s4121(struct args_t * func_args)
{
//    statement functions
//    elementwise multiplication

    initialise_arrays(__func__);

    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            a[i] += f(b[i],c[i]);
        }
        dummy(a, b, c, d, e, aa, bb, cc, 0.);
    }

    return calc_checksum(__func__);
}

int main (int argc, char **argv)
{
  int n1 = 1;
  int n3 = 1;
  int* ip;
  real_t s1,s2;
  init(&ip, &s1, &s2);

  run(&s4121, "s4121", NULL);

  return 0;
}


/* { dg-final { scan-tree-dump-times "vectorized 1 loops" 1 "vect" } } */