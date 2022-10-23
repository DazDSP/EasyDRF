/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	OFDM modulation;
 *	OFDM demodulation, SNR estimation, PSD estimation
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

#include "OFDM.h"
#include "fir.h"


/* Implementation *************************************************************/
/******************************************************************************\
* OFDM-modulation                                                              *
\******************************************************************************/
void COFDMModulation::ProcessDataInternal(CParameter& TransmParam)
{
	int	i = 0; //init DM

	/* Copy input vector in matlib vector and place bins at the correct
	   position */
	for (i = iShiftedKmin; i < iEndIndex; i++)
		veccFFTInput[i] = (*pvecInputData)[i - iShiftedKmin];

	/* Calculate inverse fast Fourier transformation */
	veccFFTOutput = Ifft(veccFFTInput, FftPlan);

	/* Copy complex FFT output in output buffer and scale */
	for (i = 0; i < iDFTSize; i++)
		(*pvecOutputData)[i + iGuardSize] = veccFFTOutput[i] * (CReal) iDFTSize;


	/* Copy data from the end to the guard-interval (Add guard-interval) */
	for (i = 0; i < iGuardSize; i++)
		(*pvecOutputData)[i] = (*pvecOutputData)[iDFTSize + i];
	
	/* Shift spectrum to desired IF ----------------------------------------- */
	//This now shifts down to a zero-IF for PAPR processing DM 2022
	/* Only apply shifting if phase is not zero */
	if (cExpStep != _COMPLEX((_REAL) 1.0, (_REAL) 0.0))
	{
		for (i = 0; i < iOutputBlockSize; i++)
		{
			(*pvecOutputData)[i] = (*pvecOutputData)[i] * Conj(cCurExp);
			
			/* Rotate exp-pointer on step further by complex multiplication
			   with precalculated rotation vector cExpStep. This saves us from
			   calling sin() and cos() functions all the time (iterative
			   calculation of these functions) */
			cCurExp *= cExpStep;
		}
	}
}

void COFDMModulation::InitInternal(CParameter& TransmParam)
{
	/* Get global parameters */
	iDFTSize = TransmParam.iFFTSizeN;
	iGuardSize = TransmParam.iGuardSize;
	iShiftedKmin = TransmParam.iShiftedKmin;

	/* Last index */
	iEndIndex = TransmParam.iShiftedKmax + 1;

	/* Normalized offset correction factor for IF shift. Subtract the
	   default IF frequency ("VIRTUAL_INTERMED_FREQ") */
//	_REAL rNormCurFreqOffset = (_REAL) -2.0 * crPi * (rDefCarOffset - VIRTUAL_INTERMED_FREQ) / SOUNDCRD_SAMPLE_RATE;
//	_REAL rNormCurFreqOffset = (_REAL)-2.0 * crPi * (-1225 - VIRTUAL_INTERMED_FREQ) / SOUNDCRD_SAMPLE_RATE;
//	_REAL rNormCurFreqOffset = (_REAL)2.0 * crPi * (VIRTUAL_INTERMED_FREQ + 1225) / SOUNDCRD_SAMPLE_RATE; //Mix directly to 0Hz IF for PAPR stage DM
	_REAL rNormCurFreqOffset = (_REAL)2.0 * crPi * (VIRTUAL_INTERMED_FREQ + OFFSET) / SOUNDCRD_SAMPLE_RATE; //Mix directly to 0Hz IF for PAPR stage DM

	/* Rotation vector for exp() calculation */
	cExpStep = _COMPLEX(cos(rNormCurFreqOffset), sin(rNormCurFreqOffset));

	/* Start with phase null (can be arbitrarily chosen) */
	cCurExp = (_REAL) 1.0;

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iDFTSize);

	/* Allocate memory for intermediate result of fft. Zero out input vector
	   (because only a few bins are used, the rest has to be zero) */
	veccFFTInput.Init(iDFTSize, (CReal) 0.0);
	veccFFTOutput.Init(iDFTSize);

	/* Define block-sizes for input and output */
	iInputBlockSize = TransmParam.iNumCarrier;
	iOutputBlockSize = TransmParam.iSymbolBlockSize;
}


/******************************************************************************\
* OFDM-demodulation                                                            *
\******************************************************************************/

CComplexVector			vecInput;
double hilbdata[1500] = {0.0};

void COFDMDemodulation::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i = 0; //init DM
	_REAL		rNormCurFreqOffset = 0; //init DM
	_REAL		rSkipGuardIntPhase = 0; //init DM
	_COMPLEX	cExpStep = 0; //init DM

	/* Total frequency offset from acquisition and tracking (we calculate the
	   normalized frequency offset) */
	rNormCurFreqOffset = (_REAL) 2.0 * crPi * (ReceiverParam.rFreqOffsetAcqui +	ReceiverParam.rFreqOffsetTrack - rInternIFNorm);
	//rNormCurFreqOffset = (_REAL) 2.0 * crPi * (2500.0/24000.0);

	/* New rotation vector for exp() calculation */
	cExpStep = _COMPLEX(cos(rNormCurFreqOffset), sin(rNormCurFreqOffset));

	/* Input data is real, make complex and compensate for frequency offset */
	if (ReceiverParam.bUseFilter)
	{
		/* To get a continuous counter we need to take the guard-interval and
		   timing corrections into account */
		rSkipGuardIntPhase = rNormCurFreqOffset * ((iGuardSize - zffiltlen) - (*pvecInputData).GetExData().iCurTimeCorr);

		/* Apply correction */
		cCurExp *= _COMPLEX(cos(rSkipGuardIntPhase), sin(rSkipGuardIntPhase));
		
		for (i = 0; i < iInputBlockSize; i++)
		{
			vecInput[i] = (*pvecInputData)[i] * Conj(cCurExp);
			cCurExp *= cExpStep;
		}
		DoFir(&vecInput[0],&veccFFTInput[0]);

	}
	else
	{
		/* To get a continuous counter we need to take the guard-interval and
		   timing corrections into account */
		rSkipGuardIntPhase = rNormCurFreqOffset * ((iGuardSize) - (*pvecInputData).GetExData().iCurTimeCorr);

		/* Apply correction */
		cCurExp *= _COMPLEX(cos(rSkipGuardIntPhase), sin(rSkipGuardIntPhase));

		for (i = 0; i < iDFTSize; i++)
		{
			veccFFTInput[i] = (*pvecInputData)[i] * Conj(cCurExp);
			cCurExp *= cExpStep;
		}
	}

	/* Calculate Fourier transformation (actual OFDM demodulation) */
	veccFFTOutput = Fft(veccFFTInput, FftPlan);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData)[i - iShiftedKmin] = veccFFTOutput[i] / (CReal) iDFTSize;


	/* Save averaged spectrum for plotting ---------------------------------- */
	/* Average power (using power of this tap) (first order IIR filter) */
	for (i = 0; i < iLenPowSpec; i++)
		IIR1(vecrPowSpec[i], SqMag(veccFFTOutput[i]), rLamPSD);
}

void COFDMDemodulation::InitInternal(CParameter& ReceiverParam)
{
	iDFTSize = ReceiverParam.iFFTSizeN;
	iGuardSize = ReceiverParam.iGuardSize;
	iShiftedKmin = ReceiverParam.iShiftedKmin;
	iShiftedKmax = ReceiverParam.iShiftedKmax;

	/* Calculate the desired frequency position (normalized) */
	rInternIFNorm = (_REAL) ReceiverParam.iIndexDCFreq / iDFTSize;

	/* Start with phase null (can be arbitrarily chosen) */
	cCurExp = (_REAL) 1.0;

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iDFTSize);


	/* Vector for power density spectrum of input signal */
	iLenPowSpec = iDFTSize / 2;
	vecrPowSpec.Init(iLenPowSpec, (_REAL) 0.0);
	rLamPSD = IIR1Lam(TICONST_PSD_EST_OFDM, (CReal) SOUNDCRD_SAMPLE_RATE / ReceiverParam.iSymbolBlockSize); /* Lambda for IIR filter */

	/* Allocate memory for intermediate result of fftw */
	veccFFTInput.Init(iDFTSize); //@@
	veccFFTOutput.Init(iDFTSize);
	/* Define block-sizes for input and output */
	iInputBlockSize = iDFTSize + zffiltlen;  //@@
	iOutputBlockSize = ReceiverParam.iNumCarrier;

	vecInput.Init(iInputBlockSize);
	
	FirInit(iDFTSize,ReceiverParam.GetSpectrumOccup()); 

}

void COFDMDemodulation::GetPowDenSpec(CVector<_REAL>& vecrData)
{
	/* Init output vectors */
	vecrData.Init(iLenPowSpec, (_REAL) 0.0);

	/* Do copying of data only if vector is of non-zero length which means that
	   the module was already initialized */
	if (iLenPowSpec != 0)
	{
		/* Lock resources */
		Lock();

		_REAL rNormData = (_REAL) iDFTSize * iDFTSize * _MAXSHORT;

		/* Apply the normalization (due to the FFT) */
		for (int i = 0; i < iLenPowSpec; i++)
		{
			_REAL rNormPowSpec = vecrPowSpec[i] / rNormData;

			if (rNormPowSpec > 0)
				vecrData[i] = (_REAL) 10.0 * log10(vecrPowSpec[i] / rNormData);
			else
				vecrData[i] = RET_VAL_LOG_0;
		}

		/* Release resources */
		Unlock();
	}
}


