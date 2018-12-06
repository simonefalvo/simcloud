#include "rvgs.h"
#include "rngs.h"
#include "basic.h"


double GetArrival(unsigned int *j)
{
	const double mean[2] = {1/L1, 1/L2};	
	static double arrival[2] = {START, START};
	static int init = 1;
	double temp;
	
	if (init) {
		SelectStream(0);
		arrival[0] += Exponential(mean[0]);
		SelectStream(1);
		arrival[1] += Exponential(mean[1]);
		init=0;
	}

	if (arrival[0] <= arrival[1])
		*j = 0;
	else
		*j = 1;

	temp = arrival[*j];
	SelectStream(*j);
	arrival[*j] += Exponential(mean[*j]);

	return temp;
}



double GetService(int j, int n)
{
	const double mean[5] = {1/M1CLET, 1/M2CLET,
                            1/M1CLOUD, 1/M2CLOUD,
                            1/MSETUP};	
    SelectStream(j + n + 2);
    return Exponential(mean[j + n]);
}

