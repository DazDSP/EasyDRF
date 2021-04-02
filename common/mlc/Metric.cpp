/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * The metric is calculated as follows:
 * M = ||r - s * h||^2 = ||h||^2 * ||r / h - s||^2
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

#include "Metric.h"


/* Implementation *************************************************************/
void CMLCMetric::CalculateMetric(CVector<CEquSig>* pcInSymb, 
								 CVector<CDistance>& vecMetric, 
								 CVector<_BINARY>& vecbiSubsetDef1, 
								 CVector<_BINARY>& vecbiSubsetDef2,
								 CVector<_BINARY>& vecbiSubsetDef3, 
								 CVector<_BINARY>& vecbiSubsetDef4,
								 CVector<_BINARY>& vecbiSubsetDef5,
								 CVector<_BINARY>& vecbiSubsetDef6,
								 int iLevel, _BOOLEAN bIteration)
{
	int i, k;
	int iTabInd0;

	switch (eMapType)
	{
	case CParameter::CS_1_SM:
		/**********************************************************************/
		/* 4QAM	***************************************************************/
		/**********************************************************************/
		/* Metric according DRM-standard: (i_0  q_0) = (y_0,0  y_0,1) */
		for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
		{
			/* Real part ---------------------------------------------------- */
			/* Distance to "0" */
			vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
				rTableQAM4[0][0], (*pcInSymb)[i].rChan);

			/* Distance to "1" */
			vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(), 
				rTableQAM4[1][0], (*pcInSymb)[i].rChan);


			/* Imaginary part ----------------------------------------------- */
			/* Distance to "0" */
			vecMetric[k + 1].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(), 
				rTableQAM4[0][1], (*pcInSymb)[i].rChan);

			/* Distance to "1" */
			vecMetric[k + 1].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(), 
				rTableQAM4[1][1], (*pcInSymb)[i].rChan);
		}

		break;

	case CParameter::CS_2_SM:
		/**********************************************************************/
		/* 16QAM **************************************************************/
		/**********************************************************************/
		/* (i_0  i_1  q_0  q_1) = (y_0,0  y_1,0  y_0,1  y_1,1) */
		switch (iLevel)
		{
		case 0:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef2" */
					iTabInd0 = vecbiSubsetDef2[k] & 1;

					/* Distance to "0" */
					vecMetric[k].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM16[iTabInd0][0], (*pcInSymb)[i].rChan);

					/* Distance to "1" */
					vecMetric[k].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM16[iTabInd0 | (1 << 1)][0],
						(*pcInSymb)[i].rChan);


					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef2" */
					iTabInd0 = vecbiSubsetDef2[k + 1] & 1;

					/* Distance to "0" */
					vecMetric[k + 1].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM16[iTabInd0][1], (*pcInSymb)[i].rChan);

					/* Distance to "1" */
					vecMetric[k + 1].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM16[iTabInd0 | (1 << 1)][1],
						(*pcInSymb)[i].rChan);
				}
				else
				{
					/* There are two possible points for each bit. Both have to
					   be used. In the first step: {i_1}, {q_1} = 0
					   In the second step: {i_1}, {q_1} = 1 */
						
					/* Calculate distances */
					/* Real part */
					vecMetric[k].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM16[0 /* [0  0] */][0],
						rTableQAM16[1 /* [0  1] */][0], (*pcInSymb)[i].rChan);

					vecMetric[k].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.real(),
						rTableQAM16[2 /* [1  0] */][0],
						rTableQAM16[3 /* [1  1] */][0], (*pcInSymb)[i].rChan);

					/* Imaginary part */
					vecMetric[k + 1].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.imag(),
						rTableQAM16[0 /* [0  0] */][1],
						rTableQAM16[1 /* [0  1] */][1], (*pcInSymb)[i].rChan);

					vecMetric[k + 1].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.imag(), 
						rTableQAM16[2 /* [1  0] */][1],
						rTableQAM16[3 /* [1  1] */][1], (*pcInSymb)[i].rChan);
				}
			}

			break;

		case 1:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				/* Real part ------------------------------------------------ */
				/* Higest bit defined by "vecbiSubsetDef1" */
				iTabInd0 = ((vecbiSubsetDef1[k] & 1) << 1);

				/* Distance to "0" */
				vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
					rTableQAM16[iTabInd0][0], (*pcInSymb)[i].rChan);

				/* Distance to "1" */
				vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
					rTableQAM16[iTabInd0 | 1][0], (*pcInSymb)[i].rChan);


				/* Imaginary part ------------------------------------------- */
				/* Higest bit defined by "vecbiSubsetDef1" */
				iTabInd0 = ((vecbiSubsetDef1[k + 1] & 1) << 1);

				/* Distance to "0" */
				vecMetric[k + 1].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(), 
					rTableQAM16[iTabInd0][1], (*pcInSymb)[i].rChan);

				/* Distance to "1" */
				vecMetric[k + 1].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM16[iTabInd0 | 1][1], (*pcInSymb)[i].rChan);
			}

			break;
		}

		break;

	case CParameter::CS_3_SM:
		/**********************************************************************/
		/* 64QAM SM ***********************************************************/
		/**********************************************************************/
		/* (i_0  i_1  i_2  q_0  q_1  q_2) = 
		   (y_0,0  y_1,0  y_2,0  y_0,1  y_1,1  y_2,1) */
		switch (iLevel)
		{
		case 0:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3" next bit defined
					   by "vecbiSubsetDef2" */
					iTabInd0 = 
						(vecbiSubsetDef3[k] & 1) | 
						((vecbiSubsetDef2[k] & 1) << 1);

					vecMetric[k].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[iTabInd0][0],	(*pcInSymb)[i].rChan);

					vecMetric[k].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM64SM[iTabInd0 | (1 << 2)][0],
						(*pcInSymb)[i].rChan);


					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3" next bit defined
					   by "vecbiSubsetDef2" */
					iTabInd0 = 
						(vecbiSubsetDef3[k + 1] & 1) | 
						((vecbiSubsetDef2[k + 1] & 1) << 1);

					/* Calculate distances, imaginary part */
					vecMetric[k + 1].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0][1],	(*pcInSymb)[i].rChan);

					vecMetric[k + 1].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0 | (1 << 2)][1],
						(*pcInSymb)[i].rChan);
				}
				else
				{
					/* Real part -------------------------------------------- */
					vecMetric[k].rTow0 = 
						Minimum4((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[0 /* [0 0 0] */][0],
						rTableQAM64SM[1 /* [0 0 1] */][0],
						rTableQAM64SM[2 /* [0 1 0] */][0],
						rTableQAM64SM[3 /* [0 1 1] */][0],
						(*pcInSymb)[i].rChan);

					vecMetric[k].rTow1 = 
						Minimum4((*pcInSymb)[i].cSig.real(),
						rTableQAM64SM[4 /* [1 0 0] */][0], 
						rTableQAM64SM[5 /* [1 0 1] */][0], 
						rTableQAM64SM[6 /* [1 1 0] */][0], 
						rTableQAM64SM[7 /* [1 1 1] */][0],
						(*pcInSymb)[i].rChan);


					/* Imaginary part --------------------------------------- */
					vecMetric[k + 1].rTow0 = 
						Minimum4((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[0 /* [0 0 0] */][1],
						rTableQAM64SM[1 /* [0 0 1] */][1],
						rTableQAM64SM[2 /* [0 1 0] */][1],
						rTableQAM64SM[3 /* [0 1 1] */][1],
						(*pcInSymb)[i].rChan);

					vecMetric[k + 1].rTow1 = 
						Minimum4((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[4 /* [1 0 0] */][1], 
						rTableQAM64SM[5 /* [1 0 1] */][1], 
						rTableQAM64SM[6 /* [1 1 0] */][1], 
						rTableQAM64SM[7 /* [1 1 1] */][1],
						(*pcInSymb)[i].rChan);
				}
			}

			break;

		case 1:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3",highest defined
					   by "vecbiSubsetDef1" */
					iTabInd0 = 
						((vecbiSubsetDef1[k] & 1) << 2) | 
						(vecbiSubsetDef3[k] & 1);

					vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[iTabInd0][0],	(*pcInSymb)[i].rChan);

					vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM64SM[iTabInd0 | (1 << 1)][0],
						(*pcInSymb)[i].rChan);


					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3",highest defined
					   by "vecbiSubsetDef1" */
					iTabInd0 = 
						((vecbiSubsetDef1[k + 1] & 1) << 2) | 
						(vecbiSubsetDef3[k + 1] & 1);

					vecMetric[k + 1].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0][1],
						(*pcInSymb)[i].rChan);

					vecMetric[k + 1].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0 | (1 << 1)][1],
						(*pcInSymb)[i].rChan);
				}
				else
				{
					/* There are two possible points for each bit. Both have to
					   be used. In the first step: {i_2} = 0, Higest bit 
					   defined by "vecbiSubsetDef1" */

					/* Real part -------------------------------------------- */
					iTabInd0 = ((vecbiSubsetDef1[k] & 1) << 2);
					vecMetric[k].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[iTabInd0][0],
						rTableQAM64SM[iTabInd0 | 1][0], (*pcInSymb)[i].rChan);

					iTabInd0 = ((vecbiSubsetDef1[k] & 1) << 2) | (1 << 1);
					vecMetric[k].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[iTabInd0][0],
						rTableQAM64SM[iTabInd0 | 1][0], (*pcInSymb)[i].rChan);


					/* Imaginary part --------------------------------------- */
					iTabInd0 = ((vecbiSubsetDef1[k + 1] & 1) << 2);
					vecMetric[k + 1].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.imag(), 
						rTableQAM64SM[iTabInd0][1],
						rTableQAM64SM[iTabInd0 | 1][1], (*pcInSymb)[i].rChan);

					iTabInd0 = ((vecbiSubsetDef1[k + 1] & 1) << 2) | (1 << 1);
					vecMetric[k + 1].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0][1],
						rTableQAM64SM[iTabInd0 | 1][1], (*pcInSymb)[i].rChan);
				}
			}

			break;

		case 2:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				/* Real part ------------------------------------------------ */
				/* Higest bit defined by "vecbiSubsetDef1" next bit defined
				   by "vecbiSubsetDef2" */
				iTabInd0 = 
					((vecbiSubsetDef1[k] & 1) << 2) | 
					((vecbiSubsetDef2[k] & 1) << 1);

				vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
					rTableQAM64SM[iTabInd0][0], (*pcInSymb)[i].rChan);

				vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
					rTableQAM64SM[iTabInd0 | 1][0], (*pcInSymb)[i].rChan);


				/* Imaginary part ------------------------------------------- */
				/* Higest bit defined by "vecbiSubsetDef1" next bit defined
				   by "vecbiSubsetDef2" */
				iTabInd0 = 
					((vecbiSubsetDef1[k + 1] & 1) << 2) | 
					((vecbiSubsetDef2[k + 1] & 1) << 1);

				/* Calculate distances, imaginary part */
				vecMetric[k + 1].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM64SM[iTabInd0][1], (*pcInSymb)[i].rChan);

				vecMetric[k + 1].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM64SM[iTabInd0 | 1][1], (*pcInSymb)[i].rChan);
			}

			break;
		}
		break;
	}
}

void CMLCMetric::Init(int iNewInputBlockSize, CParameter::ECodScheme eNewCodingScheme)
{
	iInputBlockSize = iNewInputBlockSize;
	eMapType = eNewCodingScheme;
}
