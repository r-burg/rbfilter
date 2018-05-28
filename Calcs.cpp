/******************************************************************************

                                Calcs.cpp

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


/** \file
'Calcs.cpp' is one of the files intended to hold the functions for 
calculation purposes.

It is not involved with GUIs and so it is expected that it will not (or 
barely) need editting for porting to a different OS.
*/

// ***************************
// #include <vcl\vcl.h>
// #pragma hdrstop
#include <cmath>
#include "math.h"
#include "Calcs.h"
#include <iostream>
#include <complex>
#include <stdlib.h>
#include <sstream>
// #include "filter.h"
// #include "control.h"
// ***************************

#if SHOW_FILE_COMPILING
#pragma message "Compiling Calcs.cpp"
#endif

TFilter filter;

TFilter::TFilter( )
{
	fclass = Lowpass;
	sclass = Bessel;
	cclass = SallKey;
	frequency = 1.0;
	bandwidth = 0.25;
  	gain = 1.0;
	poles = 3;
	zeroes = 0;
	fmax = tmax = 2.0;
	ripple = 0.1;
	a0 = 0.70794578;
	type = '0';		// 'Prototype' filter.
}



double isinh(double x)
{
	double y=1.0;
	do {
		y+=(x-sinh(y))/cosh(y);
	} while(((sinh(y)-x)/x) >=0.001);
	return(y);
}



double n_to_freq(int n, double f1, int nmax=FSTEPS, int decades=2)
{
	return(f1 * pow10(double(n) * decades / nmax));
}




using namespace std;

// void bessel(TFilter& filter);
// void butterworth(TFilter& filter);
// void chebyshev(TFilter& filter);
// void lowpass(TFilter& filter, double f);
// void highpass(TFilter& filter);

void geffe(double sigma, double omega, double w0, double bw, double &a1, double &b1, double &a2, double &b2);

double atof(string s)
{
	double d;
	sscanf(s.c_str(), "%lf", &d);
	return(d);
}



void
TFilter::_pole(double a, double w)
{
// Draw pole at (a,w).
int aa, ww;

#if VERBOSE
	cout << "Pole at " << a << ", " << w << "\n";
#endif
}



void TFilter::show_filter(void)
{
	int i;
#if VERBOSE
	cout << "No of poles:" << poles << "\n";
	cout << "No of zeroes:" << zeroes << "\n";

	for(i=0; i<poles; i++) cout << "i: " << st[i].pole << "\n";
#endif
}



void TFilter::show_filter(string title)
{
	int i;
#if VERBOSE
	cout << title << "\n";
	cout << "No of poles:" << poles << "\n";
	cout << "No of zeroes:" << zeroes << "\n";

	for(i=0; i<poles; i++) cout << "i: " << st[i].pole << "\n";
#endif
}



/// Assignment operator; don't actually need to copy all data for this application.
stage& stage::operator= (stage& f1)
{
	this->fclass = f1.fclass;
	this->cclass = f1.cclass;
	this->T = f1.T;
	this->q = f1.q;
	this->pole = f1.pole;
	this->zero = f1.zero;
}



/// Put the poles into falling order of imaginary part. As only a few objects are to be sorted, use a bubble sort.
void TFilter::sort(void)
{
	stage s0;
	int i, j;
	for(j=0; j < poles-1; j++) for(i=0; i < poles-1; i++) {
		if(st[i].pole.imag( ) < st[i+1].pole.imag( )) {
			// poles in wrong order, swap them.
			s0.pole = st[i].pole;
			st[i].pole = st[i+1].pole;
			st[i+1].pole = s0.pole;
		}
	}
}



double TFilter::fgain(double f)
{
	unsigned i, k;
	double t, q;
	complex<double> j = std::complex<double>(0.0, 1.0);
	complex<double> s, r;
	double rf;
	rf = 1.0;
	for(k=0; k<(poles/2); k++) {
		t = st[k].T, q = st[k].q;
		s = std::complex<double>(0.0, (2.0*M_PI)*f);
		r = 1.0/(1.0+s*t/q+s*s*t*t);
		rf *= abs(r);
	}
	if(k < (poles+1)/2) {
		s = std::complex<double>(0.0, (2.0*M_PI)*f);
		r = 1.0/(1.0+s*tau);
		rf *= abs(r);
	}
	return(rf);
}



/// We want to normalise the filter so the cut-off is at 1.0 radians per second.
void TFilter::Proto_normalise(void)
{
	int k;
	double f, f1;
	double g, g1;
	char logstr[256];
	log("\n<Normalise prototype>");
	for(f = 0.1; f < 10.0; f *= 1.01) {
		g = fgain(f);
		if(g < a0) break;
		g1 = g;
		f1 = f;
	}
//	std::cout << "Gain is " << g1 << " at " << f1 << ", and " << g << " at " << f << "\n";
	sprintf(logstr, "Prototype gain is %f at %f, and %f at %f", g1, f1, g, f);
	log(logstr);
	f = 2.0 * M_PI * (f * (-a0 + g1) + f1 * (-g + a0)) / (-g + g1);
	for(k=0; k<(poles); k++) {
		st[k].pole /= f;
	}

	std::cout << "f = " << f << "\n";
	show_filter("Normalised prototype");
	log("</Normalise prototype>\n");
}



/**
Calculate the t and q values for each stage from the poles with
positive imaginary parts (i.e. one of each pair), and if the 
number of poles is odd, calculate the tau value from the remaining 
pole.
*/
void TFilter::ts_and_qs(void)
{
	unsigned i, j, k;
	double theta;
	double tt, qq;

	log("<Determine Ts and qs>");

#if VERBOSE
	cout << "ts_and_qs\n";
#endif
	switch(fclass) {
		case Lowpass: k = poles/2; break;
		case Highpass: k = poles/2; break;
		case Bandpass: k = poles; break;
	}

	for(i=0, j=0; i<k; i++) {
		if(st[i].pole.imag( ) < 0.0) continue;
		tt=1.0/abs(st[i].pole),
		qq=-1.0/(2.0*tt*real(st[i].pole));
		st[j].T = tt;
		st[j].q = qq;
		log("T=", tt);
		log("q=", qq);
		++j;
	}
	if(poles%2) {
		tau = 1.0 / st[poles/2].pole.real();
		log("Tau=", -1.0/tau);
		st[j].T = tau;
	}
#if VERBOSE
	cout << "end ts_and_qs\n";
#endif
	log("</Determine Ts and qs>");
}



void TFilter::butterworth(void)
{
	unsigned i;
	double theta;
	double tt, qq;
	log("<Butterworth>");
	theta=M_PI/(2.0*poles);
	for(i=0; i<poles; i++) {
		if((poles%2) && (i == poles/2)) {
//			pole[i] = std::complex <double>(-sin(theta)*2.0*M_PI, 0.0);
			st[i].pole = std::complex <double>(-sin(theta), 0.0);
		} else {
//			pole[i] = std::complex <double>(-sin(theta)*2.0*M_PI, cos(theta)*2.0*M_PI);
			st[i].pole = std::complex <double>(-sin(theta), cos(theta));
		}
		log("pole at ", st[i].pole);
		theta += M_PI/poles;
	}

	type = 'P';

	log("/Butterworth");
}



void
TFilter::bessel(void)
{
int i, j, k, nn;
double a[20], b[20], bb[20], bbb[20], aa[10], ww[10], limit, p, q,
	qq, tt, dd, t, w;
	std::ofstream file;
	int n;

	log("<Bessel>");
	n = poles;
	/// Bessel-Thomson filter design.
	if((n<3)||(n>19)) {
		cout << "Unreasonable order.\n";
		return;
	}
	nn=n;
	limit=exp(std::log(3.5)*n)*0.00001;
	/// Evaluate the terms of the polynomial.
	for(i=0; i<=19; i++) a[i]=b[i]=bb[i]=bbb[i]=0;
	bbb[0]=bbb[1]=1.0;
	bb[0]=bb[1]=3.0, bb[2]=1.0;
	for(i=3; i<=n; i++) {
		for(j=0; j<=n; j++)b[j]=(2*i-1)*bb[j];
		for(j=2; j<=n; j++) b[j]+=bbb[j-2];
		for(j=0; j<=n; j++) {
			bbb[j]=bb[j];
			bb[j]=b[j];
		}
	}
#if VERBOSE
	cout << "Bessel polynomial:\n";
	for(i=0; i<=n; i++) cout << "a[" << i << "]=" << b[i] << " "; cout << "\n";
#endif
	for(i=0; i<=n; i++) a[n-i]=b[i];
	k=0;
	file.open("pfile.z");
	if(!file) {
		cout << "\nUnable to open 'pfile.z'\n";
		return;
	}
	file << "Bessel: P " << n << " " << 0;
	// 440
	int pp = 0;
	do {
		p=q=0.01;
		do {
			b[0]=a[0];
			b[1]=a[1]-b[0]*p;
			for(i=2; i<=n;i++) b[i]=a[i]-b[i-2]*q-b[i-1]*p;
			if(b[n-2]==0) b[n-2]=0.000001;
			q=a[n]/b[n-2];	//530
			p=(a[n-1]-b[n-3]*q)/b[n-2];
		} while((abs(b[n-1]) >= limit) || (abs(b[n]) >= limit));
		cout << "Solution at " << -p/2.0 << " +/- j*" << sqrt(q-(p*p/4.0)) << "\n";
		file << " " << -p/2.0 << " " << sqrt(q-(p*p/4.0)) << "\n";
		st[pp++].pole = complex<double>(-p/2.0, sqrt(q-(p*p/4.0)));
		st[pp++].pole = complex<double>(-p/2.0, -sqrt(q-(p*p/4.0)));
		aa[k]=-p/2.0, ww[k]=sqrt(q-(p*p/4.0));
		k++;
		for(i=0; i<=n; i++) a[i]=b[i];
		n-=2;
	} while(n>2);
	if(n==1) {
		// 720
		cout << "Solution at " << -b[1]/b[0] << "\n";
		file << " " << -b[1]/b[0];
		st[pp++].pole = complex<double>(-b[1]/b[0], 0.0);
	} else {
		//660
		cout << "Solution' at " << -a[1]/(2.0*a[0]);
		cout << " +/-j*" << (sqrt(-a[1]*a[1]+4.0*a[0]*a[2])/(2.0*a[0]));
		file << " " << -a[1]/(2.0*a[0]) << " " << sqrt(-a[1]*a[1]+4.0*a[0]*a[2])/(2.0*a[0]);
		st[pp++].pole = complex<double>(-a[1]/(2.0*a[0]), sqrt(-a[1]*a[1]+4.0*a[0]*a[2])/(2.0*a[0]));
		st[pp++].pole = complex<double>(-a[1]/(2.0*a[0]), -sqrt(-a[1]*a[1]+4.0*a[0]*a[2])/(2.0*a[0]));
		k++;
	}
	file.close();

	for(i=0; i<=nn/2; i++) {
		_pole(aa[i]/3.0, ww[i]/3.0);
		_pole(aa[i]/3.0, -ww[i]/3.0);
	}
	if(nn%2) {
		_pole(-b[1]/(3.0*b[0]), 0);
//		pole[nn - 1] = complex<double>(-b[1]/(3.0*b[0], 0.0));
	}

#if VERBOSE
	cout << "\nEndBessel\n";
#endif

	// 940
	if(dd!='D') ;
	type = 'P';

	log("</Bessel>");
}



void
TFilter::chebyshev(void)
{
char dump;

int i;

double aa[20], ww[20], g, a, w, tt, qq;

	// Chebyshev filter design.
	log("<Chebyshev>");
	unsigned n = poles;
	g = ripple;

	for(i=1; i<=n; i++) {
		a=-sin((2*i-1)*M_PI/(2*n))*sinh(isinh(1.0/g)/n);

		w=cos((2*i-1)*M_PI/(2*n))*cosh(isinh(1.0/g)/n);

		if(fabs(w)<1e-9) w=0;

		st[i-1].pole = complex<double>(a, w);

		cout << "\n" << a << ", " << w;
		aa[i]=-a, ww[i]=w;
	}

	type = 'P';

#if VERBOSE
	cout << "EndChebyshev\n";
#endif

	log("</Chebyshev>");
}



void
TFilter::lowpass(void)
{
	unsigned i;
	log("<lowpass>");
// Mod 2016-09-01-RB
//	for(i=0; i<poles; i++) st[i].pole *= 2.0 * M_PI * frequency;
	for(i=0; i<poles; i++) st[i].pole *= frequency;
	fclass = Lowpass;
	zeroes = 0;
	type = 'L';
	log("</lowpass>");
}



/**
Convert the continuous filter to discrete form via the bilinear transformation
*/
void TFilter::bilinear(void)
{
	std::complex<double> _bilinear(std::complex<double> s, double Ts);
	double normalise(double fc, double fs);
	unsigned i;
	double fc, fs, omega;

	log("<bilinear transformation>");

	for(i=0; i<poles/2; i++) {
		fc = 1.0 / (2.0*M_PI*st[i].T);
		fs = samplingfreq;
		if(fc >= fs/2.0) {
			std::cout << "That doesn't make sense! Cut-off must be less than sampling freq / 2.\n" <<
			 "Here fc = " << fc << " and fs = " << fs << "\n";
			return;
		}
		omega = normalise(fc, fs);
		std::cout << "\nStage " << i << "\nomega =" << omega << "\n";
		double omega_ac = tan(omega/2.0);
		std::cout << "omega_ac =" << omega_ac << "\n\n";
		st[i].iir1.transform(fclass, omega_ac, st[i].q);
		st[i].iir1.write(i);
/*		st[i].iir1.step(0.0);
		std::cout << "\n";
		std::cout << "Direct Form 1 realisation.\n--------------------------\n";
		for(i=0; i<200; i++) std::cout << st[i].iir1.step(1.0) << "\n";
*/
//		st[i].z = _bilinear(st[i].pole, 1000.0);
	}

	log("</bilinear transformation>");
}


/**
Calculate the frequency response of the filter.
*/
void
bode_calc(TFilter& filter)
{
//	extern TControl_window *Control_window;
	unsigned i, k;
	double f, w;
	double t, q;
	std::complex<double> s, r, j;
	double temp;
	double max_gain = 0.0;
	ofstream bode_file;

	filter.log("<bode_calc>");
	j = std::complex<double>(0.0, 1.0);
	
//	filter.fmax = 1.0 * filter.frequency * M_PI / 2.0;
	filter.fmax = 0.1 * filter.frequency;
	for(i=0; i<FSTEPS; i++) filter.freq_resp[i] = 1.0;
	for(k=0; k<(filter.poles/2); k++) {
//		t = 1.0 / abs(filter.pole[k]);
//		q = 1.0 / (2.0*t*-real(filter.pole[k]));
		t = filter.st[k].T, q = filter.st[k].q;
		for(i=0; i<FSTEPS; i++) {
#if LINEAR_FREQ
			f = i*filter.fmax/FSTEPS;
#else
			f = n_to_freq(i, filter.fmax);
#endif
			s = std::complex<double>(0.0, (2.0*M_PI)*f);
			r = 1.0/(1.0+s*t/q+s*s*t*t);
			if(filter.fclass == Bandpass) r *= s * t;
			if(filter.fclass == Highpass) r *= s * s * t * t;
			filter.freq_resp[i] *= abs(r);
		}
	}
	if(k < (filter.poles+1)/2) {
//		t = 1.0 / abs(filter.pole[k]);
		for(i=0; i<FSTEPS; i++) {
#if LINEAR_FREQ
			f = i*filter.fmax/FSTEPS;
#else
			f = n_to_freq(i, filter.fmax);
#endif
			s = std::complex<double>(0.0, (2.0*M_PI)*f);
			r = 1.0/(1.0+s*filter.tau);
			if(filter.fclass == Bandpass) { cout << "How do we have a bandpass filter with an odd pole?\n"; r *= s * filter.tau; }
			if(filter.fclass == Highpass) r *= s * filter.tau;
			filter.freq_resp[i] *= abs(r);
		}
	}

/// Normalise gain
	if(filter.fclass == Bandpass) {
		for(i=0, max_gain=0; i<FSTEPS; i++) {
			if(filter.freq_resp[i] > max_gain) max_gain = filter.freq_resp[i];
		}
		for(i=0; i<FSTEPS; i++) {
			filter.freq_resp[i] /= max_gain;
		}
	}

	bode_file.open("./bode.csv");
	if(bode_file.is_open( )) {
		bode_file << "Frequency, Gain\n";
		for(i=0; i<FSTEPS; i++) {
#if LINEAR_FREQ
			f = i*filter.fmax/FSTEPS;
#else
			f = n_to_freq(i, filter.frequency);
#endif
			bode_file << f << ", " << filter.freq_resp[i] << "\n";
		}
		bode_file.close( );
	}
	filter.log("</bode_calc>");
}



/**
'step_calc' convolves the impulse response of each stage with a step function
so as to derive the step response of the entire filter.
*/
void
TFilter::step_calc(void)
{
//	extern TControl_window *Control_window;
	char logstr[256];
	unsigned i, k, u;
	double ti, w, w0;
	double tt, qq, zeta;
	double a, b;
	double input[TSTEPS], output[TSTEPS], stage_n[TSTEPS];
	double integral, max;
	double k1, dt;
	std::complex<double> s, r, j;
	ofstream step_file;

	log("<step_calc>");
	j = std::complex<double>(0.0, 1.0);

	switch(fclass) {
		case Lowpass: tmax = 5.0/frequency; break;
		case Bandpass: tmax = 5.0/bandwidth; break;
		case Highpass: tmax = 2.0/frequency; break;
		default: cout << "Unknown fclass\n"; exit(-1);
	}
	dt = tmax/TSTEPS;

	sprintf(logstr, "tmax = %f seconds", tmax);
	log(logstr);

//	c = tmax / 5.0;
	/// Start with step function
	for(i=0; i<TSTEPS; i++) input[i] = 1.0, output[i] = 0.0;
	for(k=0; k<(poles/2); k++) {
		/// Calculate constants for stage 'k'
		tt = st[k].T;
		qq = st[k].q;
		a = -1.0 / (2 * qq * tt);
		b = sqrt(1.0 / (tt * tt) - (a * a));

#if VERBOSE
		cout << "T = " << tt << ", q = " << qq << "\n";
		cout << "a = " << a << ", b = " << b << "\n";
#endif

		w0 = 1.0 / tt;
		log("'step_calc' tt = ", tt);
		log("'step_calc' q = ", qq);
//		log("'step_calc' w0 = ", w0);
		k1 = 1.0 / (b*tt);
//		k1 = 1.0 / (b*tt*tt);
		for(i=0; i<TSTEPS; i++) {
			/// Calculate time for point 'i'
			ti = i*dt;		// CHECK THIS
			/// Calculate the impulse response of stage 'k'
			switch(fclass) {
				case Lowpass: stage_n[i] = k1*exp(a * ti) * sin(b * ti) * b; break;
				case Highpass: stage_n[i] = -exp(a * ti) * sin(b * ti) * k1;
				 if(i == 0) stage_n[0] += tt*TSTEPS/tmax; break;
				case Bandpass: stage_n[i] = (sin(b * ti) * a * exp(a * ti) + exp(a * ti) * b * cos(b * ti)) / b; break;
				default: cout << "\nUnknown 'fclass'!\n"; exit(-1);
			}
		}

		/// initialise the output array...
		for(i=0; i<TSTEPS; i++) output[i] = 0.0;

		/// Convolve with input to stage 'k'
		for(i=0; i<TSTEPS; i++) {
			// Calculate time for point 'i'
			ti = i*tmax/TSTEPS;		// CHECK THIS
			for(u=0; u<TSTEPS; u++) {
				if((i+u) < TSTEPS) output[i+u]+=input[i]*stage_n[u]*tmax/TSTEPS;		// CHECK THIS
				else break;
//				if((i+u) < TSTEPS) output[i+u]+=input[i]*stage_n[u];		// CHECK THIS
			}
		}

#ifdef SHOW_STAGES
		log("'step calc' stage[] = ", stage_n, TSTEPS);
		log("'step-calc' input[] = ", input, TSTEPS);
		log("’stepicalc' output[] = ", output, TSTEPS);
#endif

		/// Copy output of stage 'k' to input for stage 'k+1'
		for(i=0; i<TSTEPS; i++) {
			input[i] = output[i];
		}
	}

	/// If order is odd, process first order stage.
	if(poles%2) {
		if(fclass == Bandpass) {
			log("Impossible! Bandpass can't have odd order!");
			cout << "Impossible! Bandpass can't have odd order!\n";
			return;
		}
		tt = -st[k].T;
#if VERBOSE
		cout << "Tau = " << tt << "\n";
#endif
		w0 = 1.0 / tt;
		log("'step_calc' tt = ", tt);
		for(i=0; i<TSTEPS; i++) {
			/// Calculate time for point 'i'
			ti = i*tmax/TSTEPS;		// CHECK THIS
			/// Calculate the impulse response of 1st order stage 'k'
			switch(fclass) {
				case Lowpass: stage_n[i] = exp(-ti/tt); break;
//				case Highpass: stage_n[i] = -exp(-ti/tt)/tt; 
				case Highpass: stage_n[i] = -exp(-ti/tt);
				 if(i == 0) stage_n[0] += tt*TSTEPS/tmax; break;
				case Bandpass: std::cout << "*** ERROR! Band-pass filter cannot have odd order! ***\n"; break;
				default: cout << "\nUnknown 'fclass'!\n"; exit(-1);
			}
		}

		// initialise the output array...
		for(i=0; i<TSTEPS; i++) output[i] = 0.0;

		/// Convolve with input to stage 'k'
		for(i=0; i<TSTEPS; i++) {
			// Calculate time for point 'i'
			ti = i*tmax/TSTEPS;		// CHECK THIS
			for(u=0; u<TSTEPS; u++) {
				if((i+u) < TSTEPS) output[i+u]+=input[i]*stage_n[u]*tmax/TSTEPS;		// CHECK THIS
				else break;
//				if((i+u) < TSTEPS) output[i+u]+=input[i]*stage_n[u];		// CHECK THIS
			}
		}

#ifdef SHOW_STAGES
		log("'step calc' stage[] = ", stage_n, TSTEPS);
		log("'step-calc' input[] = ", input, TSTEPS);
		log("’stepicalc' output[] = ", output, TSTEPS);
#endif

		/// Copy output of stage 'k' to input for stage 'k+1'
		for(i=0; i<TSTEPS; i++) {
			input[i] = output[i];
		}

	}

	/// Adjust gain - I hope this is a temporary frig!
	switch(fclass) {
		case Highpass: for(i=1, max = 0.0; i<TSTEPS; i++) if(fabs(output[i]) > max) max = fabs(output[i]); break;
		default: for(i=0, max = 0.0; i<TSTEPS; i++) if(output[i] > max) max = output[i]; break;
	}
	for(i=0; i<TSTEPS; i++) output[i] /= max;

	/// Write to './step.csv' time and step response
	step_file.open("./step.csv");
	for(i=0, integral=0.0; i<TSTEPS; i++) {
		step_resp[i] = output[i];
		integral += output[i];
		step_file << i*tmax/TSTEPS << ", " << step_resp[i] /* << ", " << integral */ << "\n";
	}
	step_file.close( );
	log("</step_calc>");
}



double scale(TFilter filter)
{
	double max = 0;
	int i;
	for(i = 0; i < filter.poles; i++) {
		if(-(double)filter.st[i].pole.real() > max) max = -(double)filter.st[i].pole.real();
		if((double)filter.st[i].pole.imag() > max) max = (double)filter.st[i].pole.imag();
	}
	return((max * 4.0) / 3.0);
}



void TFilter::Calculate(void)
{
	char str[256];
	int i;
	log("<Calculate>");

/// Create prototype filter.
	switch(sclass) {
		case Butterworth: butterworth( ); break;
		case Bessel: bessel( ); sort( ); break;
		case Chebyshev: chebyshev( ); break;
		default: cout << "Unknown filter!" << "\n"; return;
	}
	/// Adjust prototype filter so attenuation is x dB at 1 Hertz.
	std::cout << "***** Prototype filter *****\n";
	for(i=0; i<poles; i++) {
		std::cout << "Pole: " << st[i].pole << "\n";
		sprintf(str, "Pole: %f + i.%f", st[i].pole.real( ), st[i].pole.imag( ));
		log(str);
	}
	std::cout << "****************************\n";
/// Calculate T and q values from poles.
	ts_and_qs( );
#ifdef NORMALISE
	Proto_normalise( );
#endif
/// Transform prototype to desired filter class (e.g. high-pass etc.).
	transform( );
	ts_and_qs( );
	show_filter( );
	log("</Calculate>");
}



void TFilter::log(string s)
{
#if LOG
	ofstream logfile;
	logfile.open("./rbfilter.log", ios::app);
	logfile << s << "\n";
	logfile.close( );
#endif
}



void TFilter::log(string s, double d)
{
	ostringstream s1;
	std::string s2;
	s1 << s << d;
	s2 = s1.str( );
	log(s2);
}




void TFilter::log(string s, double *pd)
{
	ostringstream s1;
	std::string s2;
	s1 << s << *pd;
	s2 = s1.str( );
	log(s2);
}



/// Log the array pd but no more than 100 elements of it.
void TFilter::log(string s, double *pd, int n)
{
	int i;
#if LOG
	char s1[128];
	n = n>100? 100: n;
	for(i=0; i<(n-1); i++) {
		sprintf(s1, "%s %f", s.c_str( ), *(pd+i));
		log(s1);
	}
	log("");
#endif
}




void TFilter::log(string s, std::complex<double> c)
{
	ostringstream s1;
	std::string s2;
	s1 << s << c;
	s2 = s1.str( );
	log(s2);
}




void TFilter::print_T_q(void)
{
//	cout << s << "\n";
}



/// 'transform( )' transforms the prototype filter to the desired type (e.g. bandpass) and frequency.
void TFilter::transform(void)
{
char t, type;
int i;
double p[10], z[10];
ifstream ifile;
ofstream ofile;

	type = TFilter::type;
	log("<transform>");
	if(type != 'P') {
		cout << "\nFile has already been transformed.\n";
		return;
	}
	/// 220; Transform to the chosen type.
	do {
		switch(fclass) {
			case Lowpass:
			 lowpass( );
			 type = 'L';
			 for(i=0; i<poles; i++) st[i].pole *= 2.0 * M_PI;
			 break;
			case Highpass:
			 highpass( );
			 type = 'H';
//			 for(i=0; i<poles; i++) st[i].pole *= 2.0 * M_PI;
			 break;
			case Bandpass:
			 bandpass( );
			 type = 'B';
			 for(i=0; i<poles; i++) st[i].pole *= 2.0 * M_PI;
			 break;
			default: cout << "Unknown filter class\n"; continue;
		}
		break;
	} while(true);
	/// 330; Rewrite the parameter file.
	ofile.open("pfile.z");
	if(!ofile) {
		cout << "\nUnable to open 'pfile.z'\n";
//		exit(0);
	}
	ofile << type << " " << poles << " " << zeroes << "\n";
	for(i=0; i<poles; i++) ofile << st[i].pole << " ";
	if(zeroes) {
		for(i=0; i<zeroes; i++) ofile << st[i].zero << "\n";
	}
	ofile.close();

//	ts_and_qs(filter);

	log("</transform>");
}



/** Transform to highpass 
Keep the q values unaltered but take the reciprocal of the magnitude; adjust the magnitudes for
frequency; add zeroes at the origin.
*/
void
TFilter::highpass(void)
{
int i, type;
double dsq, d, ratio;
double a1, w1;

	log("<highpass>");
	for(i=0; i<poles; i++) {
		std::cout << "From " << st[i].pole;
		dsq = st[i].pole.real( ) * st[i].pole.real( ) + st[i].pole.imag( ) *st[i].pole.imag( );
		d = sqrt(dsq);
		std::cout << " (Magnitude " << d << ") ";
		ratio = st[i].pole.imag( ) / st[i].pole.real( );
		if(st[i].pole.imag( ) == 0.0) {
			w1 = 0.0;
			a1 = 1.0 / st[i].pole.real( );
		} else {
			d = 1.0 / d;
			dsq = 1.0 / dsq;
			w1 = st[i].pole.imag( ) * dsq;
			a1 = st[i].pole.real( ) * dsq;
		}
		st[i].pole = complex<double>(a1, w1);
		st[i].zero = complex<double>(0.0, 0.0);
		st[i].pole *= 2.0 * M_PI * frequency;
//		st[i].zero *= 2.0 * M_PI * frequency;
		std::cout << " to " << st[i].pole << "\n";
	}

	fclass = Highpass;
	zeroes = poles;
	type='H';

	log("</highpass>");
}



/// Transform the filter to bandpass using the Geffe algotithm.
void TFilter::bandpass(void)
{
int i, j;
double f, w1, a1, d, bw, w0, b1, a2, b2;
ofstream ofile;
TFilter filter1;

	log("<bandpass>");
/// Transformation to bandpass doubles the number of poles!
//	if(poles % 2) log("Odd order! Impossible");

	for(i=0, j=0; i<poles; i++) {
		if(st[i].pole.imag( ) < -0.0001) continue;
		geffe(st[i].pole.real( ), st[i].pole.imag( ), frequency, bandwidth, a1, b1, a2, b2);
#if VERBOSE
		cout << i << ": " << a1 << ", +-" << b1 << " \\ " << a2 << ", +/-" << b2 << "\n";
#endif
		filter1.st[j++].pole = complex<double>(a1, b1);
		filter1.st[j++].pole = complex<double>(a1, -b1);
		if(i != poles - 1) {
			filter1.st[j++].pole = complex<double>(a2, b2);
			filter1.st[j++].pole = complex<double>(a2, -b2);
			ofile << -a1 << " " << b1 << " " << -a2 << " " << b2 << "\n";
		} else ofile << -a1 << " " << b1 << "\n";
	}
	zeroes = poles;
	filter1.poles = 2 * poles;
	type = 'B';
	filter1.type = type;
/*	if(n%2)
	 ofile << p[n]*bw/2.0 << " " << sqrt(w0*w0-p[n]*p[n]*bw*bw/4.0) << "\n";
	ofile.close();
*/

	for(i=0; i<filter1.poles; i++) st[i].pole = filter1.st[i].pole;
	poles = filter1.poles;

	fclass = Bandpass;

	log("</bandpass>");
}



double arccos(double x)
{
	return(2.0 * atan(sqrt(1.0 - x*x) / (1.0 + x)));
}



/**
'geffe' is an implementation of the "Geffe" algorithm which transforms a low pass prototype filter into a bandpass filter. The "Geffe" algorithm is described in "Analogue Filter Design" by M. E. van Valkenburg.
*/
void
geffe(double sigma, double omega, double w0, double bw,
 double &a1, double &b1, double &a2, double &b2)
{
double c, d, e, g, q, k, w, w01, w02, qc, a, b;
double q1, q2;		// Used for checking.

	filter.log("<geffe>");

	qc=w0/bw;
	c=sigma*sigma+omega*omega;
	d=-2.0*sigma/qc;
	e=4.0+c/(qc*qc);
	g=sqrt(e*e-4.0*d*d);
	q=sqrt((e+g)/2.0)/d;
	k=sigma*q/qc;
	// As originally written, 'w' would often be evaluated as 'nan'. This is intended to avoid that problem.
	if(fabs((k*k-1.0) < 0.0001)) w = k;
	else w=k+sqrt(k*k-1.0);
	w01=w*w0;
	w02=w0/w;
	a1=w01/(2.0*q);
	a2=w02/(2.0*q);
	a=1.0/(2.0*q);
#if 1
	b=sin(atan(sqrt((1.0-a*a)/(a*a))));
	b1=w01*b;
	b2=w02*b;
#else
	b1 = -w01*sin(arccos(1.0 / (2.0 * q)));
	b2 = -w02*sin(arccos(1.0 / (2.0 * q)));
#endif
	// Check:
	q1 = (a1*a1+b1*b1)/(2.0*a1);
	q2 = (a2*a2+b2*b2)/(2.0*a2);
	cout << "q=" << q1 << " & " << q2 << "\n";

	filter.log("</geffe>");
}




