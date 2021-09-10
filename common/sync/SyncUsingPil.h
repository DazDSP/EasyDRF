/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See SyncUsingPil.cpp
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

#if !defined(SYNCUSINGPIL_H__3B0BA660_CA63_434OBUVEE7A0D31912__INCLUDED_)
#define SYNCUSINGPIL_H__3B0BA660_CA63_434OBUVEE7A0D31912__INCLUDED_

#include "../Parameter.h"
#include "../Modul.h"
#include "../Vector.h"
#include "../chanest/ChanEstTime.h"


/* Definitions ****************************************************************/
/* Time constant for IIR averaging of frequency offset estimation */
#define TICONST_FREQ_OFF_EST			((CReal) 1.0) /* sec */

#ifdef USE_SAMOFFS_TRACK_FRE_PIL
/* Time constant for IIR averaging of sample rate offset estimation */
# define TICONST_SAMRATE_OFF_EST		((CReal) 20.0) /* sec */
# define CONTR_SAMP_OFF_INTEGRATION		((_REAL) 3.0)
#endif

/* If this macro is defined, the DRM frame synchronization based on the impulse
   response is used. The problem with this algorithm is that it does not perform
   well if the IR is very long and / or the DRM bandwidth is small (only a few
   scattered pilots are available)
   If this macro is not defined, the DRM frame synchronization based on the
   time pilots is used */
#undef USE_DRM_FRAME_SYNC_IR_BASED

#define NUM_FREQ_PILOTS			3

/* Classes ********************************************************************/
class CSyncUsingPil : public CReceiverModul<_COMPLEX, _COMPLEX>, 
					  public CPilotModiClass
{
public:
	CSyncUsingPil() :
#ifdef USE_SAMOFFS_TRACK_FRE_PIL
		cFreqPilotPhDiff(NUM_FREQ_PILOTS),
#endif
		bSyncInput(FALSE), bAquisition(FALSE), bTrackPil(FALSE),
		iSymbCntFraSy(0), iNumSymPerFrame(0),
		iPosFreqPil(NUM_FREQ_PILOTS), cOldFreqPil(NUM_FREQ_PILOTS) {}
	virtual ~CSyncUsingPil() {}

	/* To set the module up for synchronized DRM input data stream */
	void SetSyncInput(_BOOLEAN bNewS) {bSyncInput = bNewS;}

	void StartAcquisition();
	void StopAcquisition() {bAquisition = FALSE;}

	void StartTrackPil() {bTrackPil = TRUE;}
	void StopTrackPil() {bTrackPil = FALSE;}

protected:
	class CPilotCorr
	{
	public:
		int			iIdx1{}, iIdx2{}; //init DM
		CComplex	cPil1, cPil2;
	};

	/* Variables for frequency pilot estimation */
	CVector<int>			iPosFreqPil{}; //init DM
	CVector<CComplex>		cOldFreqPil{}; //init DM

	CReal					rNormConstFOE{}; //init DM
	CReal					rSampleFreqEst{}; //init DM

	int						iSymbCntFraSy{}; //init DM
	int						iInitCntFraSy{}; //init DM 

	int						iNumSymPerFrame{}; //init DM
	int						iNumCarrier{}; //init DM

	_BOOLEAN				bBadFrameSync{}; //init DM
	_BOOLEAN				bInitFrameSync{}; //init DM
	_BOOLEAN				bFrameSyncWasOK{}; //init DM

	_BOOLEAN				bSyncInput{}; //init DM

	_BOOLEAN				bAquisition{}; //init DM
	_BOOLEAN				bTrackPil{}; //init DM

	int						iMiddleOfInterval{}; //init DM

	CReal					rLamFreqOff{}; //init DM
	CComplex				cFreqOffVec{}; //init DM


	/* Variables for frame synchronization */
	CShiftRegister<CReal>	vecrCorrHistory;

#ifdef USE_DRM_FRAME_SYNC_IR_BASED
	/* DRM frame synchronization using impulse response */
	int						iNumPilInFirstSym;
	CComplexVector			veccChan;
	CRealVector				vecrTestImpResp;
	CFftPlans				FftPlan;
#else
	/* DRM frame synchronization based on time pilots */
	CVector<CPilotCorr>		vecPilCorr{}; //init DM
	int						iNumPilPairs{}; //init DM
	CComplex				cR_HH{}; //init DM
#endif

	ERobMode				eCurRobMode{}; //init DM

	CReal					rAvFreqPilDistToDC{}; //init DM
	CReal					rPrevSamRateOffset{}; //init DM

#ifdef USE_SAMOFFS_TRACK_FRE_PIL
	CReal					rLamSamRaOff;
	CVector<CComplex>		cFreqPilotPhDiff;
#endif

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(SYNCUSINGPIL_H__3B0BA660_CA63_434OBUVEE7A0D31912__INCLUDED_)
