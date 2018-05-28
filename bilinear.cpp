/******************************************************************************

                                Bilinear.cpp

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


#include "math.h"
#include <complex.h>
#include "Calcs.h"
#include <ostream>
#include <iostream>
#include <string>



std::complex<double> _bilinear(std::complex<double> s, double Ts)
{
	std::complex<double> z = (1.0 + s*Ts/2.0) / (1.0 - s*Ts/2.0);
	return(z);
}



