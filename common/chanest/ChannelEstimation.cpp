/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Channel estimation and equalization
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

#include "ChannelEstimation.h"


/* Implementation *************************************************************/
void CChannelEstimation::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i = 0, j = 0, k = 0; //init DM
	int			iModSymNum = 0; //init DM
	_COMPLEX	cModChanEst;
	_REAL		rSNRAftTiInt = 0; //init DM
	_REAL		rCurSNREst = 0; //init DM
	_REAL		rOffsPDSEst = 0; //init DM

	/* Check if symbol ID index has changed by the synchronization unit. If it
	   has changed, reinit this module */
	if ((*pvecInputData).GetExData().bSymbolIDHasChanged == TRUE)
	{
		SetInitFlag();
		return;
	}

	/* Move data in history-buffer (from iLenHistBuff - 1 towards 0) */
	for (j = 0; j < iLenHistBuff - 1; j++)
	{
		for (i = 0; i < iNumCarrier; i++)
			matcHistory[j][i] = matcHistory[j + 1][i];
	}

	/* Write new symbol in memory */
	for (i = 0; i < iNumCarrier; i++)
		matcHistory[iLenHistBuff - 1][i] = (*pvecInputData)[i];


	/* Time interpolation *****************************************************/
	/* Get symbol-counter for next symbol. Use the count from the frame 
	   synchronization (in OFDM.cpp). Call estimation routine */
	rSNRAftTiInt = 
		pTimeInt->Estimate(pvecInputData, veccPilots, 
						   ReceiverParam.matiMapTab[(*pvecInputData).
						   GetExData().iSymbolID],
						   ReceiverParam.matcPilotCells[(*pvecInputData).
						   GetExData().iSymbolID], rSNREstimate);

	/* Debar initialization of channel estimation in time direction */
	if (iInitCnt > 0)
	{
		iInitCnt--;

		/* Do not put out data in initialization phase */
		iOutputBlockSize = 0;

		/* Do not continue */
		return;
	}
	else
		iOutputBlockSize = iNumCarrier; 

	/* Define DC carrier for robustness mode D because there is not pilot */
	if (iDCPos != 0)
		veccPilots[iDCPos] = (CReal) 0.0;


	/* -------------------------------------------------------------------------
	   Use time-interpolated channel estimate for timing synchronization 
	   tracking */
	TimeSyncTrack.Process(ReceiverParam, veccPilots, 
		(*pvecInputData).GetExData().iCurTimeCorr, rLenPDSEst /* out */,
		rOffsPDSEst /* out */);


	/* Frequency-interploation ************************************************/
	switch (TypeIntFreq)
	{
	case FLINEAR:
		/**********************************************************************\
		 * Linear interpolation												  *
		\**********************************************************************/
		/* Set first pilot position */
		veccChanEst[0] = veccPilots[0];

		for (j = 0, k = 1; j < iNumCarrier - iScatPilFreqInt;
			j += iScatPilFreqInt, k++)
		{
			/* Set values at second time pilot position in cluster */
			veccChanEst[j + iScatPilFreqInt] = veccPilots[k];

			/* Interpolation cluster */
			for (i = 1; i < iScatPilFreqInt; i++)
			{
				/* E.g.: c(x) = (c_4 - c_0) / 4 * x + c_0 */
				veccChanEst[j + i] =
					(veccChanEst[j + iScatPilFreqInt] - veccChanEst[j]) /
					(_REAL) (iScatPilFreqInt) * (_REAL) i + veccChanEst[j];
			}
		}
		break;

	case FDFTFILTER:
		/**********************************************************************\
		 * DFT based algorithm												  *
		\**********************************************************************/
		/* ---------------------------------------------------------------------
		   Put all pilots at the beginning of the vector. The "real" length of
		   the vector "pcFFTWInput" is longer than the No of pilots, but we 
		   calculate the FFT only over "iNumCarrier / iScatPilFreqInt + 1"
		   values (this is the number of pilot positions) */
		/* Weighting pilots with window */
		veccPilots *= vecrDFTWindow;

		/* Transform in time-domain */
		veccPilots = Ifft(veccPilots, FftPlanShort);

		/* Set values outside a defined bound to zero, zero padding (noise
		   filtering). Copy second half of spectrum at the end of the new vector
		   length and zero out samples between the two parts of the spectrum */
		veccIntPil.Merge(
			/* First part of spectrum */
			veccPilots(1, iStartZeroPadding), 
			/* Zero padding in the middle, length: Total length minus length of
			   the two parts at the beginning and end */
			CComplexVector(Zeros(iLongLenFreq - 2 * iStartZeroPadding), 
			Zeros(iLongLenFreq - 2 * iStartZeroPadding)), 
			/* Set the second part of the actual spectrum at the end of the new
			   vector */
			veccPilots(iNumIntpFreqPil - iStartZeroPadding + 1, 
			iNumIntpFreqPil));

		/* Transform back in frequency-domain */
		veccIntPil = Fft(veccIntPil, FftPlanLong);

		/* Remove weighting with DFT window by inverse multiplication */
		veccChanEst = veccIntPil(1, iNumCarrier) * vecrDFTwindowInv;
		break;

	case FWIENER:
		/**********************************************************************\
		 * Wiener filter													   *
		\**********************************************************************/
#ifdef UPD_WIENER_FREQ_EACH_DRM_FRAME
		/* Update filter coefficients once in one DRM frame */
		if (iUpCntWienFilt > 0)
		{
			iUpCntWienFilt--;

			/* Get maximum delay spread and offset in one DRM frame */
			if (rLenPDSEst > rMaxLenPDSInFra)
				rMaxLenPDSInFra = rLenPDSEst;

			if (rOffsPDSEst < rMinOffsPDSInFra)
				rMinOffsPDSInFra = rOffsPDSEst;
		}
		else
		{
#else
		/* Update Wiener filter each OFDM symbol. Use current estimates */
		rMaxLenPDSInFra = rLenPDSEst;
		rMinOffsPDSInFra = rOffsPDSEst;
#endif
			/* Update filter taps */
			UpdateWienerFiltCoef(rSNRAftTiInt, rMaxLenPDSInFra / iNumCarrier,
				rMinOffsPDSInFra / iNumCarrier);

#ifdef UPD_WIENER_FREQ_EACH_DRM_FRAME
			/* Reset counter and maximum storage variable */
			iUpCntWienFilt = iNumSymPerFrame;
			rMaxLenPDSInFra = (_REAL) 0.0;
			rMinOffsPDSInFra = rGuardSizeFFT;
		}
#endif

		/* FIR filter of the pilots with filter taps. We need to filter the
		   pilot positions as well to improve the SNR estimation (which 
		   follows this procedure) */
		for (j = 0; j < iNumCarrier; j++)
		{
			/* Convolution */
			veccChanEst[j] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);

			for (i = 0; i < iLengthWiener; i++)
				veccChanEst[j] += 
					matcFiltFreq[j][i] * veccPilots[veciPilOffTab[j] + i];
		}
		break;
	}


	/* Equalize the output vector ------------------------------------------- */
	/* Write to output vector. Take oldest symbol of history for output. Also,
	   ship the channel state at a certain cell */
	for (i = 0; i < iNumCarrier; i++)
	{
		(*pvecOutputData)[i].cSig = matcHistory[0][i] / veccChanEst[i];
		(*pvecOutputData)[i].rChan = SqMag(veccChanEst[i]);
	}


	/* -------------------------------------------------------------------------
	   Calculate symbol ID of the current output block and set parameter */
	(*pvecOutputData).GetExData().iSymbolID = 
		(*pvecInputData).GetExData().iSymbolID - iLenHistBuff + 1;


	/* SNR estimation ------------------------------------------------------- */
	/* Modified symbol ID, check range {0, ..., iNumSymPerFrame} */
	iModSymNum = (*pvecOutputData).GetExData().iSymbolID;

	while (iModSymNum < 0)
		iModSymNum += iNumSymPerFrame;

	for (i = 0; i < iNumCarrier; i++)
	{
		switch (TypeSNREst)
		{
		case SNR_PIL:
			/* Use estimated channel and compare it to the received pilots. This
			   estimation works only if the channel estimation was successful */
			/* Identify pilot positions. Use MODIFIED "iSymbolID" (See lines
			   above) */
			if (_IsScatPil(ReceiverParam.matiMapTab[iModSymNum][i]))
			{
				/* We assume that the channel estimation in "veccChanEst" is
				   noise free (e.g., the wiener interpolation does noise
				   reduction). Thus, we have an estimate of the received signal
				   power \hat{r} = s * \hat{h}_{wiener} */
				cModChanEst = veccChanEst[i] *
					ReceiverParam.matcPilotCells[iModSymNum][i];


				/* Calculate and average noise and signal estimates --------- */
				/* The noise estimation is difference between the noise reduced
				   signal and the noisy received signal
				   \tilde{n} = \hat{r} - r */
				IIR1(rNoiseEst, SqMag(matcHistory[0][i] - cModChanEst),
					rLamSNREstFast);

				/* The received signal power estimation is just \hat{r} */
				IIR1(rSignalEst, SqMag(cModChanEst), rLamSNREstFast);

				/* Calculate final result (signal to noise ratio) */
				if (rNoiseEst != 0)
					rCurSNREst = rSignalEst / rNoiseEst;
				else
					rCurSNREst = (_REAL) 1.0;

				/* Bound the SNR at 0 dB */
				if (rCurSNREst < (_REAL) 1.0)
					rCurSNREst = (_REAL) 1.0;

				/* Average the SNR with a two sided recursion */
				IIR1TwoSided(rSNREstimate, rCurSNREst, rLamSNREstFast,
					rLamSNREstSlow);
			}
			break;

		case SNR_FAC:
			/* SNR estimation with initialization */
			if (iSNREstInitCnt > 0)
			{
				/* Initial signal estimate. Use channel estimation from all
				   cells. Apply averaging */
				rSignalEst += (*pvecOutputData)[i].rChan;

				iSNREstInitCnt--;
			}
			else
			{
				/* Only right after initialization phase apply initial SNR
				   value */
				if (bWasSNRInit == TRUE)
				{
					/* Normalize average */
					rSignalEst /= iNumCellsSNRInit;

					/* Apply initial SNR value */
					rNoiseEst = rSignalEst / rSNREstimate;

					bWasSNRInit = FALSE;
				}

				/* Only use FAC cells for this SNR estimation method */
				if (_IsFAC(ReceiverParam.matiMapTab[iModSymNum][i]))
				{
					/* Get tentative decision for current FAC cell (squared) */
					CReal rCurErrPow =
						TentativeFACDec((*pvecOutputData)[i].cSig);

					/* Use decision together with channel estimate to get
					   estimates for signal and noise */
					IIR1(rNoiseEst, rCurErrPow * (*pvecOutputData)[i].rChan,
						rLamSNREstFast);

					IIR1(rSignalEst, (*pvecOutputData)[i].rChan,
						rLamSNREstFast);

					/* Calculate final result (signal to noise ratio) */
					if (rNoiseEst != (_REAL) 0.0)
						rCurSNREst = rSignalEst / rNoiseEst;
					else
						rCurSNREst = (_REAL) 1.0;

					/* Bound the SNR at 0 dB */
					if (rCurSNREst < (_REAL) 1.0)
						rCurSNREst = (_REAL) 1.0;

					/* The channel estimation algorithms need the SNR normalized
					   to the energy of the pilots */
					rSNREstimate = rCurSNREst / rSNRCorrectFact;
				}
			}
			break;
		}
	}
}

void CChannelEstimation::InitInternal(CParameter& ReceiverParam)
{
	/* Get parameters from global struct */
	iScatPilTimeInt = ReceiverParam.iScatPilTimeInt;
	iScatPilFreqInt = ReceiverParam.iScatPilFreqInt;
	iNumIntpFreqPil = ReceiverParam.iNumIntpFreqPil;
	iNumCarrier = ReceiverParam.iNumCarrier;
	iFFTSizeN = ReceiverParam.iFFTSizeN;
	iNumSymPerFrame = ReceiverParam.iNumSymPerFrame;

	/* Length of guard-interval with respect to FFT-size! */
	rGuardSizeFFT = (_REAL) iNumCarrier *
		ReceiverParam.RatioTgTu.iEnum / ReceiverParam.RatioTgTu.iDenom;

	/* If robustness mode D is active, get DC position. This position cannot
	   be "0" since in mode D no 5 kHz mode is defined (see DRM-standard). 
	   Therefore we can also use this variable to get information whether
	   mode D is active or not (by simply write: "if (iDCPos != 0)") */
	iDCPos = 0;

	/* FFT must be longer than "iNumCarrier" because of zero padding effect (
	   not in robustness mode D! -> "iLongLenFreq = iNumCarrier") */
	iLongLenFreq = iNumCarrier + iScatPilFreqInt - 1;

	/* Init vector for received data at pilot positions */
	veccPilots.Init(iNumIntpFreqPil);

	/* Init vector for interpolated pilots */
	veccIntPil.Init(iLongLenFreq);

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlanShort.Init(iNumIntpFreqPil);
	FftPlanLong.Init(iLongLenFreq);

	/* Choose time interpolation method and set pointer to correcponding 
	   object */
	switch (TypeIntTime)
	{
	case TLINEAR:
		pTimeInt = &TimeLinear;
		break;

	case TWIENER:
		pTimeInt = &TimeWiener;
		break;
	}

	/* Init time interpolation interface and set delay for interpolation */
	iLenHistBuff = pTimeInt->Init(ReceiverParam);

	/* Init time synchronization tracking unit */
	TimeSyncTrack.Init(ReceiverParam, iLenHistBuff);

	/* Set channel estimation delay in global struct. This is needed for 
	   simulation */
	ReceiverParam.iChanEstDelay = iLenHistBuff;


	/* Init window for DFT operation for frequency interpolation ------------ */
	/* Init memory */
	vecrDFTWindow.Init(iNumIntpFreqPil);
	vecrDFTwindowInv.Init(iNumCarrier);

	/* Set window coefficients */
	switch (eDFTWindowingMethod)
	{
	case DFT_WIN_RECT:
		vecrDFTWindow = Ones(iNumIntpFreqPil);
		vecrDFTwindowInv = Ones(iNumCarrier);
		break;

	case DFT_WIN_HAMM:
		vecrDFTWindow = Hamming(iNumIntpFreqPil);
		vecrDFTwindowInv = (CReal) 1.0 / Hamming(iNumCarrier);
		break;
	}


	/* Set start index for zero padding in time domain for DFT method */
	iStartZeroPadding = (int) rGuardSizeFFT;
	if (iStartZeroPadding > iNumIntpFreqPil)
		iStartZeroPadding = iNumIntpFreqPil;

	/* Allocate memory for channel estimation */
	veccChanEst.Init(iNumCarrier);

	/* Allocate memory for history buffer (Matrix) and zero out */
	matcHistory.Init(iLenHistBuff, iNumCarrier,
		_COMPLEX((_REAL) 0.0, (_REAL) 0.0));

	/* After an initialization we do not put out data befor the number symbols
	   of the channel estimation delay have been processed */
	iInitCnt = iLenHistBuff - 1;

	/* Inits for SNR estimation (noise and signal averages) */
	rSNREstimate = (_REAL) pow(10.0, INIT_VALUE_SNR_ESTIM_DB / 10);
	rSignalEst = (_REAL) 0.0;
	rNoiseEst = (_REAL) 0.0;

	/* For SNR estimation initialization */
	iNumCellsSNRInit = iNumSymPerFrame * iNumCarrier; /* 1 DRM frame */
	iSNREstInitCnt = iNumCellsSNRInit;
	bWasSNRInit = TRUE;

	/* Lambda for IIR filter */
	rLamSNREstFast = IIR1Lam(TICONST_SNREST_FAST, (CReal) SOUNDCRD_SAMPLE_RATE /
		ReceiverParam.iSymbolBlockSize);
	rLamSNREstSlow = IIR1Lam(TICONST_SNREST_SLOW, (CReal) SOUNDCRD_SAMPLE_RATE /
		ReceiverParam.iSymbolBlockSize);


	/* SNR correction factor. We need this factor since we evalute the 
	   signal-to-noise ratio only on the pilots and these have a higher power as
	   the other cells */
	rSNRCorrectFact = 
		ReceiverParam.rAvPilPowPerSym /	ReceiverParam.rAvPowPerSymbol;

	/* Init delay spread length estimation (index) */
	rLenPDSEst = (_REAL) 0.0;

	/* Init maximum estimated delay spread and offset in one DRM frame */
	rMaxLenPDSInFra = (_REAL) 0.0;
	rMinOffsPDSInFra = rGuardSizeFFT;

	/* Inits for Wiener interpolation in frequency direction ---------------- */
	/* Length of wiener filter */
	switch (ReceiverParam.GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		iLengthWiener = LEN_WIENER_FILT_FREQ_RMA;
		break;

	case RM_ROBUSTNESS_MODE_B:
		iLengthWiener = LEN_WIENER_FILT_FREQ_RMB;
		break;

	case RM_ROBUSTNESS_MODE_E:
		iLengthWiener = LEN_WIENER_FILT_FREQ_RME;
		break;
	}


	/* Inits for wiener filter ---------------------------------------------- */
	/* In frequency direction we can use pilots from both sides for 
	   interpolation */
	iPilOffset = iLengthWiener / 2;

	/* Allocate memory */
	matcFiltFreq.Init(iNumCarrier, iLengthWiener);

	/* Pilot offset table */
	veciPilOffTab.Init(iNumCarrier);

	/* Number of different wiener filters */
	iNoWienerFilt = (iLengthWiener - 1) * iScatPilFreqInt + 1;

	/* Allocate temporary matlib vector for filter coefficients */
	matcWienerFilter.Init(iNoWienerFilt, iLengthWiener);

#ifdef UPD_WIENER_FREQ_EACH_DRM_FRAME
	/* Init Update counter for wiener filter update */
	iUpCntWienFilt = iNumSymPerFrame;
#endif

	/* Initial Wiener filter. Use initial SNR definition and assume that the
	   PDS ranges from the beginning of the guard-intervall to the end */
	UpdateWienerFiltCoef(pow(10.0, INIT_VALUE_SNR_WIEN_FREQ_DB / 10),
		(_REAL) ReceiverParam.RatioTgTu.iEnum / 
		ReceiverParam.RatioTgTu.iDenom, (CReal) 0.0);


	/* Define block-sizes for input and output */
	iInputBlockSize = iNumCarrier;
	iMaxOutputBlockSize = iNumCarrier; 
}

CComplexVector CChannelEstimation::FreqOptimalFilter(int iFreqInt, int iDiff, CReal rSNR, CReal rRatPDSLen, CReal rRatPDSOffs, int iLength)
{
	int				i = 0;  //init DM
	int				iCurPos = 0; //init DM
	CComplexVector	veccReturn(iLength);
	CComplexVector	veccRpp(iLength);
	CComplexVector	veccRhp(iLength);

	/* Calculation of R_hp, this is the SHIFTED correlation function */
	for (i = 0; i < iLength; i++)
	{
		iCurPos = i * iFreqInt - iDiff;

		veccRhp[i] = FreqCorrFct(iCurPos, rRatPDSLen, rRatPDSOffs);
	}

	/* Calculation of R_pp */
	for (i = 0; i < iLength; i++)
	{
		iCurPos = i * iFreqInt;

		veccRpp[i] = FreqCorrFct(iCurPos, rRatPDSLen, rRatPDSOffs);
	}

	/* Add SNR at first tap */
	veccRpp[0] += (CReal) 1.0 / rSNR;

	/* Call levinson algorithm to solve matrix system for optimal solution */
	veccReturn = Levinson(veccRpp, veccRhp);

	return veccReturn;
}

CComplex CChannelEstimation::FreqCorrFct(int iCurPos, CReal rRatPDSLen,
										 CReal rRatPDSOffs)
{
/* 
	We assume that the power delay spread is a rectangle function in the time
	domain (sinc-function in the frequency domain). Length and position of this
	window are adapted according to the current estimated PDS.
*/
	/* First calculate the argument of the sinc- and exp-function */
	const CReal rArgSinc = (CReal) iCurPos * rRatPDSLen;
	const CReal rArgExp =
		(CReal) crPi * iCurPos * (rRatPDSLen + rRatPDSOffs * 2);

	/* sinc(n * rat) * exp(pi * n * (rat + ratoffs)) */
	return Sinc(rArgSinc) * CComplex(Cos(rArgExp), Sin(rArgExp));
}

void CChannelEstimation::UpdateWienerFiltCoef(CReal rNewSNR, CReal rRatPDSLen, CReal rRatPDSOffs)
{
	int	j = 0, i = 0; //init DM
	int	iDiff = 0; //init DM
	int	iCurPil = 0; //init DM

	/* Calculate all possible wiener filters */
	for (j = 0; j < iNoWienerFilt; j++)
		matcWienerFilter[j] = FreqOptimalFilter(iScatPilFreqInt, j, rNewSNR,
			rRatPDSLen, rRatPDSOffs, iLengthWiener);


#if 0
#ifdef _DEBUG_
/* Save filter coefficients */
static FILE* pFile = fopen("test/wienerfreq.dat", "w");
for (j = 0; j < iNoWienerFilt; j++)
	for (i = 0; i < iLengthWiener; i++)
		fprintf(pFile, "%e\n", matcWienerFilter[j][i]);
fflush(pFile);
#endif
#endif



	/* Set matrix with filter taps, one filter for each carrier */
	for (j = 0; j < iNumCarrier; j++)
	{
		/* We define the current pilot position as the last pilot which the
		   index "j" has passed */
		iCurPil = (int) (j / iScatPilFreqInt);

		/* Consider special cases at the edges of the DRM spectrum */
		if (iCurPil < iPilOffset)
		{
			/* Special case: left edge */
			veciPilOffTab[j] = 0;
		}
		else if (iCurPil - iPilOffset > iNumIntpFreqPil - iLengthWiener)
		{
			/* Special case: right edge */
			veciPilOffTab[j] = iNumIntpFreqPil - iLengthWiener;
		}
		else
		{
			/* In the middle */
			veciPilOffTab[j] = iCurPil - iPilOffset;
		}

		/* Special case for robustness mode D, since the DC carrier is not used
		   as a pilot and therefore we use the same method for the edges of the
		   spectrum also in the middle of robustness mode D */
		if (iDCPos != 0)
		{
			if ((iDCPos - iCurPil < iLengthWiener) && (iDCPos - iCurPil > 0))
			{
				/* Left side of DC carrier */
				veciPilOffTab[j] = iDCPos - iLengthWiener;
			}

			if ((iCurPil - iDCPos < iLengthWiener) && (iCurPil - iDCPos > 0))
			{
				/* Right side of DC carrier */
				veciPilOffTab[j] = iDCPos;
			}
		}

		/* Difference between the position of the first pilot (for filtering)
		   and the position of the observed carrier */
		iDiff = j - veciPilOffTab[j] * iScatPilFreqInt;

		/* Copy correct filter in matrix */
		for (i = 0; i < iLengthWiener; i++)
			matcFiltFreq[j][i] = matcWienerFilter[iDiff][i];
	}
}

CReal CChannelEstimation::TentativeFACDec(const CComplex cCurRec) const
{
/* 
	Get tentative decision for this FAC QAM symbol. FAC is always 4-QAM.
	First calculate all distances to the four possible constellation points
	of a 4-QAM
*/
	/* Real axis minimum distance */
	const CReal rDistReal = Min(
		Abs(rTableQAM4[0][0] - Real(cCurRec)),
		Abs(rTableQAM4[1][0] - Real(cCurRec)));

	/* Imaginary axis minimum distance */
	const CReal rDistImag = Min(
		Abs(rTableQAM4[0][1] - Imag(cCurRec)),
		Abs(rTableQAM4[1][1] - Imag(cCurRec)));

	/* Return squared minimum distance */
	return SqMag(CComplex(rDistReal, rDistImag));
}

_REAL CChannelEstimation::GetSNREstdB() const
{
	/* Bound the SNR at 0 dB */
	if (rSNREstimate * rSNRCorrectFact > (_REAL) 1.0)
		return 10 * log10(rSNREstimate * rSNRCorrectFact);
	else
		return (_REAL) 0.0;
}

_REAL CChannelEstimation::GetDelay() const
{
	/* Delay in ms */
	return rLenPDSEst * iFFTSizeN / 
		(SOUNDCRD_SAMPLE_RATE * iNumIntpFreqPil * iScatPilFreqInt) * 1000;
}

void CChannelEstimation::GetTransferFunction(CVector<_REAL>& vecrData, CVector<_REAL>& vecrGrpDly)
{
	/* Init output vectors */
	vecrData.Init(iNumCarrier, (_REAL) 0.0);
	vecrGrpDly.Init(iNumCarrier, (_REAL) 0.0);

	/* Do copying of data only if vector is of non-zero length which means that
	   the module was already initialized */
	if (iNumCarrier != 0)
	{
		_REAL rDiffPhase, rOldPhase;

		/* Lock resources */
		Lock();

		/* Init constant for normalization */
		const _REAL rTu = (CReal) iFFTSizeN / SOUNDCRD_SAMPLE_RATE;

		/* Copy data in output vector and set scale 
		   (carrier index as x-scale) */
		for (int i = 0; i < iNumCarrier; i++)
		{
			/* Transfer function */
			CReal rNormChanEst = Abs(veccChanEst[i]) / (CReal) iNumCarrier;
				
			if (rNormChanEst > 0)
				vecrData[i] = (CReal) 20.0 * Log10(rNormChanEst);
			else
				vecrData[i] = RET_VAL_LOG_0;

			/* Group delay */
			if (i == 0)
			{
				/* At position 0 we cannot calculate a derivation -> use
				   the same value as position 0 */
				rDiffPhase = Angle(veccChanEst[1]) - Angle(veccChanEst[0]);
			}
			else
				rDiffPhase = Angle(veccChanEst[i]) - rOldPhase;

			/* Take care of wrap around of angle() function */
			if (rDiffPhase > WRAP_AROUND_BOUND_GRP_DLY)
				rDiffPhase -= 2.0 * crPi;
			if (rDiffPhase < -WRAP_AROUND_BOUND_GRP_DLY)
				rDiffPhase += 2.0 * crPi;

			/* Apply normalization */
			vecrGrpDly[i] = rDiffPhase * rTu * 1000.0 /* ms */;

			/* Store old phase */
			rOldPhase = Angle(veccChanEst[i]);

		}

		/* Release resources */
		Unlock();
	}
}

void CChannelEstimation::GetAvPoDeSp(CVector<_REAL>& vecrData,
									 CVector<_REAL>& vecrScale,
									 _REAL& rLowerBound, _REAL& rHigherBound,
									 _REAL& rStartGuard, _REAL& rEndGuard,
									 _REAL& rPDSBegin, _REAL& rPDSEnd)
{
	/* Lock resources */
	Lock();

	TimeSyncTrack.GetAvPoDeSp(vecrData, vecrScale, rLowerBound,
		rHigherBound, rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);

	/* Release resources */
	Unlock();
}
