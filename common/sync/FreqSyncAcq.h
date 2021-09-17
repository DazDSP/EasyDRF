/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See FreqSyncAcq.cpp
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

#if !defined(FREQSYNC_H__3B0BA660EDOINBEROUEBGF4344_BB2B_23E7912__INCLUDED_)
#define FREQSYNC_H__3B0BA660EDOINBEROUEBGF4344_BB2B_23E7912__INCLUDED_

#include "../Parameter.h"
#include "../Modul.h"
#include "../matlib/Matlib.h"

#ifdef HAVE_DRFFTW_H
# include <drfftw.h>
#else
# include "../libs/rfftw.h"
#endif


/* Definitions ****************************************************************/
/* Bound for peak detection between filtered signal (in frequency direction) 
   and the unfiltered signal */
#define PEAK_BOUND_FILT2SIGNAL_1		((CReal) 1.5)
#define PEAK_BOUND_FILT2SIGNAL_2		((CReal) 1.3)

/* This value MUST BE AT LEAST 2, because otherwise we would get an overrun
   when we try to add a complete symbol to the buffer! */
#ifdef USE_FRQOFFS_TRACK_GUARDCORR
# define NUM_BLOCKS_4_FREQ_ACQU			30 /* Accuracy must be higher */
#else
# define NUM_BLOCKS_4_FREQ_ACQU			6 //5
#endif

/* Number of blocks before using the average of input spectrum */
#define NUM_BLOCKS_BEFORE_US_AV			10

/* The average symbol duration of all possible robustness modes is 22.5 ms. A
   timeout of approx. 2 seconds corresponds from that to 100 */
#define AVERAGE_TIME_OUT_NUMBER			100

/* Ratio between highest and second highest peak at the frequency pilot
   positions in the PSD estimation (after peak detection) */
#define MAX_RAT_PEAKS_AT_PIL_POS		3 /* originally 2, -> 3db */


/* Classes ********************************************************************/
class CFreqSyncAcq : public CReceiverModul<_REAL, _REAL>
{
public:
	CFreqSyncAcq() : bAquisition(FALSE), 
		rWinSize((_REAL) 200),
		veciTableFreqPilots(3), /* 3 freqency pilots */
		rCenterFreq((_REAL) 350),
		rPeakBoundFiltToSig(PEAK_BOUND_FILT2SIGNAL_2) {}
	virtual ~CFreqSyncAcq() {}

	void SetSearchWindow(_REAL rNewCenterFreq, _REAL rNewWinSize);
	void SetSensivity(_REAL rNewSensivity);

	void StartAcquisition();
	void StopAcquisition() {bAquisition = FALSE;}
	_BOOLEAN GetAcquisition() {return bAquisition;}

protected:
	CVector<int>				veciTableFreqPilots;
	CShiftRegister<fftw_real>	vecrFFTHistory;

	CFftPlans					FftPlan;
	CRealVector					vecrFFTInput;
	CRealVector					vecrHann;
	CComplexVector				veccFFTOutput;

	int							iTotalBufferSize;

	int							iFFTSize;

	_BOOLEAN					bAquisition;

	int							iAquisitionCounter;

	_BOOLEAN					bFreqFound;

	_REAL						rCenterFreq;
	_REAL						rWinSize;
	int							iStartDCSearch;
	int							iEndDCSearch;
	_REAL						rPeakBoundFiltToSig;

	CRealVector					vecrPSDPilCor;
	CRealVector					vecrPSD;
	int							iHalfBuffer;
	int							iSearchWinSize;
	CRealVector					vecrFiltResLR;
	CRealVector					vecrFiltResRL;
	CRealVector					vecrFiltRes;
	CVector<int>				veciPeakIndex;
	int							iAverageCounter;

	int							iAverTimeOutCnt;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(FREQSYNC_H__3B0BA660EDOINBEROUEBGF4344_BB2B_23E7912__INCLUDED_)
