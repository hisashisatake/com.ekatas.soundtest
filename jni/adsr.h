/*
 * adsr.h
 *
 *  Created on: 2013/03/05
 *      Author: satake
 */

#ifndef ADSR_H_
#define ADSR_H_

#include <math.h>

void ADSR(double e[], int A, int D, double S, int R, int gate, int duration)
{
  int n;

  if (A != 0)
  {
    for (n = 0; n < A; n++)
    {
      e[n] = 1.0 - exp(-5.0 * n / A);
    }
  }

  if (D != 0)
  {
    for (n = A; n < gate; n++)
    {
      e[n] = S + (1 - S) * exp(-5.0 * (n - A) / D);
    }
  }
  else
  {
    for (n = A; n < gate; n++)
    {
      e[n] = S;
    }
  }

  if (R != 0)
  {
    for (n = gate; n < duration; n++)
    {
      e[n] = e[gate - 1] * exp(-5.0 * (n - gate + 1) / R);
    }
  }
}

#endif /* ADSR_H_ */
