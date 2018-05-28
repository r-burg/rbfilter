/******************************************************************************

                                Rauch.cpp

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include "Calcs.h"

#if SHOW_FILE_COMPILING
#pragma message "Compiling Rauch.cpp"
#endif

/*! \file Rauch.cpp
	This program calculates the T and q values for a Rauch stage from the component values and vice-versa.
*/

using namespace std;


stage stage1;


double e24[ ] = { 1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,\
 3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1, 10.0 };

double e12[ ] = { 1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2, 10.0 };

double e6[ ] = { 1.0, 1.5, 2.2, 3.3, 4.7, 6.8, 10.0 };

enum ee { _e6, _e12, _e24 };


/**
Find the nearest e6, e12 or e24 preferred value to 'v'.
*/
double nearest(double v, ee e1)
{
    int i, n;
    double l = log10(v);
    double p = int(l);			// Power of ten: i.e. exponent
    double f = l - p;
    double x = pow10(f+1.0);	// Mantissa
	double *ep;
	switch(e1) {
		case _e6: ep = e6; n = 6; break;
		case _e12: ep = e12; n = 12; break;
		case _e24: ep = e24; n = 24; break;
		default: cout << "Don't understand!";
		exit(-1);
	};
    for(i=0; i<n; i++) {
        if(ep[i] > x) break;
    }
    if(i == 0) {
        cout << "Value below 1.0\n";
        return(1.0);
    }
    return((x/ep[i-1] < ep[i]/x)? ep[i-1] * pow10(p-1.0): ep[i] * pow10(p-1.0));
}



/** Here comes a clever bit! Design a low-pass stage, choosing e6 values only for the capacitors, keeping sensible values for resistors.
Note: gain = R2 / R1; T = sqrt(C1.C2.R2.R3); q = T / ((1 + R2/R3 + R2/R1).C2.R3)
*/
void stage::synthesise_R_low(void)
{
#if false
	double R, C, r, r1;
	double a, b, c;
	R1 = R2 = R3 = 1E4;
//	R3 /= 1.1;
	r1 = 10.0 / 1.2;
	do {
		r1 *= 1.2;
		/// Choose ideal capacitor values; T^2 = R2*R3*C1*C2 --> C1*C2 = T^2/(R2*R3)
		C = sqrt((T * T) / (R2 * R3));
		r = sqrt(r1) * q;
		C1 = C * r;
		C2 = C / r;
		// If R1 = R2 = R3 then q = T/(3*C2*R3)
//		R3 *= 1.1;
		/// Pick the nearest e6 values for caps.
		cout << "Nearest to " << C1 << " is " << nearest(C1, _e6) << "\n";
		cout << "Nearest to " << C2 << " is " << nearest(C2, _e6) << "\n";
		C1 = nearest(C1, _e6);
		C2 = nearest(C2, _e6);
		/// Restore T to correct value
		R = sqrt(T * T / (C1 * C2));
		/// Adjust resistors to get back to desired q, keeping T and gain unchanged
		a = C2*R;
		b = -T / q;
		c = 2.0*C2*R;
	} while(b*b <= 4.0 * a * c);
	double r = (-b + sqrt(b*b - 4.0 * a * c)) / (2.0 * a);
	R1 = R * r;
	R2 = R1;
	R3 = R / r;
#else
	double R, C, r, r1;
	double a, b, c;
#if SHOW_CALC
	cout << "\nsynthesise_low\n";
#endif
	R1 = R2 = R3 = 1E4;
	r1 = 10.0 / 1.2;
	do {
		r1 *= 1.2;
		/// Choose ideal capacitor values; T^2 = R2*R3*C1*C2 --> C1*C2 = T^2/(R2*R3)
		C = sqrt((T * T) / (R2 * R3));
		r = sqrt(r1) * q;
		C1 = C * r;
		C2 = C / r;
		// If R1 = R2 = R3 then q = T/(3*C2*R3)
		/// Pick the nearest e6 values for caps.
#if SHOW_CALC
		cout << "Nearest to " << C1 << " is " << nearest(C1, _e6) << "\n";
		cout << "Nearest to " << C2 << " is " << nearest(C2, _e6) << "\n";
#endif
		C1 = nearest(C1, _e6);
		C2 = nearest(C2, _e6);
		/// Restore T to correct value
		R = sqrt(T * T / (C1 * C2));
		/// Adjust resistors to get back to desired q, keeping T and gain unchanged
		a = C2*R;
		b = -T / q;
		c = 2.0*C2*R;
	} while(b*b <= 4.0 * a * c);
	r = (-b + sqrt(b*b - 4.0 * a * c)) / (2.0 * a);
	R1 = R * r;
	R2 = R1;
	R3 = R / r;
#if SHOW_CALC
	cout << "End synthesise_low\n";
#endif

#endif
}



/// Calculate gain, T and q from component values
void stage::R_low_pass(void)
{
	gain = -R2 / R1;
	T = sqrt(C1*C2*R2*R3);
	q = T / ((1.0 + R2/R3 + R2/R1)*C2*R3);
}



/** Design a band-pass stage, choosing e6 preferred values for the capacitors. Note:
gain = 1 / ((C1.R1)*(2+R1.(C1+C2)/(C1.C2.R1.R2))); T = sqrt(C1.C2.R1.R2); T/q = R1.(C1 + C2); i.e. q = T / (R1.(C1 + C2))
*/
void stage::synthesise_R_band(void)
{
	double r;
	double a, b, c;
	double C;
	double R;

    cout << "There may be a problem in this function. Try another method of realization.\n";
    return;

#if SHOW_CALC
	cout << "\nsynthesise_band\n";
#endif
	R1 = 1.0e4; R2 = 1.0e4/1.5;
	do {
		/// Choose ideal capacitor values; T^2 = R1*R2*C1*C2 --> C1*C2 = T^2/(R1*R2)
		// Let C1 = C.r, C2 = C/r
		// If R1 = R2 then q = T/(R1*(C.r+C/r))
		// C.r^2 - T/(q * R1).r + C = 0
		R2 *= 1.5;			/// Try progressively increasing ratios of R2:R1 until a solution is possible.
		C = T / sqrt(R1*R2);
		R = sqrt(R1*R2);
		a = C;
		b = -T / (q * R1);
		c = C;
	} while (b*b < 5.0*a*c);
	r = (-b + sqrt(b*b - 4.0 * a * c * 1.1)) / (2.0 * a);
	C1 = C * r;
	C2 = C / r;

	/// Pick the nearest e6 values for caps.
#if SHOW_CALC
	cout << "Nearest to " << C1 << " is " << nearest(C1, _e6) << "\n";
	cout << "Nearest to " << C2 << " is " << nearest(C2, _e6) << "\n";
#endif
	C1 = nearest(C1, _e6);
	C2 = nearest(C2, _e6);
	/// Adjust resistors to get back to desired q, keeping T unchanged
	R = sqrt(T * T / (C1 * C2));
	R1 = sqrt(C1*C2*R*R) / (q*(C1+C2));
	R2 = R * R / R1;

#if SHOW_CALC
	cout << "\nEnd synthesise_band\n";
#endif
}



/** Calculate gain, T and q from component values for a band-pass stage. Note
gain = -C1/C3; T = sqrt(C2.C3.R1.R2); T/q = R1.(C1+C3); i.e. q = T / (R1.(C1+C3))
*/
void stage::R_band_pass(void)
{
	gain = -1.0 / ((C1*R1)*(2+R1*(C1+C2)/(C1*C2*R1*R2)));
	T = sqrt(R1*R2*C1*C2);
	q = T / (R1*(C1 + C2));
}



/** Design a high pass stage, choosing e6 preferred values for C2 and C3. If the gain is to be -1
the value of C1 will be identical to C2.
*/
void stage::synthesise_R_high(void)
{
#if false
	double r;
	double a, b, c;
	double C;
	double R;
#if SHOW_CALC
	cout << "\nsynthesise_high\n";
#endif
	R1 = 1.0e4; R2 = 1.0e4/1.5;
	do {
		R2 *= 1.5;
		R = sqrt(R1 * R2);
		// T = sqrt(R1*R2*C2*C3)
		C = T / sqrt(R1 * R2);
		a = -gain * C;
		b = -T / (q * R1);
		c = C;
	} while (b*b <= 4.0 * a * c * 0.9);
	r = (-b + sqrt(b*b - 4.0 * a * c)) / (2.0 * a);
//	cout << "C =" << C << " r =" << r << "\n";
	C2 = C * r;
	C3 = C / r;
	C1 = -gain * C * r;
#if SHOW_CALC
	cout << "Ideal values: C1 =" << C1 << " C2 =" << C2 << " C3 =" << C3 << "\n";
	cout << "R1 =" << R1 << " R2 =" << R2 << "\n";
#endif

	/// Pick the nearest e6 values for caps C2 and C3.
#if SHOW_CALC
	cout << "Nearest to " << C2 << " is " << nearest(C2, _e6) << "\n";
	cout << "Nearest to " << C3 << " is " << nearest(C3, _e6) << "\n";
#endif
	C2 = nearest(C2, _e6);
	C3 = nearest(C3, _e6);
	C1 = -gain * C2;

	/// Adjust resistors to get back to desired q, keeping T unchanged
	R1 = T / (q * (C1 + C3));
	R2 = T * T / (C2 * C3 * R1);
#endif
	double R, C, r, c;
/// Calculate ideal component values
	R = 1e4;
	C = T / R;
	r = 1.0;
	c = 1.0;
	C2 = C * c;
	C3 = C / c;
	C1 = C2 / -gain;
	R1 = T / (q * (C1 + C2 + C3));
	R2 = R * R / R1;
	cout << "\nIdeal values: C1 =" << C1 << " C2 =" << C2 << " C3 =" << C3 << "\n";
	cout << "R1 =" << R1 << " R2 =" << R2 << "\n";

/// Convert to more practical components, capacitors at least!
	cout << "\nPractical values:\n";
	C2 = nearest(C2, _e6);
	C3 = nearest(C3, _e6);
	C1 = -gain * C2;
	R1 = T / (q * (C1 + C2 + C3));
	R2 = T * T / (R1 * C2 * C3);


#if SHOW_CALC
	cout << "\nEnd synthesise_high\n";
#endif
}



/// Calculate gain, T and q from component values for a high-pass filter
void stage::R_high_pass(void)
{
	gain = -C1/C2;
	T = sqrt(C3*C2*R1*R2);
	q = T / (R1*(C1 + C3));
}




void TFilter::Synth_Rauch(void)
{
	int i;
	stage *pstage;
	ofstream file;
	file.open("./Circuit.txt");
	file << "Order = " << poles << "\n";
	for(i=0, pstage=st; i<poles/2; i++) {
		pstage->fclass = fclass;
		pstage->T = st[i].T;
		pstage->q = st[i].q;
		pstage->gain = -1.0;
		cout << "\n*************************************\nSecond order stage: Rauch\n";
		cout << "Stage " << i+1 << "\n";
		switch(pstage->fclass) {
			case Lowpass: pstage->synthesise_R_low( ); file << "Lowpass\n"; break;
			case Highpass: pstage->synthesise_R_high( );  file << "Highpass\n";break;
			case Bandpass: pstage->synthesise_R_band( );  file << "Bandpass\n";break;
			default: cout << "Unknown fclass\n"; exit(-1);
		}
//		cout << "\n*************************************\nSecond order stage: Rauch\n";
		file << "Stage = " << i << "\n";
		file << "Rauch\n";
		cout << "R1: " << pstage->R1; file << "R1 = " << pstage->R1;
		cout << ", R2: " << pstage->R2; file << "\nR2 = " << pstage->R2;
		if((pstage->fclass == Lowpass)||(pstage->fclass == Bandpass)) { cout << ", R3: " << pstage->R3; file << "\nR3 = " << pstage->R3;}
		cout << "\nC1: " << pstage->C1; file << "\nC1 = " << pstage->C1;
		cout << ", C2: " << pstage->C2; file << "\nC2 = " << pstage->C2;
		if(pstage->fclass == Highpass) { cout << ", C3: " << pstage->C3; file << "\nC3 = " << pstage->C3; }
		cout << "\n"; file << "\n";
		pstage++;
	}
	// Now for the odd pole if there is one.
	if(poles%2) {
		pstage->fclass = fclass;
		pstage->T = abs(st[i].T);
		pstage->R1 = 1E4;
		pstage->C1 = pstage->T / pstage->R1;
		pstage->C1 = nearest(pstage->C1, _e6);
		pstage->R1 = pstage->T / pstage->C1;
		switch(pstage->fclass) {
			case Lowpass: break;
			case Highpass: break;
			case Bandpass: cout << "Impossible! order of bandpass filter cannot be odd.\n"; return;
			default: cout << "Unknown fclass\n"; exit(-1);
		}
		cout << "\n*************************************\nFirst order stage:\n";
		file << "First order stage:\n";
		cout << "R1: " << pstage->R1; file << "R1 = " << pstage->R1;
		cout << "\nC1: " << pstage->C1 << "\n"; file << "\nC1 = " << pstage->C1;
	}
	cout << "*************************************\n";
	if(file.is_open( )) file.close( );
}



#if false
int main( )
{
	char c;
	do {
		cout << "Synthesise or Analyse?\n"; cin >> c;
		if(c == 'S' || c == 's') {
			do {
				cout << "Lowpass, Bandpass or Highpass?\n"; cin >> c;
				if(c != 'L' && c != 'l' && c != 'B' && c != 'b' && c != 'H' && c != 'h') continue;
			} while(false);
			do {
				cout << "Gain:\n"; cin >> stage1.gain;
				if(stage1.gain >= 0.0) cout << "No! Gain must be negative.\n";
			} while(stage1.gain >= 0.0);
			cout << "T:\n"; cin >> stage1.T;
			cout << "q:\n"; cin >> stage1.q;

			switch(c) {
				case 'L':
				case 'l': stage1.synthesise_low( ); break;
				case 'B':
				case 'b': stage1.synthesise_band( ); break;
				case 'H':
				case 'h': stage1.synthesise_high( ); break;
			}
			cout << "C1 =" << stage1.C1;
			cout << " C2 =" << stage1.C2;
			if(c == 'h' || c == 'H') cout << " C3 =" << stage1.C3;
			cout << "\n";
			cout << "R1 =" << stage1.R1;
			cout << " R2 =" << stage1.R2;
			if(c == 'l' || c == 'L') cout << " R3 =" << stage1.R3 << "\n";

			switch(c) {
				case 'L':
				case 'l': stage1.low_pass( ); break;
				case 'B':
				case 'b': stage1.band_pass( ); break;
				case 'H':
				case 'h': stage1.high_pass( ); break;
			}
			cout << "Check: gain = " << stage1.gain << " T = " << stage1.T << " q = " << stage1.q << "\n";
		} else {
			cout << "Lowpass, Bandpass or Highpass?\n"; cin >> c;
			if(c != 'L' && c != 'l' && c != 'B' && c != 'b' && c != 'H' && c != 'h') continue;
			cout << "R1: \n"; cin >> stage1.R1;
			cout << "R2: \n"; cin >> stage1.R2;
			if(c == 'L' || c == 'l') { cout << "R3: \n"; cin >> stage1.R3; }
			cout << "C1: \n"; cin >> stage1.C1;
			cout << "C2: \n"; cin >> stage1.C2;
			if(c == 'H' || c == 'h') { cout << "C3: \n"; cin >> stage1.C3; }
			switch(c) {
				case 'L':
				case 'l': stage1.low_pass( ); break;
				case 'B':
				case 'b': stage1.band_pass( ); break;
				case 'H':
				case 'h': stage1.high_pass( ); break;
			}
			cout << "gain = " << stage1.gain << " T = " << stage1.T << " q = " << stage1.q << "\n";
		}
		cout << "Again?\n";
		cin >> c;
	} while(c == 'y' || c == 'Y');
}
#endif



