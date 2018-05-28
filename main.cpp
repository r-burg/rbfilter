/******************************************************************************

                                main.cpp

                Copyright (c) Roger Burghall 2014..2017

******************************************************************************/


//*********************************************************

#include "math.h"
#include <complex.h>
#include "calcs.h"
#include "SandK.h"
#include "preferred.h"
#include <ostream>
#include <iostream>
#include <string>

#if SHOW_FILE_COMPILING
#pragma message "Compiling " __FILE__
#endif

using std::cout;

extern TFilter filter;

void SallenAndKey1(double Tau, double q);

int main(int argc, char **argv)
{
    cout << nearest(e_12, 1025.4) << "\n";
    cout << nearest(e_12, 1175.4) << "\n";
    filter.poles = 5;
    filter.fmax = 10.0;
    butterworth(filter);
    lowpass(filter, 10.0);
    bode_calc(filter);
    cout << "\n";
    bessel(filter);
    cout << "\n";
    chebyshev(filter);
    cout << "\n";
    bode_calc(filter);
    
    SandK cct1;
    cct1.SallenAndKey(t_lowpass, 1.0, 1.0e4, 1.0e4, 0.0, 1.0e-7, 2.0e-7);
    cct1.SallenAndKey1(0.001, 0.707);
}



