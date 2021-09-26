/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *
 * Description:
 *	See DrmReceiver.cpp
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

#if !defined(DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include <iostream>
#include "Parameter.h"
#include "Buffer.h"
#include "Data.h"
#include "OFDM.h"
#include "DRMSignalIO.h"
#include "MSCMultiplexer.h"
#include "InputResample.h"
#include "datadecoding/DataDecoder.h"
#include "sourcedecoders/AudioSourceDecoder.h"
#include "mlc/MLC.h"
#include "interleaver/SymbolInterleaver.h"
#include "ofdmcellmapping/OFDMCellMapping.h"
#include "chanest/ChannelEstimation.h"
#include "sync/FreqSyncAcq.h"
#include "sync/TimeSync.h"
#include "sync/SyncUsingPil.h"
#include "../sound/sound.h"


/* Definitions ****************************************************************/
/* Number of FAC frames until the acquisition is activated in case a signal
   was successfully decoded */
#define	NUM_FAC_FRA_U_ACQ_WITH			12		//12*400ms = 4.8 sec

/* Number of FAC frames until the acquisition is activated in case no signal
   could be decoded after previous acquisition try */
#define	NUM_FAC_FRA_U_ACQ_WITHOUT_S		200		//150*27ms = 5.4 sec
#define	NUM_FAC_FRA_U_ACQ_WITHOUT_F		100		// 75*27ms = 2.7 sec

/* Number of FAC blocks for delayed tracking mode switch (caused by time needed
   for initalizing the channel estimation */
#define NUM_FAC_DEL_TRACK_SWITCH		2


/* Classes ********************************************************************/
class CDRMReceiver
{
public:
	/* Acquisition state of receiver */
	enum EAcqStat {AS_NO_SIGNAL, AS_WITH_SIGNAL};

	/* Receiver state */
	enum ERecState {RS_TRACKING, RS_ACQUISITION};

	/* RM: Receiver mode (analog or digital demodulation) */
	enum ERecMode {RM_DRM, RM_AM, RM_NONE};


	//Added WriteData(&SoundInterface) DM
	CDRMReceiver() : eAcquiState(AS_NO_SIGNAL), iAcquDetecCnt(0),
		iGoodSignCnt(0), bWasFreqAcqu(TRUE), bDoInitRun(FALSE),
		eReceiverMode(RM_DRM), 	eNewReceiverMode(RM_NONE),
		ReceiveData(&SoundInterface), WriteData(&SoundInterface),
		rInitResampleOffset((_REAL) 0.0) {}
	virtual ~CDRMReceiver() {}

	/* For GUI */
	void					Init();
	void					Start();
	void					Stop();
	void					Rec();
	void					NotRec();
	EAcqStat				GetReceiverState() {return eAcquiState;}
	ERecMode				GetReceiverMode() {return eReceiverMode;}
	void					SetReceiverMode(ERecMode eNewMode)
								{eNewReceiverMode = eNewMode;}

	void					SetInitResOff(_REAL rNRO)
								{rInitResampleOffset = rNRO;}

	/* Get pointer to internal modules */
	CUtilizeFACData*		GetFAC() {return &UtilizeFACData;}
	CTimeSync*				GetTimeSync() {return &TimeSync;}
	CChannelEstimation*		GetChanEst() {return &ChannelEstimation;}
	CFACMLCDecoder*			GetFACMLC() {return &FACMLCDecoder;}
	CMSCMLCDecoder*			GetMSCMLC() {return &MSCMLCDecoder;}
	CReceiveData*			GetReceiver() {return &ReceiveData;}
	COFDMDemodulation*		GetOFDMDemod() {return &OFDMDemodulation;}
	CSyncUsingPil*			GetSyncUsPil() {return &SyncUsingPil;}
	CWriteData*				GetWriteData() {return &WriteData;}												   
	CSound*					GetSoundInterface() {return &SoundInterface;}
	CDataDecoder*			GetDataDecoder() {return &DataDecoder;}
	CAudioSourceDecoder*	GetAudSrcDec() {return &AudioSourceDecoder;}


	CParameter*				GetParameters() {return &ReceiverParam;}
	void					StartParameters(CParameter& Param);
	void					SetInStartMode();
	void					SetInTrackingMode();
	void					SetInTrackingModeDelayed();
	void					SetFreqAcqWinSize(_REAL rNewCenterFreq, _REAL rNewWinSize)
							{FreqSyncAcq.SetSearchWindow(rNewCenterFreq,rNewWinSize); }
	void					SetFastReset(_BOOLEAN bisfast) { bDoFastReset = bisfast; }

	void					InitsForAllModules();

	void					InitsForWaveMode();
	void					InitsForSpectrumOccup();
	void					InitsForAudParam();
	void					InitsForDataParam();
	void					InitsForInterlDepth();
	void					InitsForMSCCodSche();
	void					InitsForMSC();
	void					InitsForMSCDemux();

	int						GetCtR(void) { return iAcquRestartCnt; }
	int						GetCtA(void) { return iAcquDetecCnt; }
	EAcqStat				GetEAS(void) { return eAcquiState; }
	ERecState				GetERS(void) { return eReceiverState; }

protected:
	void					Run();
	void					DetectAcquiFAC();
	void					DetectAcquiSymbol();
	void					InitReceiverMode();

	/* Modules */
	CReceiveData			ReceiveData;
	CInputResample			InputResample;
	CFreqSyncAcq			FreqSyncAcq;
	CTimeSync				TimeSync;
	COFDMDemodulation		OFDMDemodulation;
	CSyncUsingPil			SyncUsingPil;
	CChannelEstimation		ChannelEstimation;
	COFDMCellDemapping		OFDMCellDemapping;
	CFACMLCDecoder			FACMLCDecoder;
	CUtilizeFACData			UtilizeFACData;
	CSymbDeinterleaver		SymbDeinterleaver;
	CMSCMLCDecoder			MSCMLCDecoder;
	CMSCDemultiplexer		MSCDemultiplexer;
	CAudioSourceDecoder		AudioSourceDecoder;
	CDataDecoder			DataDecoder;
	CWriteData				WriteData; //Added DM						 

	/* Parameters */
	CParameter				ReceiverParam;

	/* Buffers */
	CSingleBuffer<_REAL>	RecDataBuf;
	CCyclicBuffer<_REAL>	FreqSyncAcqBuf;
	CCyclicBuffer<_REAL>	InpResBuf;
	CSingleBuffer<_REAL>	TimeSyncBuf;
	CSingleBuffer<_COMPLEX>	OFDMDemodBuf;
	CSingleBuffer<_COMPLEX>	SyncUsingPilBuf;
	CSingleBuffer<CEquSig>	ChanEstBuf;
	CCyclicBuffer<CEquSig>	MSCCarDemapBuf;
	CCyclicBuffer<CEquSig>	FACCarDemapBuf;
	CSingleBuffer<CEquSig>	DeintlBuf;
	CSingleBuffer<_BINARY>	FACDecBuf;
	CSingleBuffer<_BINARY>	MSCMLCDecBuf;
	CSingleBuffer<_BINARY>	MSCDeMUXBufAud;
	CSingleBuffer<_BINARY>	MSCDeMUXBufData;
	CCyclicBuffer<_SAMPLE>	AudSoDecBuf;
	
	EAcqStat				eAcquiState;
	int						iAcquRestartCnt;
	int						iAcquDetecCnt;
	int						iGoodSignCnt;
	int						iDelayedTrackModeCnt;
	ERecState				eReceiverState;
	ERecMode				eReceiverMode;
	ERecMode				eNewReceiverMode;

	CSound					SoundInterface;

	_BOOLEAN				bWasFreqAcqu;
	_BOOLEAN				bDoInitRun;
	_BOOLEAN				bDoFastReset;

	_REAL					rInitResampleOffset;
};


#endif // !defined(DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
