
#include "quadrantscan.h"

void QuadrantScan(int** order, int hmin, int vmin, 
		 int hmax, int vmax, int size, int &current)
{

	int hmintemp, vmintemp, hmaxtemp, vmaxtemp;

	if (size == 1) // 1x1 array
		if(order[vmin][hmin] != 0)
		{
			order[vmin][hmin] = current;  // store order seen
			current++;
		}
		else
			; // don't store if marked out
	else{
		size /= 2;

		hmintemp = hmin;
		vmintemp = vmin;
		hmaxtemp = hmax - size;
		vmaxtemp = vmax - size;
		QuadrantScan(order, hmintemp, vmintemp,
			    hmaxtemp, vmaxtemp, size, current);

		hmintemp = hmin;
		vmintemp = vmin + size;
		hmaxtemp = hmax - size;
		vmaxtemp = vmax;
		QuadrantScan(order, hmintemp, vmintemp,
			    hmaxtemp, vmaxtemp, size, current);

		hmintemp = hmin + size;
		vmintemp = vmin + size;
		hmaxtemp = hmax;
		vmaxtemp = vmax;
		QuadrantScan(order, hmintemp, vmintemp,
			    hmaxtemp, vmaxtemp, size, current);

		hmintemp = hmin + size;
		vmintemp = vmin;
		hmaxtemp = hmax;
		vmaxtemp = vmax - size;
		QuadrantScan(order, hmintemp, vmintemp,
			    hmaxtemp, vmaxtemp, size, current);

	}

	return;
}
/*
8x8
1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1
0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0


8x8
1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1
1 2 3 4 5 6 7 8
9 10 11 12 13 14 15 16
17 18 19 20 21 22 23 24
25 26 27 28 29 30 31 32*/
