/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#include "QAMMapping.h"


/* Implementation *************************************************************/
void CQAMMapping::Map(CVector<_BINARY>& vecbiInputData1, 
					  CVector<_BINARY>& vecbiInputData2, 
					  CVector<_BINARY>& vecbiInputData3, 
					  CVector<_BINARY>& vecbiInputData4, 
					  CVector<_BINARY>& vecbiInputData5, 
					  CVector<_BINARY>& vecbiInputData6, 
					  CVector<_COMPLEX>* pcOutputData)
{
/* 
	We always use "& 1" when we combine binary values with logical operators
	for safety reasons.
*/
	int	i = 0; //inits DM
	int	iIndexReal = 0;
	int	iIndexImag = 0;

	switch (eMapType)
	{
	case CParameter::CS_1_SM:
		/* 4QAM ------------------------------------------------------------- */
		/* Mapping according DRM-standard: 
		   {i_0  q_0} = (y'_0  y'_1) = (y_0,0  y_0,1) */
		for (i = 0; i < iOutputBlockSize; i++)
		{
			(*pcOutputData)[i] = 
						 /* Odd entries (second column in "rTableQAM4") */
				_COMPLEX(rTableQAM4[vecbiInputData1[2 * i]][0],
			             /* Even entries in input-vector */
						 rTableQAM4[vecbiInputData1[2 * i + 1]][1]);
		}

		break;

	case CParameter::CS_2_SM:
		/* 16QAM ------------------------------------------------------------ */
		/* Mapping according DRM-standard: 
		   {i_0  i_1  q_0  q_1} = (y_0,0  y_1,0  y_0,1  y_1,1) */
		for (i = 0; i < iOutputBlockSize; i++)
		{
			const int i2i = 2 * i;
			const int i2ip1 = 2 * i + 1;

			/* Filling indices [y_0,0, y_1,0]. Incoming bits are shifted to 
			   their desired positions in the integer variables "iIndexImag" 
			   and "iIndexReal" and combined */
			iIndexReal = ((vecbiInputData1[i2i] & 1) << 1) | 
						  (vecbiInputData2[i2i] & 1);
			iIndexImag = ((vecbiInputData1[i2ip1] & 1) << 1) | 
						  (vecbiInputData2[i2ip1] & 1);
	
			(*pcOutputData)[i] = 
						 /* Odd entries (second column in "rTableQAM16") */
				_COMPLEX(rTableQAM16[iIndexReal][0],
			             /* Even entries in input-vector */
						 rTableQAM16[iIndexImag][1]);
		}

		break;

	case CParameter::CS_3_SM:
		/* 64QAM SM --------------------------------------------------------- */
		/* Mapping according DRM-standard: {i_0  i_1  i_2  q_0  q_1  q_2} = 
		   (y_0,0  y_1,0  y_2,0  y_0,1  y_1,1  y_2,1) */
		for (i = 0; i < iOutputBlockSize; i++)
		{
			const int i2i = 2 * i;
			const int i2ip1 = 2 * i + 1;

			/* Filling indices [y_0,0, y_1,0, y_2,0]. Incoming bits 
			   are shifted to their desired positions in the integer variables 
			   "iIndexImag" and "iIndexReal" and combined */
			iIndexReal = ((vecbiInputData1[i2i] & 1) << 2) |
						 ((vecbiInputData2[i2i] & 1) << 1) | 
						  (vecbiInputData3[i2i] & 1);
			iIndexImag = ((vecbiInputData1[i2ip1] & 1) << 2) |
						 ((vecbiInputData2[i2ip1] & 1) << 1) | 
						  (vecbiInputData3[i2ip1] & 1);
	
			(*pcOutputData)[i] = 
						 /* Odd entries (second column in "rTableQAM64SM") */
				_COMPLEX(rTableQAM64SM[iIndexReal][0],
			             /* Even entries in input-vector */
						 rTableQAM64SM[iIndexImag][1]);
		}

		break;
	}
}

void CQAMMapping::Init(int iNewOutputBlockSize, 
					   CParameter::ECodScheme eNewCodingScheme)
{
	/* Set the two internal parameters */
	iOutputBlockSize = iNewOutputBlockSize;
	eMapType = eNewCodingScheme;
}
