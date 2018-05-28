/******************************************************************************

                                Discrete.h

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


#ifndef DISCRETE_H
#define DISCRETE_H

#include "Enums.h"


class iir {
	int i;
	double a0, a1, a2, b0, b1, b2, k;
	double y[3], x[3];
public:
	iir(void) { k = 1.0; for(i=0; i<3; i++) x[i] = y[i] = 0.0; }
	void setup(double kk, double aa0, double aa1, double aa2, double bb1, double bb2);
	void transform(filter_class fclass, double T, double q);
	double step(double in);
	void write(int stage);
};


#endif

