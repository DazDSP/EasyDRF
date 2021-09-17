/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Multi-level-channel (de)coder (MLC)
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

#include "MLC.h"
#include "../Tables/TableCarrier.h"


/* Implementation *************************************************************/
/******************************************************************************\
* MLC-encoder                                                                  *
\******************************************************************************/
void CMLCEncoder::ProcessDataInternal(CParameter& Parameter)
{
	int	i = 0, j = 0; //init DM
	int iElementCounter = 0; //init DM

	/* Energy dispersal ----------------------------------------------------- */
	/* VSPP is treated as a separate part for energy dispersal */
	EnergyDisp.ProcessData(pvecInputData);


	/* Partitioning of input-stream ----------------------------------------- */
	iElementCounter = 0;

	if (iL[2] == 0)
	{
		/* Standard departitioning */
		/* Protection level A */
		for (j = 0; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][0]; i++)
			{
				vecbiEncInBuffer[j][i] = (*pvecInputData)[iElementCounter];

				iElementCounter++;
			}
		}

		/* Protection level B */
		for (j = 0; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][1]; i++)
			{
				vecbiEncInBuffer[j][iM[j][0] + i] =
					(*pvecInputData)[iElementCounter];

				iElementCounter++;
			}
		}
	}
	else
	{
		/* Special partitioning with hierarchical modulation. First set
		   hierarchical bits at the beginning, then append the rest */
		/* Hierarchical frame (always "iM[0][1]"). "iM[0][0]" is always "0" in
		   this case */
		for (i = 0; i < iM[0][1]; i++)
		{
			vecbiEncInBuffer[0][i] = (*pvecInputData)[iElementCounter];

			iElementCounter++;
		}


		/* Protection level A (higher protected part) */
		for (j = 1; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][0]; i++)
			{
				vecbiEncInBuffer[j][i] = (*pvecInputData)[iElementCounter];

				iElementCounter++;
			}
		}

		/* Protection level B  (lower protected part) */
		for (j = 1; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][1]; i++)
			{
				vecbiEncInBuffer[j][iM[j][0] + i] =
					(*pvecInputData)[iElementCounter];

				iElementCounter++;
			}
		}
	}


	/* Convolutional encoder ------------------------------------------------ */
	for (j = 0; j < iLevels; j++)
		ConvEncoder[j].Encode(vecbiEncInBuffer[j], vecbiEncOutBuffer[j]);


	/* Bit interleaver ------------------------------------------------------ */
	for (j = 0; j < iLevels; j++)
		if (piInterlSequ[j] != -1)
			BitInterleaver[piInterlSequ[j]].Interleave(vecbiEncOutBuffer[j]);


	/* QAM mapping ---------------------------------------------------------- */
	QAMMapping.Map(vecbiEncOutBuffer[0],
				   vecbiEncOutBuffer[1],
				   vecbiEncOutBuffer[2],
				   vecbiEncOutBuffer[3],
				   vecbiEncOutBuffer[4],
				   vecbiEncOutBuffer[5], pvecOutputData);
}

void CMLCEncoder::InitInternal(CParameter& TransmParam)
{
	int i;
	int	iNumInBits;

	CalculateParam(TransmParam, eChannelType);
	
	iNumInBits = iL[0] + iL[1] + iL[2];


	/* Init modules --------------------------------------------------------- */
	/* Energy dispersal */
	EnergyDisp.Init(iNumInBits, iL[2]);

	/* Encoder */
	for (i = 0; i < iLevels; i++)
		ConvEncoder[i].Init(eCodingScheme, eChannelType, iN[0], iN[1],
			iM[i][0], iM[i][1], iCodeRate[i][0], iCodeRate[i][1], i);

	/* Bit interleaver */
	/* First init all possible interleaver (According table "TableMLC.h" ->
	   "Interleaver sequence") */
	{
		BitInterleaver[0].Init(2 * iN[0], 2 * iN[1], 13);
		BitInterleaver[1].Init(2 * iN[0], 2 * iN[1], 21);
	}

	/* QAM-mapping */
	QAMMapping.Init(iN_mux, eCodingScheme);


	/* Allocate memory for internal bit-buffers ----------------------------- */
	for (i = 0; i < iLevels; i++)
	{
		/* Buffers for each encoder on all different levels */
		/* Add bits from higher protected and lower protected part */
		vecbiEncInBuffer[i].Init(iM[i][0] + iM[i][1]);
	
		/* Encoder output buffers for all levels. Must have the same length */
		vecbiEncOutBuffer[i].Init(iNumEncBits);
	}

	/* Define block-size for input and output */
	iInputBlockSize = iNumInBits;
	iOutputBlockSize = iN_mux;
}


/******************************************************************************\
* MLC-decoder                                                                  *
\******************************************************************************/
void CMLCDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i = 0, j = 0, k = 0; //init DM
	int			iElementCounter = 0; //init DM
	_BOOLEAN	bIteration = FALSE; //init DM

	/* Save input signal for signal constellation. We cannot use the copy
	   operator of vector because the input vector is not of the same size as
	   our intermediate buffer, therefore the "for"-loop */
	for (i = 0; i < iInputBlockSize; i++)
		vecSigSpacBuf[i] = (*pvecInputData)[i].cSig;



#if 0
// TEST
static FILE* pFile = fopen("test/constellation.dat", "w");
if (eChannelType == CParameter::CT_MSC) {
for (i = 0; i < iInputBlockSize; i++)
	fprintf(pFile, "%e %e\n", vecSigSpacBuf[i].real(), vecSigSpacBuf[i].imag());
fflush(pFile);
}
// close all;load constellation.dat;constellation=complex(constellation(:,1),constellation(:,2));plot(constellation,'.')
#endif




	/* Iteration loop */
	for (k = 0; k < iNumIterations + 1; k++)
	{
		for (j = 0; j < iLevels; j++)
		{
			/* Metric ------------------------------------------------------- */
			if (k > 0)
				bIteration = TRUE;
			else
				bIteration = FALSE;

			MLCMetric.CalculateMetric(pvecInputData, vecMetric,
				vecbiSubsetDef[0], vecbiSubsetDef[1], vecbiSubsetDef[2],
				vecbiSubsetDef[3], vecbiSubsetDef[4], vecbiSubsetDef[5],
				j, bIteration);


			/* Bit deinterleaver -------------------------------------------- */
			if (piInterlSequ[j] != -1)
				BitDeinterleaver[piInterlSequ[j]].Deinterleave(vecMetric);


			/* Viterbi decoder ---------------------------------------------- */
			rAccMetric =
				ViterbiDecoder[j].Decode(vecMetric, vecbiDecOutBits[j]);

			/* The last branch of encoding and interleaving must not be used at
			   the very last loop */
			/* "iLevels - 1" for iLevels = 1, 2, 3
			   "iLevels - 2" for iLevels = 6 */
			if ((k < iNumIterations) ||
				((k == iNumIterations) && !(j >= iIndexLastBranch)))
			{
				/* Convolutional encoder ------------------------------------ */
				ConvEncoder[j].Encode(vecbiDecOutBits[j], vecbiSubsetDef[j]);
			

				/* Bit interleaver ------------------------------------------ */
				if (piInterlSequ[j] != -1)
					BitInterleaver[piInterlSequ[j]].
						Interleave(vecbiSubsetDef[j]);
			}
		}
	}


	/* De-partitioning of input-stream -------------------------------------- */
	iElementCounter = 0;

	if (iL[2] == 0)
	{
		/* Standard departitioning */
		/* Protection level A (higher protected part) */
		for (j = 0; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][0]; i++)
			{
				(*pvecOutputData)[iElementCounter] = vecbiDecOutBits[j][i];

				iElementCounter++;
			}
		}

		/* Protection level B (lower protected part) */
		for (j = 0; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][1]; i++)
			{
				(*pvecOutputData)[iElementCounter] =
					vecbiDecOutBits[j][iM[j][0] + i];

				iElementCounter++;
			}
		}
	}
	else
	{
		/* Special departitioning with hierarchical modulation. First set
		   hierarchical bits at the beginning, then append the rest */
		/* Hierarchical frame (always "iM[0][1]"). "iM[0][0]" is always "0" in
		   this case */
		for (i = 0; i < iM[0][1]; i++)
		{
			(*pvecOutputData)[iElementCounter] = vecbiDecOutBits[0][i];

			iElementCounter++;
		}

		/* Protection level A (higher protected part) */
		for (j = 1; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][0]; i++)
			{
				(*pvecOutputData)[iElementCounter] = vecbiDecOutBits[j][i];

				iElementCounter++;
			}
		}

		/* Protection level B (lower protected part) */
		for (j = 1; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][1]; i++)
			{
				(*pvecOutputData)[iElementCounter] =
					vecbiDecOutBits[j][iM[j][0] + i];

				iElementCounter++;
			}
		}
	}


	/* Energy dispersal ----------------------------------------------------- */
	/* VSPP is treated as a separate part for energy dispersal (7.2.2) */
	EnergyDisp.ProcessData(pvecOutputData);
}

void CMLCDecoder::InitInternal(CParameter& ReceiverParam)
{
	int i = 0; //init DM

	/* First, calculate all necessary parameters for decoding process */
	CalculateParam(ReceiverParam, eChannelType);

	/* Reasonable number of iterations depends on coding scheme. With a
	   4-QAM no iteration is possible */
	if (eCodingScheme == CParameter::CS_1_SM)
		iNumIterations = 0;
	else
		iNumIterations = iInitNumIterations;

	/* Set this parameter to identify the last level of coder (important for
	   very last loop */
	iIndexLastBranch = iLevels - 1;

	iNumOutBits = iL[0] + iL[1] + iL[2];

	/* Reset accumulated metric for reliability test of transmission */
	rAccMetric = (_REAL) 0.0;


	/* Init modules --------------------------------------------------------- */
	/* Energy dispersal */
	EnergyDisp.Init(iNumOutBits, iL[2]);

	/* Viterby decoder */
	for (i = 0; i < iLevels; i++)
		ViterbiDecoder[i].Init(eCodingScheme, eChannelType, iN[0], iN[1], 
			iM[i][0], iM[i][1], iCodeRate[i][0], iCodeRate[i][1], i);

	/* Encoder */
	for (i = 0; i < iLevels; i++)
		ConvEncoder[i].Init(eCodingScheme, eChannelType, iN[0], iN[1],
			iM[i][0], iM[i][1], iCodeRate[i][0], iCodeRate[i][1], i);

	/* Bit interleaver */
	/* First init all possible interleaver (According table "TableMLC.h" ->
	   "Interleaver sequence") */
	{
		BitDeinterleaver[0].Init(2 * iN[0], 2 * iN[1], 13);
		BitDeinterleaver[1].Init(2 * iN[0], 2 * iN[1], 21);
		BitInterleaver[0].Init(2 * iN[0], 2 * iN[1], 13);
		BitInterleaver[1].Init(2 * iN[0], 2 * iN[1], 21);
	}
	
	/* Metric */
	MLCMetric.Init(iN_mux, eCodingScheme);


	/* Allocate memory for internal bit (metric) -buffers ------------------- */
	vecMetric.Init(iNumEncBits);

	/* Decoder output buffers for all levels. Have different length */
	for (i = 0; i < iLevels; i++)
		vecbiDecOutBits[i].Init(iM[i][0] + iM[i][1]);

	/* Buffers for subset definition (always number of encoded bits long) */
	for (i = 0; i < MC_MAX_NUM_LEVELS; i++)
		vecbiSubsetDef[i].Init(iNumEncBits);

	/* Init buffer for signal space */
	vecSigSpacBuf.Init(iN_mux);

	/* Define block-size for input and output */
	iInputBlockSize = iN_mux;
	iOutputBlockSize = iNumOutBits;
}

void CMLCDecoder::GetVectorSpace(CVector<_COMPLEX>& veccData)
{
	/* Init output vectors */
	veccData.Init(iN_mux);

	/* Do copying of data only if vector is of non-zero length which means that
	   the module was already initialized */
	if (iN_mux != 0)
	{
		/* Lock resources */
		Lock();

		/* Copy vectors */
		for (int i = 0; i < iN_mux; i++)
			veccData[i] = vecSigSpacBuf[i];

		/* Release resources */
		Unlock();
	}
}


/******************************************************************************\
* MLC base class                                                               *
\******************************************************************************/
void CMLC::CalculateParam(CParameter& Parameter, int iNewChannelType)
{
	int i = 0; //init DM
	int iMSCDataLenPartA = 0; //init DM

	switch (iNewChannelType)
	{
	/* FAC ********************************************************************/
	case CParameter::CT_FAC:
		eCodingScheme = CParameter::CS_1_SM;
		iN_mux = NUM_FAC_CELLS;

		iNumEncBits = NUM_FAC_CELLS * 2;

		iLevels = 1;

		/* Code rates for prot.-Level A and B for each level */
		/* Protection Level A */
		iCodeRate[0][0] = 0;

		/* Protection Level B */
		iCodeRate[0][1] = iCodRateCombFDC4SM;

		/* Define interleaver sequence for all levels */
		piInterlSequ = iInterlSequ4SM;


		/* iN: Number of OFDM-cells of each protection level ---------------- */
		iN[0] = 0;
		iN[1] = iN_mux;


		/* iM: Number of bits each level ------------------------------------ */
		iM[0][0] = 0;
		iM[0][1] = NUM_FAC_BITS_PER_BLOCK;

		/* iL: Number of bits each protection level ------------------------- */
		/* Higher protected part */
		iL[0] = 0;

		/* Lower protected part */
		iL[1] = iM[0][1];

		/* Very strong protected part (VSPP) */
		iL[2] = 0;
		break;


	/* MSC ********************************************************************/
	case CParameter::CT_MSC:
		eCodingScheme = Parameter.eMSCCodingScheme;
		iN_mux = Parameter.iNumUsefMSCCellsPerFrame;

		/* Data length for part A is the sum of all lengths of the streams */
		iMSCDataLenPartA = 0;

		switch (eCodingScheme)
		{
		case CParameter::CS_1_SM:
			iLevels = 1;

			/* Code rates for prot.-Level A and B for each level */
				/* Protection Level A */
			iCodeRate[0][0] = 0;

			/* Protection Level B */
			iCodeRate[0][1] = iCodRateCombMSC4SM;
			
			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ4SM;
			iNumEncBits = iN_mux * 2;

			/* iN: Number of OFDM-cells of each protection level ---------------- */
			iN[0] = 0;
			iN[1] = iN_mux;

			/* iM: Number of bits each level ------------------------------------ */
			iM[0][0] = 0;

			/* M_p,2 = RX_p * floor((2 * N_2 - 12) / RY_p) */
			iM[0][1] = iPuncturingPatterns[iCodRateCombMSC4SM][0] *
				(int) ((_REAL) (2 * iN_mux - 12) /
				iPuncturingPatterns[iCodRateCombMSC4SM][1]);

			/* iL: Number of bits each protection level ------------------------- */
			/* Higher protected part */
			iL[0] = 0;

			/* Lower protected part */
			iL[1] = iM[0][1];

			/* Very strong protected part (VSPP) */
			iL[2] = 0;

			break;

		case CParameter::CS_2_SM:
			iLevels = 2;

			/* Code rates for prot.-Level A and B for each level */
			for (i = 0; i < 2; i++)
			{
				/* Protection Level A */
				iCodeRate[i][0] = 0;

				/* Protection Level B */
				iCodeRate[i][1] =
					iCodRateCombMSC16SM[Parameter.MSCPrLe.iPartB][i];
			}

			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ16SM;

			iNumEncBits = iN_mux * 2;


			/* iN: Number of OFDM-cells of each protection level ------------ */
			/* N_1 = ceil(8 * X / (2 * RY_Icm * sum(R_p)) * RY_Icm */
			iN[0] = 0;

			/* Check if result can be possible, if not -> correct. This can
			   happen, if a wrong number is in "Param.Stream[x].iLenPartA" */
			if (iN[0] > iN_mux)
				iN[0] = 0;

			iN[1] = iN_mux - iN[0];


			/* iM: Number of bits each level -------------------------------- */
			for (i = 0; i < 2; i++)
			{
				/* M_p,1 = 2 * N_1 * R_p */
				iM[i][0] = 0;

				/* M_p,2 = RX_p * floor((2 * N_2 - 12) / RY_p) */
				iM[i][1] = 
					iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartB][i]][0] *
					(int) ((_REAL) (2 * iN[1] - 12) /
					iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartB][i]][1]);
			}


			/* iL: Number of bits each protection level --------------------- */
			/* Higher protected part */
			iL[0] = iM[0][0] + iM[1][0];

			/* Lower protected part */
			iL[1] = iM[0][1] + iM[1][1];

			/* Very strong protected part (VSPP) */
			iL[2] = 0;
			break;

		case CParameter::CS_3_SM:
			iLevels = 3;

			/* Code rates for prot.-Level A and B for each level */
			for (i = 0; i < 3; i++)
			{
				/* Protection Level A */
				iCodeRate[i][0] = 0;

				/* Protection Level B */
				iCodeRate[i][1] =
					iCodRateCombMSC64SM[Parameter.MSCPrLe.iPartB][i];
			}

			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ64SM;

			iNumEncBits = iN_mux * 2;


			/* iN: Number of OFDM-cells of each protection level ------------ */
			/* N_1 = ceil(8 * X / (2 * RY_Icm * sum(R_p)) * RY_Icm */
			iN[0] = 0;

			/* Check if result can be possible, if not -> correct. This can
			   happen, if a wrong number is in "Param.Stream[x].iLenPartA" */
			if (iN[0] > iN_mux)
				iN[0] = 0;

			iN[1] = iN_mux - iN[0];


			/* iM: Number of bits each level -------------------------------- */
			for (i = 0; i < 3; i++)
			{
				/* M_p,1 = 2 * N_1 * R_p */
				iM[i][0] = 0;

				/* M_p,2 = RX_p * floor((2 * N_2 - 12) / RY_p) */
				iM[i][1] = 
					iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartB][i]][0] *
					(int) ((_REAL) (2 * iN[1] - 12) /
					iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartB][i]][1]);
			}


			/* iL: Number of bits each protection level --------------------- */
			/* Higher protected part */
			iL[0] = iM[0][0] + iM[1][0] + iM[2][0];

			/* Lower protected part */
			iL[1] = iM[0][1] + iM[1][1] + iM[2][1];

			/* Very strong protected part (VSPP) */
			iL[2] = 0;
			break;
		}

		/* Set number of output bits for next module */
		Parameter.SetNumDecodedBitsMSC(iL[0] + iL[1] + iL[2]);

		/* Set total number of bits for hiearchical frame (needed for MSC
		   demultiplexer module) */
		Parameter.SetNumBitsHieraFrTot(iL[2]);
		break;
	}
}
