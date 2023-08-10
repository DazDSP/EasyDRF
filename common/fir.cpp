#include "Modul.h"
#include <stdio.h>
#include "fir.h"
#include "parameter.h"

#define firlen 81
#define maxsamples 2000
 
double coeff[zffiltlen] = 
	{
		0.000231, 0.000685, 0.000548, -0.000503, -0.001665, -0.001526, 
		0.000341, 0.002406, 0.002471, 0.000279, -0.001810, -0.001620, 
		-0.000195, -0.000547, -0.002866, -0.002861, 0.002832, 0.010087, 
		0.009575, -0.002028, -0.015127, -0.015745, -0.001943, 0.012217, 
		0.012665, 0.002855, -0.000922, 0.006865, 0.009597, -0.010406, 
		-0.040729, -0.041605, 0.008812, 0.075215, 0.087218, 0.013115, 
		-0.092208, -0.128373, -0.049219, 0.081615, 0.145000, 0.081615, 
		-0.049219, -0.128373, -0.092208, 0.013115, 0.087218, 0.075215, 
		0.008812, -0.041605, -0.040729, -0.010406, 0.009597, 0.006865, 
		-0.000922, 0.002855, 0.012665, 0.012217, -0.001943, -0.015745, 
		-0.015127, -0.002028, 0.009575, 0.010087, 0.002832, -0.002861, 
		-0.002866, -0.000547, -0.000195, -0.001620, -0.001810, 0.000279, 
		0.002471, 0.002406, 0.000341, -0.001526, -0.001665, -0.000503, 
		0.000548, 0.000685, 0.000231 
	};


int no_dft = 0;

FILE * cfile;

void FirInit(int dftsize,ESpecOcc spec)
{
	int i;
	no_dft = dftsize;

	//for (i=0;i<zffiltlen;i++) //This did nothing except waste time! DM
	//	coeff[i] = coeff[i];
}

void DoFir (_COMPLEX * samples,_COMPLEX * outsamples)
{
	int i = 0,k; //init DM
	_COMPLEX firres;

	for (k=0;k<no_dft;k++)
	{
		firres = 0.0;
		for (i=0;i<zffiltlen;i++)
		{
			firres += *(samples+i+k) * coeff[i];
		}
		*(outsamples+k) = firres;
	}
}

void NoFir (_COMPLEX * samples,_COMPLEX * outsamples)
{
	int k;
	_COMPLEX firres;
	for (k=0;k<no_dft;k++)
	{
		firres = *(samples+zffiltleadlen+k);
		*(outsamples+k) = firres;
	}
}
