/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	
 *	Note: We always shift the bits towards the MSB 
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

#include "ConvEncoder.h"


/* Implementation *************************************************************/
int CConvEncoder::Encode(CVector<_BINARY>& vecInputData, 
						 CVector<_BINARY>& vecOutputData)
{
	int		iOutputCounter = 0; //inits DM
//	int		iCurPunctPattern; //unused? DM
	_BYTE	byStateShiftReg = 0;

	/* Set output size to zero, increment it each time a new bit is encoded */
	iOutputCounter = 0;

	/* Reset counter for puncturing and state-register */
	byStateShiftReg = 0;

	for (int i = 0; i < iNumInBitsWithMemory; i++)
	{
		/* Update shift-register (state information) ------------------------ */
		/* Shift bits in state-shift-register */
		byStateShiftReg <<= 1;

		/* Tailbits are calculated in this loop. Check when end of vector is
		   reached and no more bits must be added */
		if (i < iNumInBits)
		{
			/* Add new bit at the beginning */
			if (vecInputData[i] == TRUE)
				byStateShiftReg |= 1;
		}


		/* Puncturing ------------------------------------------------------- */
		/* Depending on the current puncturing pattern, different numbers of
		   output bits are generated. The state shift register "byStateShiftReg"
		   is convoluted with the respective patterns for this bit (is done
		   inside the convolution function) */
		switch (veciTablePuncPat[i])
		{
		case PP_TYPE_0001:
			/* Pattern 0001 */
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 0);
			break;

		case PP_TYPE_0101:
			/* Pattern 0101 */
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 0);
	
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 2);
			break;

		case PP_TYPE_0011:
			/* Pattern 0011 */
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 0);
	
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 1);
			break;

		case PP_TYPE_0111:
			/* Pattern 0111 */
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 0);
	
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 1);

			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 2);
			break;

		case PP_TYPE_1111:
			/* Pattern 1111 */
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 0);
	
			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 1);

			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 2);

			vecOutputData[iOutputCounter++] =
				Convolution(byStateShiftReg, 3);
			break;
		}
	}

	/* Return number of encoded bits */
	return iOutputCounter;
}

void CConvEncoder::Init(CParameter::ECodScheme eNewCodingScheme,
						CParameter::EChanType eNewChannelType, int iN1, 
						int iN2, int iNewNumInBitsPartA,
						int iNewNumInBitsPartB, int iPunctPatPartA,
						int iPunctPatPartB, int iLevel)
{
	/* Number of bits out is the sum of all protection levels */
	iNumInBits = iNewNumInBitsPartA + iNewNumInBitsPartB;

	/* Number of out bits including the state memory */
	iNumInBitsWithMemory = iNumInBits + MC_CONSTRAINT_LENGTH - 1;

	/* Init vector, storing table for puncturing pattern and generate pattern */
	veciTablePuncPat.Init(iNumInBitsWithMemory);

	veciTablePuncPat = GenPuncPatTable(eNewCodingScheme, eNewChannelType, iN1,
		iN2, iNewNumInBitsPartA, iNewNumInBitsPartB, iPunctPatPartA,
		iPunctPatPartB, iLevel);
}
