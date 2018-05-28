/******************************************************************************

                                Calcs.h

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


#ifndef CALCSH
#define CALCSH

/*! \file Calcs.h
*/

#include <string>
using std::string;
#include <complex>
#include <stdio.h>
#include <math.h>
#include <fstream>
#include <sstream>
#include "Enums.h"
#include "Discrete.h"

#define FSTEPS 1000
#define TSTEPS 1000
#define IMAX 19
#define MAXS 19

#define LOG true

#define NORMALISE

#define VERBOSE	false

// Shall we show detailed response data?
// #define SHOW_STAGES

using namespace std;




/*! \class stage
Describes one stage of a cascaded continuous filter.
*/
class stage {
public:
	filter_class fclass;
	circuit_class cclass;
	double T, q;
	std::complex<double> pole, zero;
//	double t, q;
	double R1, R2, R3, C1, C2, C3;
	double gain;
	std::complex<double> z;
	iir iir1;
	void R_low_pass(void);
	void R_high_pass(void);
	void R_band_pass(void);
	void synthesise_R_low(void);
	void synthesise_R_band(void);
	void synthesise_R_high(void);
	void synthesise_SK_low(void);
	void synthesise_SK_band(void);
	void synthesise_SK_high(void);
	void bilinear(void);
	stage& operator= (stage& f1);
};


class TFilter {
// private:
public:
	filter_class fclass;
	circuit_class cclass;
	shape_class sclass;
	double frequency;
	double gain;
    unsigned int poles, zeroes;
//	std::complex<double> pole[20], zero[20];
    double /* t[10], q[10],*/ tau;		// TODO: use tau for single pole.
    char type;
    double fmax, tmax;
	double ripple;
	double bandwidth;
	double samplingfreq;
    double freq_resp[FSTEPS];
    double step_resp[TSTEPS];
	stage st[10];
	void Proto_normalise(void);
	double fgain(double f);
	double a0;
public:
    TFilter( );
//    ~TFilter( );
	void Calculate(void);
    void log(string);
    void log(string, double);
    void log(string, double *);
    void log(string, double *, int);
    void log(string, std::complex<double>);
	void _pole(double a, double w);
	void transform(void);
    void print_T_q(void);
	void ts_and_qs(void);
	void show_filter(void);
	void show_filter(string title);
	void bessel(void);
	void sort(void);
	void butterworth(void);
	void chebyshev(void);
	void lowpass(void);
	void highpass(void);
	void bandpass(void);
	void Synth_Rauch(void);
	void Synth_SallKey(void);
	void step_calc(void);
	void bilinear(void);
};


double tq(std::complex<double> p, double &t, double &q);
void bode_calc(TFilter& filter);
void step_calc(TFilter& filter);


#endif
