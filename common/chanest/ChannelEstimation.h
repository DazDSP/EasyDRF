/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See ChannelEstimation.cpp
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

#if !defined(CHANEST_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define CHANEST_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../Parameter.h"
#include "../Modul.h"
#include "../ofdmcellmapping/OFDMCellMapping.h"
#include "../tables/TableQAMMapping.h"
#include "../matlib/Matlib.h"
#include "TimeLinear.h"
#include "TimeWiener.h"

#ifdef HAVE_DFFTW_H
# include <dfftw.h>
#else
# include "../libs/fftw.h"
#endif

#include "../sync/TimeSyncTrack.h"


/* Definitions ****************************************************************/
#define LEN_WIENER_FILT_FREQ_RMA		6 //was 6
#define LEN_WIENER_FILT_FREQ_RMB		4 //was 11 * Length 4 tracks rapid fading better DM
#define LEN_WIENER_FILT_FREQ_RME		11 //was 11

/* Time constant for IIR averaging of fast signal power estimation */
#define TICONST_SNREST_FAST				((CReal) 3.0) /* sec */ //Edited DM - was 30

/* Time constant for IIR averaging of slow signal power estimation */
#define TICONST_SNREST_SLOW				((CReal) 10.0) /* sec */ //Edited DM was 100

/* Initial value for SNR */
#define INIT_VALUE_SNR_WIEN_FREQ_DB		((_REAL) 10.0) /* dB */ //was 10.0

/* SNR estimation initial SNR value */
#define INIT_VALUE_SNR_ESTIM_DB			((_REAL) 10.0) /* dB */ //was 10.0

/* If this flag is set, the Wiener filter coefficients are updated every DRM
   frame. If not, the update is done every OFDM symbol. If the update is only
   done every DRM frame, the processing power is lower but if the timing
   changes quickly it can happen that the PDS moves out of the estimated
   window (defined by beginning and end value) */
#undef UPD_WIENER_FREQ_EACH_DRM_FRAME

/* Wrap around bound for calculation of group delay. It is wrapped by the 2 pi
   periodicity of the angle() function */
#define WRAP_AROUND_BOUND_GRP_DLY		((_REAL) 4.0) //was 4.0

/* Classes ********************************************************************/
class CChannelEstimation : public CReceiverModul<_COMPLEX, CEquSig>
{
public:
	CChannelEstimation() : iLenHistBuff(0), TypeIntFreq(FWIENER), 
		TypeIntTime(TWIENER), eDFTWindowingMethod(DFT_WIN_HAMM),
		TypeSNREst(SNR_PIL) {}
	virtual ~CChannelEstimation() {}

	enum ETypeIntFreq {FLINEAR, FDFTFILTER, FWIENER};
	enum ETypeIntTime {TLINEAR, TWIENER, TDECIDIR};
	enum ETypeSNREst {SNR_FAC, SNR_PIL};

	void GetTransferFunction(CVector<_REAL>& vecrData, 
		CVector<_REAL>& vecrGrpDly);
	void GetAvPoDeSp(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, 
					 _REAL& rLowerBound, _REAL& rHigherBound,
					 _REAL& rStartGuard, _REAL& rEndGuard, _REAL& rPDSBegin,
					 _REAL& rPDSEnd);


	CTimeLinear* GetTimeLinear() {return &TimeLinear;}
	CTimeWiener* GetTimeWiener() {return &TimeWiener;}
	CTimeSyncTrack* GetTimeSyncTrack() {return &TimeSyncTrack;}

	/* Set (get) frequency and time interpolation algorithm */
	void SetFreqInt(ETypeIntFreq eNewTy) {TypeIntFreq = eNewTy;}
	ETypeIntFreq GetFreqInt() {return TypeIntFreq;}
	void SetTimeInt(ETypeIntTime eNewTy) {TypeIntTime = eNewTy;
		SetInitFlag();}
	ETypeIntTime GetTimeInt() const {return TypeIntTime;}

	/* Which SNR estimation algorithm */
	void SetSNREst(ETypeSNREst eNewTy) {TypeSNREst = eNewTy;}
	ETypeSNREst GetSNREst() {return TypeSNREst;}

	_REAL GetSNREstdB() const;
	_REAL GetSigma() {return TimeWiener.GetSigma();}
	_REAL GetDelay() const;

	void StartSaRaOffAcq() {TimeSyncTrack.StartSaRaOffAcq(); SetInitFlag();}

protected:
	enum EDFTWinType {DFT_WIN_RECT, DFT_WIN_HAMM};
	EDFTWinType			eDFTWindowingMethod;

	int					iNumSymPerFrame;

	CChanEstTime*		pTimeInt;

	CTimeLinear			TimeLinear;
	CTimeWiener			TimeWiener;

	CTimeSyncTrack		TimeSyncTrack;

	ETypeIntFreq		TypeIntFreq;
	ETypeIntTime		TypeIntTime;
	ETypeSNREst			TypeSNREst;

	int					iNumCarrier;

	CMatrix<_COMPLEX>	matcHistory;

	int					iLenHistBuff;

	int					iScatPilFreqInt; /* Frequency interpolation */
	int					iScatPilTimeInt; /* Time interpolation */

	CComplexVector		veccChanEst;

	int					iFFTSizeN;

	CReal				rGuardSizeFFT;

	CRealVector			vecrDFTWindow;
	CRealVector			vecrDFTwindowInv;

	int					iLongLenFreq;
	CComplexVector		veccPilots;
	CComplexVector		veccIntPil;
	CFftPlans			FftPlanShort;
	CFftPlans			FftPlanLong;

	int					iNumIntpFreqPil;

	CReal				rLamSNREstFast;
	CReal				rLamSNREstSlow;

	_REAL				rNoiseEst;
	_REAL				rSignalEst;
	_REAL				rSNREstimate;
	_REAL				rSNRCorrectFact;
	int					iUpCntWienFilt;

	_REAL				rLenPDSEst; /* Needed for GetDelay() */
	_REAL				rMaxLenPDSInFra;
	_REAL				rMinOffsPDSInFra;

	int					iStartZeroPadding;

	CReal				TentativeFACDec(const CComplex cCurRec) const;

	/* Wiener interpolation in frequency direction */
	void UpdateWienerFiltCoef(CReal rNewSNR, CReal rRatPDSLen,
							  CReal rRatPDSOffs);

	CComplexVector FreqOptimalFilter(int iFreqInt, int iDiff, CReal rSNR, 
									 CReal rRatPDSLen, CReal rRatPDSOffs,
									 int iLength);
	CComplex FreqCorrFct(int iCurPos, CReal rRatPDSLen, CReal rRatPDSOffs);
	CMatrix<_COMPLEX>	matcFiltFreq;
	int					iLengthWiener;
	CVector<int>		veciPilOffTab;

	int					iDCPos;
	int					iPilOffset;
	int					iNoWienerFilt;
	CComplexMatrix		matcWienerFilter;

	int					iInitCnt;
	int					iSNREstInitCnt;
	int					iNumCellsSNRInit;
	_BOOLEAN			bWasSNRInit;

	
	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(CHANEST_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
