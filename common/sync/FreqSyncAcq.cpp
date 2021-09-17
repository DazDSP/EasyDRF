/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Daz Man 2021
 *
 * Description:
 *	Frequency synchronization acquisition (FFT-based)
 *
 * The input data is not modified by this module, it is just a measurement
 * of the frequency offset. The data is fed through this module.
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

#include "FreqSyncAcq.h"
#include "../Tables/TableCarrier.h"

FILE* ofile;

/* Implementation *************************************************************/
void CFreqSyncAcq::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i = 0; //init DM
	int			iMaxIndex = 0; //init DM
	fftw_real	rMaxValue;
	int			iNumDetPeaks = 0; //init DM
	_BOOLEAN	bNoPeaksLeft = FALSE; //init DM
	CRealVector vecrPSDPilPoin(3);

	if (bAquisition == TRUE)
	{
		/* Add new symbol in history (shift register) */			// hist size = 5120
		vecrFFTHistory.AddEnd((*pvecInputData), iInputBlockSize);	// input size = 1280


		/* Start algorithm when history memory is filled -------------------- */
		/* Wait until history memory is filled for the first time */
		if (iAquisitionCounter > 0)									// sum 5 times -> 5*1280
		{
			/* Decrease counter */
			iAquisitionCounter--;
		}
		else
		{
			/* Introduce a time-out for the averaging in case of no detected
			   peak in a certain time interval */
			iAverTimeOutCnt++;
			if (iAverTimeOutCnt > AVERAGE_TIME_OUT_NUMBER)
			{
				/* Reset counter and average vector */
				iAverTimeOutCnt = 0;
				iAverageCounter = NUM_BLOCKS_BEFORE_US_AV;
				vecrPSD = Zeros(iHalfBuffer);
			}

			/* Copy vector to matlib vector and calculate real-valued FFTW */
			for (i = 0; i < iTotalBufferSize; i++)	// 1024 * 5 Samples per block
				vecrFFTInput[i] = vecrFFTHistory[i] * vecrHann[i];			

			veccFFTOutput = rfft(vecrFFTInput, FftPlan);

			/* Calculate power spectrum (X = real(F)^2 + imag(F)^2) and average
			   results */
			for (i = 1; i < iHalfBuffer; i++)		// summed until timeout -> 100 blocks -> 2 sec
				vecrPSD[i] += SqMag(veccFFTOutput[i]);

			/* Wait until we have sufficient data averaged */
			if (iAverageCounter > 0)				// 10 blocks for averaging -> 200 msec
			{
				/* Decrease counter */
				iAverageCounter--;
			}
			else
			{
				/* Correlate known frequency-pilot structure with power
				   spectrum */
				for (i = 0; i < iSearchWinSize; i++)
				{
					vecrPSDPilCor[i] = 
						vecrPSD[i + veciTableFreqPilots[0]] +
						vecrPSD[i + veciTableFreqPilots[1]] +
						vecrPSD[i + veciTableFreqPilots[2]];
				}

				/* -------------------------------------------------------------
				   Low pass filtering over frequency axis. We do the filtering 
				   from both sides, once from right to left and then from left 
				   to the right side. Afterwards, these results are averaged */
				const CReal rLambdaF = 0.9;
				/* From the left edge to the right edge */
				vecrFiltResLR[0] = vecrPSDPilCor[0];
				for (i = 1; i < iSearchWinSize; i++)
					vecrFiltResLR[i] = rLambdaF * (vecrFiltResLR[i - 1] -
						vecrPSDPilCor[i]) + vecrPSDPilCor[i];

				/* From the right edge to the left edge */
				vecrFiltResRL[iSearchWinSize - 1] =
					vecrPSDPilCor[iSearchWinSize - 1];
				for (i = iSearchWinSize - 2; i >= 0; i--)
					vecrFiltResRL[i] = rLambdaF * (vecrFiltResRL[i + 1] -
						vecrPSDPilCor[i]) + vecrPSDPilCor[i];

				/* Average RL and LR filter outputs */
				vecrFiltRes = vecrFiltResLR + vecrFiltResRL;
		

				/* Detect peaks by the distance to the filtered curve ------- */
				/* Get peak indices of detected peaks */
				iNumDetPeaks = 0;
				
				for (i = iStartDCSearch; i < iEndDCSearch; i++)
				{
					if (vecrPSDPilCor[i] / vecrFiltRes[i] > rPeakBoundFiltToSig)
					{
						veciPeakIndex[iNumDetPeaks] = i;
						iNumDetPeaks++;
					}
				}

				/* Check, if at least one peak was detected */
				if (iNumDetPeaks > 0)
				{
					/* ---------------------------------------------------------
					   The following test shall exclude sinusoid interferers in
					   the received spectrum */
					CVector<int> vecbFlagVec(iNumDetPeaks, 1);

					/* Check all detected peaks in the "PSD-domain" if there are
					   at least two peaks with approx the same power at the
					   correct places (positions of the desired pilots) */
					for (i = 0; i < iNumDetPeaks; i++)
					{
						/* Fill the vector with the values at the desired 
						   pilot positions */
						vecrPSDPilPoin[0] =
							vecrPSD[veciPeakIndex[i] + veciTableFreqPilots[0]];
						vecrPSDPilPoin[1] =
							vecrPSD[veciPeakIndex[i] + veciTableFreqPilots[1]];
						vecrPSDPilPoin[2] =
							vecrPSD[veciPeakIndex[i] + veciTableFreqPilots[2]];

						/* Sort, to extract the highest and second highest
						   peak */
						vecrPSDPilPoin = Sort(vecrPSDPilPoin);

						/* Debar peak, if it is much higher than second highest
						   peak (most probably a sinusoid interferer) */
						if (vecrPSDPilPoin[2] / vecrPSDPilPoin[1] >
							MAX_RAT_PEAKS_AT_PIL_POS)
						{
							/* Reset "good-flag" */
							vecbFlagVec[i] = 0;
						}
					}


					/* Get maximum ------------------------------------------ */
					/* First, get the first valid peak entry and init the 
					   maximum with this value. We also detect, if a peak is 
					   left */
					bNoPeaksLeft = TRUE;
					for (i = 0; i < iNumDetPeaks; i++)
					{
						if (vecbFlagVec[i] == 1)
						{
							/* At least one peak is left */
							bNoPeaksLeft = FALSE;

							/* Init max value */
							iMaxIndex = veciPeakIndex[i];
							rMaxValue = vecrPSDPilCor[veciPeakIndex[i]];
						}
					}

					if (bNoPeaksLeft == FALSE)
					{
						/* Actual maximum detection, take the remaining peak
						   which has the highest value */
						for (i = 0; i < iNumDetPeaks; i++)
						{
							if ((vecbFlagVec[i] == 1) &&
								(vecrPSDPilCor[veciPeakIndex[i]] >
								rMaxValue))
							{
								iMaxIndex = veciPeakIndex[i];
								rMaxValue = vecrPSDPilCor[veciPeakIndex[i]];
							}
						}

						/* -----------------------------------------------------
						   An acquisition frequency offest estimation was
						   found */
						/* Calculate frequency offset and set global parameter
						   for offset */
						ReceiverParam.rFreqOffsetAcqui =
							(_REAL) iMaxIndex / iTotalBufferSize;  

						/* Reset acquisition flag */
						bAquisition = FALSE;
					}
				}
			}
		}

		/* Do not transfer any data to the next block if no frequency
		   acquisition was successfully done */
		iOutputBlockSize = 0;
		if (bFreqFound)
		{
			PostWinMessage(MS_FREQ_FOUND, 2); //added DM
			bFreqFound = FALSE;
		}
		else if (bAquisition == FALSE)	// copy last block
		{
			int ioutvecs = (*pvecOutputData).Size();
			int ihistvecs = vecrFFTHistory.Size();
			int ifrom;
			if (ioutvecs >= ihistvecs)
			{ iOutputBlockSize = ihistvecs; ifrom = 0; }
			else
			{ iOutputBlockSize = ioutvecs; ifrom = ihistvecs - ioutvecs; }
			for (i = 0; i < iOutputBlockSize; i++)
				(*pvecOutputData)[i] = vecrFFTHistory[i + ifrom];
		}
		
	}
	else
	{
		/* Use the same block size as input block size */
		iOutputBlockSize = iInputBlockSize;

		/* Copy data from input to the output. Data is not modified in this
		   module */
		for (i = 0; i < iOutputBlockSize; i++)
			(*pvecOutputData)[i] = (*pvecInputData)[i];

		if (!bFreqFound)
		{
			PostWinMessage(MS_FREQ_FOUND, 0); //added DM
			bFreqFound = TRUE;
		}
	}
}

void CFreqSyncAcq::InitInternal(CParameter& ReceiverParam)
{

	/* Needed for calculating offset in Hertz in case of synchronized input
	   (for simulation) */
	iFFTSize = ReceiverParam.iFFTSizeN;

	/* We using parameters from robustness mode B as pattern for the desired
	   frequency pilot positions */
	veciTableFreqPilots[0] =
		iTableFreqPilRobModB[0][0] * NUM_BLOCKS_4_FREQ_ACQU;
	veciTableFreqPilots[1] =
		iTableFreqPilRobModB[1][0] * NUM_BLOCKS_4_FREQ_ACQU;
	veciTableFreqPilots[2] =
		iTableFreqPilRobModB[2][0] * NUM_BLOCKS_4_FREQ_ACQU;

	/* Total buffer size */
	iTotalBufferSize = RMB_FFT_SIZE_N * NUM_BLOCKS_4_FREQ_ACQU; // 1024 * 5


	/* -------------------------------------------------------------------------
	   Set start- and endpoint of search window for DC carrier after the
	   correlation with the known pilot structure */
	/* Normalize the desired position and window size which are in Hertz */
	const _REAL rNormDesPos = rCenterFreq / SOUNDCRD_SAMPLE_RATE * 2;
	const _REAL rNormHalfWinSize = rWinSize / SOUNDCRD_SAMPLE_RATE;

	/* Length of the half of the spectrum of real input signal (the other half
	   is the same because of the real input signal). We have to consider the
	   Nyquist frequency ("iTotalBufferSize" is always even!) */
	iHalfBuffer = iTotalBufferSize / 2 + 1;

	/* Search window is smaller than haft-buffer size because of correlation
	   with pilot positions */
	iSearchWinSize = iHalfBuffer - veciTableFreqPilots[2];

	/* Calculate actual indices of start and end of search window */
	iStartDCSearch =
		(int) Floor((rNormDesPos - rNormHalfWinSize) * iHalfBuffer);
	iEndDCSearch = (int) Ceil((rNormDesPos + rNormHalfWinSize) * iHalfBuffer);
	//iStartDCSearch = (int) ((rNormDesPos - rWinSize / 2) * iHalfBuffer);
	//iEndDCSearch = (int) ((rNormDesPos + rWinSize / 2) * iHalfBuffer);

	/* Check range. If out of range -> correct */
	if (!((iStartDCSearch > 0) && (iStartDCSearch < iSearchWinSize)))
		iStartDCSearch = 0;

	if (!((iEndDCSearch > 0) && (iEndDCSearch < iSearchWinSize)))
		iEndDCSearch = iSearchWinSize;

	/* Init vectors and fft plan -------------------------------------------- */
	/* Allocate memory for FFT-histories and init with zeros */
	vecrFFTHistory.Init(iTotalBufferSize, (_REAL) 0.0);
	vecrFFTInput.Init(iTotalBufferSize);
	veccFFTOutput.Init(iHalfBuffer);
	vecrHann.Init(iTotalBufferSize);
	vecrHann = Hann(iTotalBufferSize);

	vecrPSD.Init(iHalfBuffer);

	/* Allocate memory for PSD after pilot correlation */
	vecrPSDPilCor.Init(iHalfBuffer);

	/* Init vectors for filtering in frequency direction */
	vecrFiltResLR.Init(iHalfBuffer);
	vecrFiltResRL.Init(iHalfBuffer);
	vecrFiltRes.Init(iHalfBuffer);

	/* Index memory for detected peaks (assume worst case with the size) */
	veciPeakIndex.Init(iHalfBuffer);

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iTotalBufferSize);

	/* Define block-sizes for input (The output block size is set inside
	   the processing routine, therefore only a maximum block size is set
	   here) */
	iInputBlockSize = ReceiverParam.iSymbolBlockSize;

	/* We have to consider that the next module can take up to two symbols per
	   step. This can be satisfied be multiplying with "3" */
	iMaxOutputBlockSize = 5 * ReceiverParam.iSymbolBlockSize;
}

void CFreqSyncAcq::SetSearchWindow(_REAL rNewCenterFreq, _REAL rNewWinSize)
{
	/* Set internal parameters */
	rCenterFreq = rNewCenterFreq;
	rWinSize = rNewWinSize;

	/* Set flag to initialize the module to the new parameters */
	SetInitFlag();
}

void CFreqSyncAcq::SetSensivity(_REAL rNewSensivity)
{
	rPeakBoundFiltToSig = rNewSensivity;
}

void CFreqSyncAcq::StartAcquisition()
{
	/* Set flag so that the actual acquisition routine is entered */
	bAquisition = TRUE;

	/* Reset (or init) counters */
	iAquisitionCounter = NUM_BLOCKS_4_FREQ_ACQU;
	iAverageCounter = NUM_BLOCKS_BEFORE_US_AV;
	iAverTimeOutCnt = 0;

	/* Reset vector for the averaged spectrum */
	vecrPSD = Zeros(iHalfBuffer);

	/* Reset FFT-history */
	vecrFFTHistory.Reset((_REAL) 0.0);

	bFreqFound = FALSE;
}
