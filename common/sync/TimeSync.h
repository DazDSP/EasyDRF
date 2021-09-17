/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See TimeSync.cpp
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

#if !defined(TIMESYNC_H__3B0BEVJBN872345NBEROUEBGF4344_BB27912__INCLUDED_)
#define TIMESYNC_H__3B0BEVJBN872345NBEROUEBGF4344_BB27912__INCLUDED_

#include "../Parameter.h"
#include "../Modul.h"
#include "../Vector.h"
#include "../matlib/Matlib.h"
#include "TimeSyncFilter.h"


/* Definitions ****************************************************************/

#define MAX_TIMING_CORRECTIONS			3
#define MAX_ROBMODE_CORRECTIONS			2

#define LAMBDA_LOW_PASS_START			((CReal) 0.99)
#define TIMING_BOUND_ABS				150

/* Non-linear correction of the timing if variation is too big */
#define NUM_SYM_BEFORE_RESET			5

/* Definitions for robustness mode detection */
#define NUM_BLOCKS_FOR_RM_CORR			16
#define THRESHOLD_RELI_MEASURE			((CReal) 8.0)

/* The guard-interval correlation is only updated every "STEP_SIZE_GUARD_CORR"
   samples to save computations */
#define STEP_SIZE_GUARD_CORR			4

/* "GRDCRR_DEC_FACT": Downsampling factor. We only use approx. 6 [12] kHz for
   correlation, therefore we can use a decimation of 8 [4]
   (i.e., 48 kHz / 8 [4] = 6 [12] kHz). Must be 8 [4] since all symbol and
   guard-interval lengths at 48000 for all robustness modes are dividable
   by 8 [4] */
//# define GRDCRR_DEC_FACT				4
# define GRDCRR_DEC_FACT				8
# define NUM_TAPS_HILB_FILT				NUM_TAPS_HILB_FILT_2_5
# define HILB_FILT_BNDWIDTH				HILB_FILT_BNDWIDTH_2_5
static float* fHilLPProt =				fHilLPProt2_5;

#ifdef USE_FRQOFFS_TRACK_GUARDCORR
/* Time constant for IIR averaging of frequency offset estimation */
# define TICONST_FREQ_OFF_EST_GUCORR	((CReal) 60.0) /* sec */
#endif


/* Classes ********************************************************************/
class CTimeSync : public CReceiverModul<_REAL, _REAL>
{
public:
	CTimeSync() : iTimeSyncPos(0), bTimingAcqu(FALSE),
		bAcqWasActive(FALSE), bRobModAcqu(FALSE),
		iLengthIntermCRes(NUM_ROBUSTNESS_MODES),
		iPosInIntermCResBuf(NUM_ROBUSTNESS_MODES),
		iLengthOverlap(NUM_ROBUSTNESS_MODES),
		iLenUsefPart(NUM_ROBUSTNESS_MODES),
		iLenGuardInt(NUM_ROBUSTNESS_MODES),
		cGuardCorr(NUM_ROBUSTNESS_MODES),
		rGuardPow(NUM_ROBUSTNESS_MODES),
		cGuardCorrBlock(NUM_ROBUSTNESS_MODES),
		rGuardPowBlock(NUM_ROBUSTNESS_MODES),
		rLambdaCoAv((CReal) 1.0) {}
	virtual ~CTimeSync() {}

	void StartAcquisition();
	void StopTimingAcqu() {bTimingAcqu = FALSE;}
	void StopRMDetAcqu() {bRobModAcqu = FALSE;}
	void SetFilterTaps(CReal rNewOffsetNorm);
	_BOOLEAN IsMaxCorr() {return bMaxCorr;}

protected:
	int							iCorrCounter;
	int							iAveCorr;
	int							iStepSizeGuardCorr;

	CShiftRegister<_REAL>		HistoryBuf;
	CShiftRegister<_COMPLEX>	HistoryBufCorr;
	CShiftRegister<_REAL>		pMaxDetBuffer;
	CRealVector					vecrHistoryFilt;
	CRealVector					pMovAvBuffer;

	CRealVector					vecCorrAvBuf;
	int							iCorrAvInd;

	int							iMaxDetBufSize;
	int							iCenterOfMaxDetBuf;

	int							iMovAvBufSize;
	int							iTotalBufferSize;
	int							iSymbolBlockSize;
	int							iDecSymBS;
	int							iGuardSize;
	int							iTimeSyncPos;
	int							iDFTSize;
	CReal						rStartIndex;

	int							iPosInMovAvBuffer;
	CReal						rGuardEnergy;

	int							iCenterOfBuf;

	_BOOLEAN					bMaxCorr;
	int							iTimeCorr;
	int							iRobCorr;

	_BOOLEAN					bInitTimingAcqu;
	_BOOLEAN					bTimingAcqu;
	_BOOLEAN					bRobModAcqu;
	_BOOLEAN					bAcqWasActive;

	int							iSelectedMode;

	CRealVector					rvecZ;
	CComplexVector				cvecB;
	CVector<_COMPLEX>			cvecOutTmpInterm;

	CReal						rLambdaCoAv;


	/* Intermediate correlation results and robustness mode detection */
	CComplexVector				veccIntermCorrRes[NUM_ROBUSTNESS_MODES];
	CRealVector					vecrIntermPowRes[NUM_ROBUSTNESS_MODES];
	CVector<int>				iLengthIntermCRes;
	CVector<int>				iPosInIntermCResBuf;
	CVector<int>				iLengthOverlap;
	CVector<int>				iLenUsefPart;
	CVector<int>				iLenGuardInt;

	CComplexVector				cGuardCorr;
	CComplexVector				cGuardCorrBlock;
	CRealVector					rGuardPow;
	CRealVector					rGuardPowBlock;

	CRealVector					vecrRMCorrBuffer[NUM_ROBUSTNESS_MODES];
	CRealVector					vecrCos[NUM_ROBUSTNESS_MODES];
	int							iRMCorrBufSize;

#ifdef USE_FRQOFFS_TRACK_GUARDCORR
	CShiftRegister<_COMPLEX>	HistoryBufTrGuCorr;
	CComplex					cCurExp;
	CComplex					cFreqOffAv;
	CReal						rLamFreqOff;
	CReal						rNormConstFOE;
#endif

	int GetIndFromRMode(ERobMode eNewMode);
	ERobMode GetRModeFromInd(int iNewInd);

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(TIMESYNC_H__3B0BEVJBN872345NBEROUEBGF4344_BB27912__INCLUDED_)
