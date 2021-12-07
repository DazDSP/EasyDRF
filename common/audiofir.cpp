/******************************************************************************\
 * Copyright (c) 2004
 *
 * Author(s):
 *	Francesco Lanza
 *
 * Description:
 *	
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "audiofir.h"

 
float coeff[audfiltlen] = 
{
-0.000242f, -0.000141f, -0.000469f, 0.000029f, -0.000463f, 0.000057f, 
-0.000170f, -0.000047f, 0.000281f, -0.000128f, 0.000627f, -0.000002f, 
0.000636f, 0.000363f, 0.000292f, 0.000748f, -0.000158f, 0.000778f, 
-0.000380f, 0.000218f, -0.000246f, -0.000752f, 0.000011f, -0.001543f, 
-0.000036f, -0.001545f, -0.000582f, -0.000627f, -0.001260f, 0.000653f, 
-0.001303f, 0.001424f, -0.000176f, 0.001215f, 0.001781f, 0.000411f, 
0.003339f, -0.000037f, 0.003184f, 0.000490f, 0.001017f, 0.001520f, 
-0.001978f, 0.001681f, -0.003863f, -0.000116f, -0.003436f, -0.003400f, 
-0.001278f, -0.005995f, 0.000657f, -0.005452f, 0.000741f, -0.001138f, 
-0.000708f, 0.004780f, -0.001465f, 0.008534f, 0.000643f, 0.007527f, 
0.005332f, 0.002514f, 0.009239f, -0.002776f, 0.008121f, -0.004602f, 
0.000517f, -0.002470f, -0.010095f, 0.000196f, -0.016927f, -0.000982f, 
-0.014647f, -0.006880f, -0.003834f, -0.012792f, 0.008723f, -0.011252f, 
0.014777f, 0.001622f, 0.011186f, 0.021075f, 0.002949f, 0.034709f, 
-0.000487f, 0.030180f, 0.006281f, 0.004849f, 0.017664f, -0.029835f, 
0.017867f, -0.053570f, -0.009522f, -0.049715f, -0.068813f, -0.017016f, 
-0.145364f, 0.027852f, -0.210443f, 0.059539f, 0.764000f, 0.059539f, 
-0.210443f, 0.027852f, -0.145364f, -0.017016f, -0.068813f, -0.049715f, 
-0.009522f, -0.053570f, 0.017867f, -0.029835f, 0.017664f, 0.004849f, 
0.006281f, 0.030180f, -0.000487f, 0.034709f, 0.002949f, 0.021075f, 
0.011186f, 0.001622f, 0.014777f, -0.011252f, 0.008723f, -0.012792f, 
-0.003834f, -0.006880f, -0.014647f, -0.000982f, -0.016927f, 0.000196f, 
-0.010095f, -0.002470f, 0.000517f, -0.004602f, 0.008121f, -0.002776f, 
0.009239f, 0.002514f, 0.005332f, 0.007527f, 0.000643f, 0.008534f, 
-0.001465f, 0.004780f, -0.000708f, -0.001138f, 0.000741f, -0.005452f, 
0.000657f, -0.005995f, -0.001278f, -0.003400f, -0.003436f, -0.000116f, 
-0.003863f, 0.001681f, -0.001978f, 0.001520f, 0.001017f, 0.000490f, 
0.003184f, -0.000037f, 0.003339f, 0.000411f, 0.001781f, 0.001215f, 
-0.000176f, 0.001424f, -0.001303f, 0.000653f, -0.001260f, -0.000627f, 
-0.000582f, -0.001545f, -0.000036f, -0.001543f, 0.000011f, -0.000752f, 
-0.000246f, 0.000218f, -0.000380f, 0.000778f, -0.000158f, 0.000748f, 
0.000292f, 0.000363f, 0.000636f, -0.000002f, 0.000627f, -0.000128f, 
0.000281f, -0.000047f, -0.000170f, 0.000057f, -0.000463f, 0.000029f, 
-0.000469f, -0.000141f, -0.000242f
};


float firbufRX[audfiltlen] = {0.0};
float firresRX;

bool useRXfilt = false;

float AudFirRX (float input)
{
	int i = 0; //init DM
	if (!useRXfilt) return input;
	for (i=0;i<audfiltlen-1;i++)
		firbufRX[i] = firbufRX[i+1];
	firbufRX[audfiltlen-1] = input;
		
	firresRX = 0.0;
	for (i=0;i<audfiltlen;i++)
	{
		firresRX += firbufRX[i] * coeff[i];
	}
	return firresRX;
}

void setRXfilt(bool enable)
{
	useRXfilt = enable;
}

