/******************************************************************************

                                Discrete.cpp

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


/** \file
'Discrete.cpp' holds the code for implementing discrete, i.e. z-transform, filters.
*/

#include <cmath>
#include "math.h"
#include "Calcs.h"
#include <iostream>
#include <fstream>
#include <complex>
#include <stdlib.h>
#include "Discrete.h"


iir iir1;


/// 'iir::step(...)' implements the Direct Form 1 realisation.
double iir::step(double in)
{
	int i;
	
	for(i=2; i>0; i--) x[i] = x[i-1], y[i] = y[i-1];
	x[0] = in * k;
	y[0] = -a1*y[1] - a2*y[2] + b0*x[0] + b1*x[1] + b2*x[2];

	return(y[0]);
}




/** Set the iir filter objects coefficients. */
void iir::setup(double kk, double aa0, double aa1, double aa2, double bb1, double bb2)
{
	k = kk;
	a1 = aa1, a2 = aa2;
	b1 = bb1, b2 = bb2;
}



void iir::write(int stage)
{
	char str[256];
	std::ofstream out;
	if(stage == 0) {
		out.open("./out.c", std::ofstream::trunc);
		/// Write initial code to 'out.c'
	}
	else out.open("./out.c", std::ofstream::app);
	sprintf(str, "y%1d = -a%1d1*y%1d[1] - a%1d2*y%1d[2] + b%1d0*x%1d[0] + b%1d1*x%1d[1] + b%1d2*x%1d[2];\n",
	 stage, stage, stage, stage, stage, stage, stage, stage, stage, stage, stage);
	out << str;
	if(out.is_open( )) { out.close( ); }
}



/** Pre-warp the continuous filter. */
void prewarp(void)
{
	/// Not yet designed.
}



/** Transform the continuous filter into a discrete one. */
void iir::transform(filter_class fclass, double Omega_ac, double q)
{
	double ro2 = 1.0 / (Omega_ac * Omega_ac);
	double roq = 1.0 / (Omega_ac * q);
	b0 = 1.0;
	switch(fclass) {
		case Lowpass:
		b1 = 2.0;
		b2 = 1.0;
		break;
		case Highpass:
		b0 = ro2;
		b1 = -2.0 * ro2;
		b2 = ro2;
		break;
		case Bandpass:
		b0 = 1.0;
		b1 = 0.0;
		b2 = -1.0;
		break;
		default: cout << "\nUnknown filter type.\n"; return;
	}
	a0 = ro2 + roq + 1.0;
	std::cout << "a0 =" << a0 << "\n";
//	k = 1.0 / a0;
	a1 = 2.0 - 2.0 * ro2;
	a2 = ro2 - roq + 1.0;

	a1 /= a0;
	a2 /= a0;
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	std::cout << "(" << b0 << "+(z^-1)*" << b1 << "+(z^-2)*" << b2 << ")/(" << 1.0 << "+(z^-1)*" << a1 << "+(z^-2)*" << a2 << ")\n";
	std::cout << "-->" << (b0 + b1 + b2) / (1.0 + a1 + a2) << "\n";

	std::cout << "[a0 = 1.0, a1 =" << a1 << ", a2 =" << a2 << "; b0 =" << b0 << ", b1 =" << b1 << ", b2 =" << b2 << "]\n";

/*	std::cout << "Direct Form 2 realisation.\n--------------------------\n";
	std::cout << "y[n] = b0 * w[n] + b1 * w[n-1] + b2 * w[n-2], w[n] = x[n] - a1 * w[n-1] - a2 * w[n-2]";
*/
}



/** 'normalise(...)' sets 'omega' */
double normalise(double fc, double fs)
{
	double omega = 2.0 * M_PI * fc / fs;
	return(omega);
}


#if false
int main( )
{
	int i;
	double fc, fs, omega;
	std::cout << "cut-off freq: ";
	std::cin >> fc;
	std::cout << "sampling freq: ";
	std::cin >> fs;
	if(fc >= fs/2.0) {
		std::cout << "That doesn't make sense! Cut-off must be less than sampling freq / 2.\n";
		exit(-1);
	}
	omega = normalise(fc, fs);
	std::cout << "omega =" << omega << "\n";
	double omega_ac = tan(omega/2.0);
	std::cout << "omega_ac =" << omega_ac << "\n\n";

	iir1.transform(omega_ac, 0.7);
//	iir1.setup(1.384, 0.1311136, 0.2162924, 0.1311136, -0.829328, 0.307046);
	iir1.step(0.0);
	std::cout << "\n";
	std::cout << "Direct Form 1 realisation.\n--------------------------\n";
	for(i=0; i<200; i++) std::cout << iir1.step(1.0) << "\n";
	return(0);
}
#endif




