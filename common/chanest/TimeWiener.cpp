/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Wiener filter in time direction for channel estimation
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

#include "TimeWiener.h"


/* Implementation *************************************************************/
_REAL CTimeWiener::Estimate(CVectorEx<_COMPLEX>* pvecInputData,
						    CComplexVector& veccOutputData,
						    CVector<int>& veciMapTab,
						    CVector<_COMPLEX>& veccPilotCells, _REAL rSNR)
{
	int			j, i;
	int			iPiHiIndex;
	int			iCurrFiltPhase;
	int			iTimeDiffNew;
	_COMPLEX	cNewPilot;

	/* Timing correction history -------------------------------------------- */
	/* Shift old vaules and add a "0" at the beginning of the vector */
	vecTiCorrHist.AddBegin(0);

	/* Add new one to all history values except of the current one */
	for (i = 1; i < iLenTiCorrHist; i++)
		vecTiCorrHist[i] += (*pvecInputData).GetExData().iCurTimeCorr;


	/* Update histories for channel estimates at the pilot positions -------- */
	for (i = 0; i < iNumCarrier; i++)
	{
		/* Identify and calculate transfer function at the pilot positions */
		if (_IsScatPil(veciMapTab[i]))
		{
			/* Pilots are only every "iScatPilFreqInt"'th carrier. It is not
			   possible just to increase the "iPiHiIndex" because not in all
			   cases a pilot is at position zero in "matiMapTab[]" */
			iPiHiIndex = i / iScatPilFreqInt;

			/* Save channel estimates at the pilot positions for each carrier
			   Move old estimates and put new value. Use reversed order to
			   prepare vector for convolution */
			for (j = iLengthWiener - 1; j > 0; j--)
				matcChanAtPilPos[j][iPiHiIndex] =
					matcChanAtPilPos[j - 1][iPiHiIndex];

			/* Add new channel estimate: h = r / s, h: transfer function of the
			   channel, r: received signal, s: transmitted signal */
			matcChanAtPilPos[0][iPiHiIndex] =
				(*pvecInputData)[i] / veccPilotCells[i];


			/* Estimation of the channel correlation function --------------- */
			/* We calcuate the estimation for one symbol first and average this
			   result */
			for (j = 0; j < iNumTapsSigEst; j++)
			{
				/* Correct pilot information for phase rotation */
				iTimeDiffNew = vecTiCorrHist[iScatPilTimeInt * j];
				cNewPilot = Rotate(matcChanAtPilPos[j][iPiHiIndex], i,
					iTimeDiffNew);

				/* Use IIR filtering for averaging */
				IIR1(veccTiCorrEst[j],
					Conj(matcChanAtPilPos[0][iPiHiIndex]) * cNewPilot,
					rLamTiCorrAv);
			}
		}
	}


	/* Update sigma estimation ---------------------------------------------- */
	if (bTracking == TRUE)
	{
		/* Update filter coefficients once in one DRM frame */
		if (iUpCntWienFilt > 0)
		{
			iUpCntWienFilt--;

			/* Average estimated SNR values */
			rAvSNR += rSNR;
			iAvSNRCnt++;
		}
		else
		{
			/* Actual estimation of sigma */
			rSigma = ModLinRegr(veccTiCorrEst);

			/* Use overestimated sigma for filter update */
			_REAL rSigOverEst = rSigma * SIGMA_OVERESTIMATION_FACT;

			/* Update the wiener filter, use averaged SNR */
			if (rSigOverEst < rSigmaMax)
				rMMSE = UpdateFilterCoef(rAvSNR / iAvSNRCnt, rSigOverEst);
			else
				rMMSE = UpdateFilterCoef(rAvSNR / iAvSNRCnt, rSigmaMax);

			/* If no SNR improvent is achieved by the optimal filter, use
			   SNR estimation for MMSE */
			_REAL rNewSNR = (_REAL) 1.0 / rMMSE;
			if (rNewSNR < rSNR)
				rMMSE = (_REAL) 1.0 / rSNR;

			/* Reset counter and sum (for SNR) */
			iUpCntWienFilt = iNumSymPerFrame;
			iAvSNRCnt = 0;
			rAvSNR = (_REAL) 0.0;
		}
	}


	/* Wiener interpolation, filtering and prediction ----------------------- */
	for (i = 0; i < iNumCarrier; i += iScatPilFreqInt)
	{
		/* This check is for robustness mode D since "iScatPilFreqInt" is "1"
		   in this case it would include the DC carrier in the for-loop */
		if (!_IsDC(veciMapTab[i]))
		{
			/* Pilots are only every "iScatPilFreqInt"'th carrier */
			iPiHiIndex = i / iScatPilFreqInt;

			/* Calculate current filter phase, use distance to next pilot */
			iCurrFiltPhase = (iScatPilTimeInt - DisToNextPil(iPiHiIndex,
				(*pvecInputData).GetExData().iSymbolID)) % iScatPilTimeInt;

			/* Convolution with one phase of the optimal filter */
			/* Init sum */
			_COMPLEX cCurChanEst = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
			for (j = 0; j < iLengthWiener; j++)
			{
				/* We need to correct pilots due to timing corrections ------ */
				/* Calculate timing difference */
				iTimeDiffNew =
					vecTiCorrHist[j * iScatPilTimeInt + iCurrFiltPhase] -
					vecTiCorrHist[iLenHistBuff - 1];

				/* Correct pilot information for phase rotation */
				cNewPilot = 
					Rotate(matcChanAtPilPos[j][iPiHiIndex], i, iTimeDiffNew);

				/* Actual convolution with filter phase */
				cCurChanEst +=
					cNewPilot * matrFiltTime[iCurrFiltPhase][j];
			}


			/* Copy channel estimation from current symbol in output buffer - */
			veccOutputData[iPiHiIndex] = cCurChanEst;
		}
	}

	/* Return the SNR improvement by wiener interpolation in time direction */
	return 1 / rMMSE;
}

int CTimeWiener::DisToNextPil(const int iPiHiIndex, const int iSymNum) const
{
	/* Distance to next pilot (later in time!) of one specific
	   carrier (with pilot). We do the "iNumSymPerFrame - iSymNum" to avoid
	   negative numbers in the modulo operation */
	return (iNumSymPerFrame - iSymNum + iFirstSymbWithPi + iPiHiIndex) %
		iScatPilTimeInt;
}

int CTimeWiener::Init(CParameter& ReceiverParam)
{
	int		iNoPiFreqDirAll;
	int		iSymDelyChanEst;
	int		iNumPilOneOFDMSym;
	int		iNumIntpFreqPil;
	_REAL	rSNR;

	/* Init base class, must be at the beginning of this init! */
	CPilotModiClass::InitRot(ReceiverParam);

	/* Set local parameters */
	iNumCarrier = ReceiverParam.iNumCarrier;
	iScatPilTimeInt = ReceiverParam.iScatPilTimeInt;
	iScatPilFreqInt = ReceiverParam.iScatPilFreqInt;
	iNumSymPerFrame = ReceiverParam.iNumSymPerFrame;
	iNumIntpFreqPil = ReceiverParam.iNumIntpFreqPil;

	/* We have to consider the last pilot at the end of the symbol ("+ 1") */
	iNoPiFreqDirAll = iNumCarrier / iScatPilFreqInt + 1;

	/* Init length of filter and maximum value of sigma (doppler) */
	switch (ReceiverParam.GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		iLengthWiener = LEN_WIENER_FILT_TIME_RMA;
		rSigmaMax = MAX_SIGMA_RMA;
		break;

	case RM_ROBUSTNESS_MODE_B:
		iLengthWiener = LEN_WIENER_FILT_TIME_RMB;
		rSigmaMax = MAX_SIGMA_RMB;
		break;

	case RM_ROBUSTNESS_MODE_E:
		iLengthWiener = LEN_WIENER_FILT_TIME_RME;
		rSigmaMax = MAX_SIGMA_RME;
		break;
	}

	/* Set delay of this channel estimation type. The longer the delay is, the
	   more "acausal" pilots can be used for interpolation. We use the same
	   amount of causal and acausal filter taps here */
	iSymDelyChanEst = (iLengthWiener / 2) * iScatPilTimeInt - 1;

	/* Set number of phases for wiener filter */
	iNoFiltPhasTi = iScatPilTimeInt;

	/* Set length of history-buffer */
	iLenHistBuff = iSymDelyChanEst + 1;

	/* Duration of useful part plus-guard interval */
	Ts = (_REAL) ReceiverParam.iSymbolBlockSize / SOUNDCRD_SAMPLE_RATE;

	/* Allocate memory for Channel at pilot positions (matrix) and init with
	   ones */
	matcChanAtPilPos.Init(iLengthWiener, iNoPiFreqDirAll,
		_COMPLEX((_REAL) 1.0, (_REAL) 0.0));

	/* Set number of taps for sigma estimation */
	if (iLengthWiener < NO_TAPS_USED4SIGMA_EST)
		iNumTapsSigEst = iLengthWiener;
	else
		iNumTapsSigEst = NO_TAPS_USED4SIGMA_EST;

	/* Init vector for estimation of the correlation function in time direction
	   (IIR average) */
	veccTiCorrEst.Init(iNumTapsSigEst, (CReal) 0.0);

	/* Init time constant for IIR filter for averaging correlation estimation.
	   Consider averaging over frequency axis, too. Pilots in frequency
	   direction are "iScatPilTimeInt * iScatPilFreqInt" apart */
	iNumPilOneOFDMSym = iNumIntpFreqPil / iScatPilTimeInt;
	rLamTiCorrAv = IIR1Lam(TICONST_TI_CORREL_EST * iNumPilOneOFDMSym,
		(CReal) SOUNDCRD_SAMPLE_RATE / ReceiverParam.iSymbolBlockSize);

	/* Init Update counter for wiener filter update. We immediatly use the
	   filtered result although right at the beginning there is no averaging.
	   But sine the estimation usually starts with bigger values and goes down
	   to the correct one, this should be not critical */
	iUpCntWienFilt = iNumSymPerFrame;

	/* Init averaging of SNR values */
	rAvSNR = (_REAL) 0.0;
	iAvSNRCnt = 0;


	/* Allocate memory for filter phases (Matrix) */
	matrFiltTime.Init(iNoFiltPhasTi, iLengthWiener);

	/* Length of the timing correction history buffer */
	iLenTiCorrHist = iLengthWiener * iNoFiltPhasTi;

	/* Init timing correction history with zeros */
	vecTiCorrHist.Init(iLenTiCorrHist, 0);

	/* Get the index of first symbol in a super-frame on where the first cell
	   (carrier-index = 0) is a pilot. This is needed for determining the
	   correct filter phase for the convolution */
	iFirstSymbWithPi = 0;
	while (!_IsScatPil(ReceiverParam.matiMapTab[iFirstSymbWithPi][0]))
		iFirstSymbWithPi++;


	/* Calculate optimal filter --------------------------------------------- */
	/* Distinguish between simulation and regular receiver. When we run a
	   simulation, the parameters are taken from simulation init */
	{
		/* Init SNR value */
		rSNR = pow(10, INIT_VALUE_SNR_WIEN_TIME_DB / 10);

		/* Init sigma with a very large value. This make the acquisition more
		   robust in case of a large sample frequency offset. But we get more
		   aliasing in the time domain and this could make the timing unit
		   perform worse. Therefore, this is only a trade-off */
		rSigma = rSigmaMax * 4;
	}

	/* Calculate initialization wiener filter taps and init MMSE */
	rMMSE = UpdateFilterCoef(rSNR, rSigma);

	/* Return delay of channel equalization */
	return iLenHistBuff;
}

_REAL CTimeWiener::UpdateFilterCoef(_REAL rNewSNR, _REAL rNewSigma)
{
	int		j;
	int		iCurrDiffPhase;
	_REAL	rMMSE;

	/* Calculate MMSE for wiener filtering for all phases and average */
	rMMSE = (_REAL) 0.0;

	/* One filter for all possible filter phases */
	for (j = 0; j < iNoFiltPhasTi; j++)
	{
		/* We have to define the dependency between the difference between the
		   current pilot to the observed symbol in the history buffer and the
		   indizes of the FiltTime array. Definition:
		   Largest distance = index zero, index increases to smaller
		   distances */
		iCurrDiffPhase = -(iLenHistBuff - j - 1);

		/* Calculate filter phase and average MMSE */
		rMMSE += TimeOptimalFilter(matrFiltTime[j], iScatPilTimeInt,
			iCurrDiffPhase,	rNewSNR, rNewSigma, Ts, iLengthWiener);
	}

#if 0
#ifdef _DEBUG_
/* Save filter coefficients */
static FILE* pFile = fopen("test/wienertime.dat", "w");
for (int i = 0; i < iLengthWiener; i++)
	for (j = 0; j < iNoFiltPhasTi; j++)
		fprintf(pFile, "%e\n", matrFiltTime[j][i]);
fflush(pFile);
#endif
#endif

	/* Normalize averaged MMSE */
	rMMSE /= iNoFiltPhasTi;

	return rMMSE;
}

CReal CTimeWiener::TimeOptimalFilter(CRealVector& vecrTaps, const int iTimeInt,
									 const int iDiff, const CReal rNewSNR,
									 const CReal rNewSigma, const CReal rTs,
									 const int iLength)
{
	int			i;
	CReal		rFactorArgExp;
	CReal		rMMSE;
	int			iCurPos;

	CRealVector	vecrReturn(iLength);
	CRealVector vecrRpp(iLength);
	CRealVector vecrRhp(iLength);

	/* Factor for the argument of the exponetial function to generate the
	   correlation function */
	rFactorArgExp = 
		(CReal) -2.0 * crPi * crPi * rTs * rTs * rNewSigma * rNewSigma;

	/* Doppler-spectrum for short-wave channel is Gaussian
	   (Calculation of R_hp!) */
	for (i = 0; i < iLength; i++)
	{
		iCurPos = i * iTimeInt + iDiff;

		vecrRhp[i] = exp(rFactorArgExp * iCurPos * iCurPos);
	}

	/* Doppler-spectrum for short-wave channel is Gaussian
	   (Calculation of R_pp!) */
	for (i = 0; i < iLength; i++)
	{
		iCurPos = i * iTimeInt;

		vecrRpp[i] = exp(rFactorArgExp * iCurPos * iCurPos);
	}

	/* Add SNR at first tap */
	vecrRpp[0] += (CReal) 1.0 / rNewSNR;

	/* Call levinson algorithm to solve matrix system for optimal solution */
	vecrTaps = Levinson(vecrRpp, vecrRhp);

	/* Calculate MMSE for the current wiener filter */
	rMMSE = (CReal) 1.0 - Sum(vecrRhp * vecrTaps);

	return rMMSE;
}

CReal CTimeWiener::ModLinRegr(CComplexVector& veccCorrEst)
{
	/* Modified linear regression to estimate the "sigma" of the Gaussian
	   correlation function */
	/* Get vector length */
	int iVecLen = Size(veccCorrEst);

	/* Init vectors and variables */
	CReal		rSigmaRet;
	CRealVector Tau(iVecLen);
	CRealVector Z(iVecLen);
	CRealVector W(iVecLen);
	CRealVector Wmrem(iVecLen);
	CReal		Wm, Zm;
	CReal		A1;

	/* Generate the tau vector */
	for (int i = 0; i < iVecLen; i++)
		Tau[i] = (CReal) (i * iScatPilTimeInt);

	/* Linearize acf equation:  y = a * exp(-b * x^2)
	   z = ln(y);   w = x^2
	   -> z = a0 + a1 * w */
	Z = Log(Abs(veccCorrEst));

	W = Tau * Tau;

	Wm = Mean(W);
	Zm = Mean(Z);

	Wmrem = W - Wm; /* Remove mean of W */

	A1 = Sum(Wmrem * (Z - Zm)) / Sum(Wmrem * Wmrem);

	/* Final sigma calculation from estimation and assumed Gaussian model */
	rSigmaRet = (CReal) 0.5 / crPi * sqrt((CReal) -2.0 * A1) / Ts;

	/* Bound estimated sigma value */
	if (rSigmaRet > rSigmaMax)
		rSigmaRet = rSigmaMax;
	if (rSigmaRet < LOW_BOUND_SIGMA)
		rSigmaRet = LOW_BOUND_SIGMA;

	return rSigmaRet;
}
