/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See TimeWiener.cpp
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

#if !defined(TIMEWIENER_LINEAR_H__3B0BA660_C345345_4344_D31912__INCLUDED_)
#define TIMEWIENER_LINEAR_H__3B0BA660_C345345_4344_D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../Vector.h"
#include "../ofdmcellmapping/OFDMCellMapping.h"
#include "../matlib/Matlib.h"
#include "ChanEstTime.h"


/* Definitions ****************************************************************/
/* Number of taps we want to use for sigma estimation */
#define NO_TAPS_USED4SIGMA_EST			3 //was 3

/* Lengths of wiener filter for wiener filtering in time direction */
#define LEN_WIENER_FILT_TIME_RMA		15
#define LEN_WIENER_FILT_TIME_RMB		20 //was 20
#define LEN_WIENER_FILT_TIME_RME		9

/* Maximum values for doppler for a specific robustness mode.
   Parameters found by looking at resulting filter coefficients. The values
   "rSigma" are set to the maximum possible doppler frequency which can be
   interpolated by the pilot frequency grid. Since we have a Gaussian
   power spectral density, the power is never exactely zero. Therefore we
   determine the point where the PDS has fallen below a 50 dB limit */
#define MAX_SIGMA_RMA					((_REAL) 1.6 /* Hz */ / 2)
#define MAX_SIGMA_RMB					((_REAL) 2.7 /* Hz */ / 2)
#define MAX_SIGMA_RME					((_REAL) 4.0 /* Hz */ / 2)

/* Define a lower bound for the doppler */
#define LOW_BOUND_SIGMA					((_REAL) 0.1 /* Hz */ / 2)

/* Initial value for SNR */
#define INIT_VALUE_SNR_WIEN_TIME_DB		((_REAL) 25.0) /* dB */ //was 25.0

/* Time constant for IIR averaging of time correlation estimation */
#define TICONST_TI_CORREL_EST			((CReal) 60.0) /* sec */ //was 60.0

/* Overestimation factor for sigma estimation.
   We overestimate the sigma since the channel estimation result is much worse
   if we use a sigma which is small than the real one compared to a value
   which is bigger than the real one. Since we can only estimate on sigma for
   all paths, it can happen that a path with a small gain but a high doppler
   does not contribute enough on the global sigma estimation. Therefore the
   overestimation */
#define SIGMA_OVERESTIMATION_FACT		((_REAL) 3.0)


/* Classes ********************************************************************/
class CTimeWiener : public CChanEstTime
{
public:
	CTimeWiener() : bTracking(FALSE) {}
	virtual ~CTimeWiener() {}

	virtual int Init(CParameter& ReceiverParam);
	virtual _REAL Estimate(CVectorEx<_COMPLEX>* pvecInputData, 
						   CComplexVector& veccOutputData, 
						   CVector<int>& veciMapTab, 
						   CVector<_COMPLEX>& veccPilotCells, _REAL rSNR);

	_REAL GetSigma() {return rSigma * 2;}

	void StartTracking() {bTracking = TRUE;}
	void StopTracking() {bTracking = FALSE;}
	
protected:
	CReal TimeOptimalFilter(CRealVector& vecrTaps, const int iTimeInt, 
							const int iDiff, const CReal rNewSNR, 
							const CReal rNewSigma, const CReal rTs, 
							const int iLength);
	int DisToNextPil(const int iPiHiIndex, const int iSymNum) const;
	_REAL UpdateFilterCoef(_REAL rNewSNR, _REAL rNewSigma);
	CReal ModLinRegr(CComplexVector& veccCorrEst);

	int					iNumCarrier;

	int					iLengthWiener;
	int					iNoFiltPhasTi;
	CRealMatrix			matrFiltTime;
	
	CMatrix<_COMPLEX>	matcChanAtPilPos;

	CComplexVector		veccTiCorrEst;
	CReal				rLamTiCorrAv;

	int					iScatPilFreqInt; /* Frequency interpolation */
	int					iScatPilTimeInt; /* Time interpolation */
	int					iNumSymPerFrame;

	/* Number of first symbol with pilot at carrier-number 0 */
	int					iFirstSymbWithPi;

	int					iLenHistBuff;

	CShiftRegister<int>	vecTiCorrHist;
	int					iLenTiCorrHist;
	int					iNumTapsSigEst;
	int					iUpCntWienFilt;

	_REAL				Ts;
	_REAL				rSigma;
	_REAL				rSigmaMax;

	_REAL				rMMSE;
	_REAL				rAvSNR;
	int					iAvSNRCnt;

	_BOOLEAN			bTracking;
};


#endif // !defined(TIMEWIENER_LINEAR_H__3B0BA660_C345345_4344_D31912__INCLUDED_)
