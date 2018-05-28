/******************************************************************************

                                filter.h

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


#ifndef FILTER_H
#define FILTER_H


#include <complex.h>
#include "calcs.h"

#define SHOW_FILE_COMPILING	0

class TFilter {
public:
	complex pole[40];
	void log(string);
	void log(string, double);
	void log(string, complex);
	int poles;
	char type;
	int zeroes;
	double fmax, tmax;
	double step_resp[IMAX];
};


#endif


