/******************************************************************************

                           Sallen_and_Key.cpp

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <iostream>
#include "Calcs.h"

#if SHOW_FILE_COMPILING
#pragma message "Compiling Sallen_and_Key.cpp"
#endif

/*! \file Sallen_and_Key.cpp
	This program calculates the component values for a Sallen and Key stage to give a specified T and q.
*/

using namespace std;


/*
class stage {
public:
	double T, q;
	double R1, R2, R3, C1, C2;
	double gain;
	char type;
	circuit_class cct;
	void low_pass(void);
	void high_pass(void);
	void band_pass(void);
	void synthesise_SK_low(void);
	void synthesise_SK_high(void);
	void synthesise_SK_band(void);
};
*/

extern double e24[ ];
extern double e12[ ];
extern double e6[ ];
enum ee { _e6, _e12, _e24 };

extern double nearest(double v, ee e1);

double Fn_e6(double c)
{
	int i = 0;
	double temp;
	float e6[] = { 10.0, 6.8, 4.7, 3.3, 2.2, 1.5, 1.0, 0.68 };
	double exponent = pow10(int(log10(c)));
	double mantissa = c / exponent;
	cout << "mantissa = " << mantissa << " exponent = " << exponent << "\n";
	do {
		temp = sqrt(e6[i] * e6[i+1]) / 10.0;
		if(temp <= mantissa) break;
		i++;
		if(i > 7) { cout << "i Error!\n"; return(0); }
	} while(1);
	return(e6[i] * exponent);
}



double Fn_e12(double c)
{
	int i = 0;
	double temp;
	float e12[] = { 10.0, 8.2, 6.8, 5.6, 4.7, 3.9, 3.3, 2.7, 2.2, 1.8, 1.5, 1.2, 1.0, 0.82 };
	double exponent = pow10(int(log10(c)));
	double mantissa = c / exponent;
	do {
		temp = sqrt(e12[i] * e12[i+1]) / 10.0;
		if(temp <= mantissa) break;
		i++;
		if(i > 13) { cout << "i Error!\n"; return(0); }
	} while(1);
	return(e12[i] * exponent);
}



void low( )
{

}



void stage::synthesise_SK_low(void)
{
#if SHOW_CALC
	cout << "stage::synthesise_SK_low\n";
#endif
#if true
	double aa, bb, cc;
    double r = 1e4;
    double c = T / r;
    // q = T / (C1 * R1 + C1 * R2 + (1.0 - gain) * C2 * R1);
    double d = sqrt(2.0)/1.25;
	do {
		d *= 1.25;
		C1 = c / d;
		C2 = c * d;
#if SHOW_CALC
		cout << "C1 = " << C1 << ", C2 = " << C2 << "\n";
#endif
		// R1 = r / s;
		// R2 = r * s;
		// c / d * r * s^2 + c / d * r +  (1.0 - gain) * c * d * r - s * T / q = 0
		aa =  c / d * r;
		bb =  -T / q;
		cc =  c / d * r +  (1.0 - gain) * c * d * r;
	} while(bb * bb < 5.0 * aa * cc);
    double sq = sqrt(bb * bb - 4.0 * aa * cc);
#if SHOW_CALC
    cout << "-bb = " << -bb << ", sq = " << sq << ", aa = " << aa << "\n";
#endif
    double s1 = (-bb + sq) / (2.0 * aa);
    double s2 = (-bb - sq) / (2.0 * aa);
    
	// Now choose e6 preferred values of capacitor
	C1 = nearest(C1, _e6);
	C2 = nearest(C2, _e6);
	c = sqrt(C1 * C2);
	d = sqrt(C2 / C1);
	r = sqrt(T*T / (C1 * C2));
	aa =  c / d * r;
	bb =  -T / q;
	cc =  c / d * r +  (1.0 - gain) * c * d * r;
    sq = sqrt(bb * bb - 4.0 * aa * cc);
    s1 = (-bb + sq) / (2.0 * aa);
    s2 = (-bb - sq) / (2.0 * aa);

    R1 = r / s1;
    R2 = r * s1;
#if SHOW_CALC
    cout << "C1 = " << C1 << ", C2 = " << C2 << ", R1 = " << R1 << ", R2 = " << R2 << "\n";
	cout << "T =" << sqrt(R1 * R2 * C1 * C2) << ", q =" << sqrt(R1 * R2 * C1 * C2) / (C1 * R1 + C1 * R2 + (1.0 - gain) * C2 * R1) << "\n";
#endif    
    R1 = r / s2;
    R2 = r * s2;
#if SHOW_CALC
    cout << "C1 = " << C1 << ", C2 = " << C2 << ", R1 = " << R1 << ", R2 = " << R2 << "\n";
	cout << "T =" << sqrt(R1 * R2 * C1 * C2) << ", q =" << sqrt(R1 * R2 * C1 * C2) / (C1 * R1 + C1 * R2 + (1.0 - gain) * C2 * R1) << "\n";
#endif    

#else
//    double k = 1.0;
	double aa, bb, cc;
    double r = 1e4;
    double c = T / r;
    // q = T / (C1 * R1 + C1 * R2 + (1.0 - gain) * C2 * R1);
    double d = sqrt(2.0)/1.25;
	do {
		d *= 1.25;
		C1 = c / d;
		C2 = c * d;
		cout << "C1 = " << C1 << ", C2 = " << C2 << "\n";
		// R1 = r / s;
		// R2 = r * s;
		// c / d * r * s^2 + c / d * r +  (1.0 - gain) * c * d * r - s * T / q = 0
		aa =  c / d * r;
		bb =  -T / q;
		cc =  c / d * r +  (1.0 - gain) * c * d * r;
	} while(bb * bb < 5.0 * aa * cc);
    double sq = sqrt(bb * bb - 4.0 * aa * cc);
    cout << "-bb = " << -bb << ", sq = " << sq << ", aa = " << aa << "\n";
    double s1 = (-bb + sq) / (2.0 * aa);
    double s2 = (-bb - sq) / (2.0 * aa);
    
    R1 = r / s1;
    R2 = r * s1;
    cout << "C1 = " << C1 << ", C2 = " << C2 << ", R1 = " << R1 << ", R2 = " << R2 << "\n";
//    R1 = R1, R2 = R2, C1 = C1, C2 = C2, R3 = 0.0;
    
    R1 = r / s2;
    R2 = r * s2;
    cout << "C1 = " << C1 << ", C2 = " << C2 << ", R1 = " << R1 << ", R2 = " << R2 << "\n";
#endif

#if SHOW_CALC
	cout << "End stage::synthesise_SK_low\n";
#endif
}



void stage::synthesise_SK_high(void)
{
	double cf;
	R1 = 47.0E3;
	R2 = R1 * (1.0 / (4.0 * q * q) + (gain - 1.0) * R1);
	C1 = T / (2.0 * R2 * q);
	cf = Fn_e6(C1) / C1;
	C1 = C1 * cf; C2 = C1;
	R1 = R1 / cf; R2 = R2 / cf;
}



/** Synthesize Sallen and Key bandpass stage.
Let R1 = R3 = R, R2 = 2R, C1 = C2 = C.
Then set q = 1 / (3 - k) i.e. k = 3.0 - 1/q
w0 = 1 / (R * C)
[The maximum gain of the filter stage will be k / (3 - k)]
*/
void stage::synthesise_SK_band(void)
{
	double c, r;
	gain = 3.0 - 1.0 / q;
	r = 1e4;
	c = T / r;
	C1 = C2 = nearest(c, _e6);
	r = T / C1;
	R1 = R3 = r;
	R2 = 2.0 * r;

/*	cout << "Gain = " << gain;
	cout << "\nR1 = " << R1 << ", R2 = " << R2 << ", R3 = " << R3 << "\n";
	cout << "C1 =" << C1 << ", C2 =" << C2 /* << "v =" << v << " b =" << b* / << "\n";
*/
}



void TFilter::Synth_SallKey(void)
{
	int i;
	ofstream file;
	file.open("./Circuit.txt");
	file << "Order = " << poles << "\n";
	for(i=0; i<poles/2; i++) {
		st[i].fclass = fclass;
//		st[i].T = st[i].T;
		st[i].q = st[i].q; 
		st[i].gain = 1.0;
		switch(st[i].fclass) {
			case Lowpass: st[i].synthesise_SK_low( ); file << "Lowpass\n"; break;
			case Highpass: st[i].synthesise_SK_high( );  file << "Highpass\n";break;
			case Bandpass: st[i].synthesise_SK_band( );  file << "Bandpass\n";break;
			default: cout << "Unknown fclass\n"; exit(-1);
		}

		cout << "\n*************************************\nSecond order stage: Sallen and Key\n";
		cout << "Stage " << i+1 << "\n";
		file << "Stage = " << i << "\n";
		file << "Sallen and Key\n";
		cout << "Gain = " << st[i].gain << "\n"; file << "Gain = " << st[i].gain << "\n"; 
		cout << "R1: " << st[i].R1; file << "R1 = " << st[i].R1;
		cout << ", R2: " << st[i].R2; file << "\nR2 = " << st[i].R2;
		if(st[i].fclass == Bandpass) { cout << ", R3: " << st[i].R3; file << "\nR3 = " << st[i].R3;}
		cout << "\nC1: " << st[i].C1; file << "\nC1 = " << st[i].C1;
		cout << ", C2: " << st[i].C2; file << "\nC2 = " << st[i].C2;
//		if(st[i].fclass == Highpass) { cout << ", C3: " << st[i].C3; file << "\nC3 = " << st[i].C3; }
//		cout << "\nAmplifier gain = " << st[i].gain; file << "\nAmplifier gain = " << st[i].gain;
		cout << "\n"; file << "\n";
	}
	// Now for the odd pole if there is one.
	if(poles%2) {
		st[i].fclass = fclass;
//		st[i].T = st[i].T;
		st[i].R1 = 1E4;
		st[i].C1 = abs(st[i].T) / st[i].R1;
		st[i].C1 = nearest(st[i].C1, _e6);
		st[i].R1 = abs(st[i].T) / st[i].C1;
		switch(st[i].fclass) {
			case Lowpass: break;
			case Highpass: break;
			case Bandpass: cout << "Impossible! order of bandpass filter cannot be odd.\n"; return;
			default: cout << "Unknown fclass\n"; exit(-1);
		}
		cout << "\n*************************************\nFirst order stage:\n";
		file << "First order stage:\n";
		cout << "R1: " << st[i].R1; file << "R1 = " << st[i].R1;
		cout << "\nC1: " << st[i].C1 << "\n"; file << "\nC1 = " << st[i].C1;
	}
	cout << "*************************************\n";
	if(file.is_open( )) file.close( );
}




#if false
stage s1;


int main( )
{
/* Tests
	cout << "123.0 -> " << Fn_e12(123.0) << "\n";
	cout << "99.0 -> " << Fn_e12(99.0) << "\n";
	cout << "3000.0 -> " << Fn_e12(3000.0) << "\n";
	cout << "1800000.0 -> " << Fn_e12(1800000.0) << "\n";
*/

/*	s1.T = 1e-3;
	s1.q = 0.691;
	s1.gain = 2.0;
*/
	cout << "L, H or B:\n"; cin >> s1.type;
	if(s1.type != 'L' && s1.type != 'H' && s1.type != 'B' && s1.type != 'l' && s1.type != 'h' && s1.type != 'b') {
		cout << "Not a valid option!\n"; return(-1);
	}
	cout << "Gain: \n"; cin >> s1.gain;
	cout << "T: \n"; cin >> s1.T;
	cout << "q: \n"; cin >> s1.q;
	switch(s1.type) {
		case 'L':
		case 'l': s1.synthesise_SK_low( ); break;
		case 'H':
		case 'h':  s1.synthesise_SK_lowh( ); break;
		case 'B':
		case 'b': s1.synthesise_SK_lowb( ); break;
		default: cout << "\nThat shouldn't happen!\n"; exit(-1);
	}
	cout << "R1: " << s1.R1 << "\n";
	cout << "R2: " << s1.R2 << "\n";
	cout << "C1: " << s1.C1 << "\n";
	cout << "C2: " << s1.C2 << "\n";
}
#endif





