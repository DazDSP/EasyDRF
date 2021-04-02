/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *
 * Description:
 *	DRM-receiver
 * The hand over of data is done via an intermediate-buffer. The calling
 * convention is always "input-buffer, output-buffer". Additional, the
 * DRM-parameters are fed to the function.
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

#include "DrmReceiver.h"

BOOL DoNotRec = TRUE;
BOOL isfirstrx = FALSE;

/* Implementation *************************************************************/
void CDRMReceiver::Run()
{
	_BOOLEAN bEnoughData;

	/* Reset all parameters to start parameter settings */
	SetInStartMode();

	do
	{
		if (DoNotRec)
		{
			isfirstrx = TRUE;
			Sleep(500);
		}
		else
		{

			/* Check for parameter changes from GUI thread ---------------------- */
			/* The parameter changes are done through flags, the actual
			   initialization is done in this (the working) thread to avoid
			   problems with shared data */
			if (isfirstrx)
				InitReceiverMode();
			isfirstrx = FALSE;

			if (eNewReceiverMode != RM_NONE)
				InitReceiverMode();


			/* Receive data ----------------------------------------------------- */
			ReceiveData.ReadData(ReceiverParam, RecDataBuf);

			bEnoughData = TRUE;

			while (bEnoughData && ReceiverParam.bRunThread)
			{
				/* Init flag */
				bEnoughData = FALSE;

				{
					/* Resample input DRM-stream -------------------------------- */
					if (InputResample.ProcessData(ReceiverParam, RecDataBuf,
						InpResBuf))
					{
						bEnoughData = TRUE;
					}

					/* Frequency synchronization acquisition -------------------- */
					if (FreqSyncAcq.ProcessData(ReceiverParam, InpResBuf,
						FreqSyncAcqBuf))
					{
						bEnoughData = TRUE;

						if (FreqSyncAcq.GetAcquisition() == FALSE)
						{
							/* This flag ensures that the following functions are
							   called only once after frequency acquisition was
							   done */
							if (bWasFreqAcqu == TRUE)
							{
								/* Frequency acquisition is done, now the filter for
								   guard-interval correlation can be designed */
								TimeSync.SetFilterTaps(ReceiverParam.rFreqOffsetAcqui);
								bWasFreqAcqu = FALSE;
							}
						}
						else
							bWasFreqAcqu = TRUE;
					}

					/* Time synchronization ------------------------------------- */
					if (TimeSync.ProcessData(ReceiverParam, FreqSyncAcqBuf,
						TimeSyncBuf))
					{
						bEnoughData = TRUE;
						if (TimeSync.IsMaxCorr()) SetInStartMode();

						/* Use count of OFDM-symbols for detecting aquisition state */
						DetectAcquiSymbol();
					}

					/* OFDM-demodulation ---------------------------------------- */
					if (OFDMDemodulation.ProcessData(ReceiverParam, TimeSyncBuf,
						OFDMDemodBuf))
					{
						bEnoughData = TRUE;
					}

					/* Synchronization in the frequency domain (using pilots) --- */
					if (SyncUsingPil.ProcessData(ReceiverParam, OFDMDemodBuf,
						SyncUsingPilBuf))
					{
						bEnoughData = TRUE;
					}

					/* Channel estimation and equalisation ---------------------- */
					if (ChannelEstimation.ProcessData(ReceiverParam,
						SyncUsingPilBuf, ChanEstBuf))
					{
						bEnoughData = TRUE;
					}

					/* Demapping of the MSC, FAC and pilots off the carriers */
					if (OFDMCellDemapping.ProcessData(ReceiverParam, ChanEstBuf,
						MSCCarDemapBuf, FACCarDemapBuf))
					{
						bEnoughData = TRUE;
					}

					/* FAC ------------------------------------------------------ */
					if (FACMLCDecoder.ProcessData(ReceiverParam, FACCarDemapBuf,
						FACDecBuf))
					{
						bEnoughData = TRUE;
					}
					if (UtilizeFACData.WriteData(ReceiverParam, FACDecBuf))
					{
						bEnoughData = TRUE;

						/* Use information of FAC CRC for detecting the acquisition requirement */
						DetectAcquiFAC();
					}

					/* MSC ------------------------------------------------------ */
					/* Symbol de-interleaver */
					if (SymbDeinterleaver.ProcessData(ReceiverParam, MSCCarDemapBuf,
						DeintlBuf))
					{
						bEnoughData = TRUE;
					}

					/* MLC decoder */
					if (MSCMLCDecoder.ProcessData(ReceiverParam, DeintlBuf,
						MSCMLCDecBuf))
					{
						bEnoughData = TRUE;
					}

					/* MSC data/audio demultiplexer */
					if (MSCDemultiplexer.ProcessData(ReceiverParam,
						MSCMLCDecBuf, MSCDeMUXBufAud, MSCDeMUXBufData))
					{
						bEnoughData = TRUE;
					}

					/* Data decoding */
					if (DataDecoder.WriteData(ReceiverParam, MSCDeMUXBufData))
						bEnoughData = TRUE;

					/* Source decoding (audio) */
					if (AudioSourceDecoder.ProcessData(ReceiverParam, MSCDeMUXBufAud, AudSoDecBuf))
					{
						bEnoughData = TRUE;
					}
				}

				/* Save or dump the data */ //This is essential for the speech codecs to work on receive DM ========================
				if (WriteData.WriteData(ReceiverParam, AudSoDecBuf))
					bEnoughData = TRUE;
			}
		}
	} while (ReceiverParam.bRunThread && (!bDoInitRun));
}

void CDRMReceiver::DetectAcquiSymbol()
{

	/* Only for aquisition detection if no signal was decoded before */
	if (eAcquiState == AS_NO_SIGNAL)
	{
		/* Increment symbol counter and check if bound is reached */
		iAcquDetecCnt++;

		if (bDoFastReset)
		{
			if (iAcquDetecCnt > NUM_FAC_FRA_U_ACQ_WITHOUT_F) SetInStartMode();
		}
		else
		{
			if (iAcquDetecCnt > NUM_FAC_FRA_U_ACQ_WITHOUT_S) SetInStartMode();
		}
	}
}

void CDRMReceiver::DetectAcquiFAC()
{
	/* Acquisition switch */
	if (!UtilizeFACData.GetCRCOk())
	{
		/* Reset "good signal" count */
		iGoodSignCnt = 0;

		iAcquRestartCnt++;

		/* Check situation when receiver must be set back in start mode */
		if ((eAcquiState == AS_WITH_SIGNAL) &&
			(iAcquRestartCnt > NUM_FAC_FRA_U_ACQ_WITH))
		{
			SetInStartMode();
		}
	}
	else
	{
		/* Set the receiver state to "with signal" not until two successive FAC
		   frames are "ok", because there is only a 8-bit CRC which is not good
		   for many bit errors. But it is very unlikely that we have two
		   successive FAC blocks "ok" if no good signal is received */
		if (iGoodSignCnt > 0)
		{
			eAcquiState = AS_WITH_SIGNAL;

			/* Take care of delayed tracking mode switch */
			if (iDelayedTrackModeCnt > 0)
				iDelayedTrackModeCnt--;
			else
				SetInTrackingModeDelayed();
		}
		else
		{
			/* If one CRC was correct, reset acquisition since
			   we assume, that this was a correct detected signal */
			iAcquRestartCnt = 0;
			iAcquDetecCnt = 0;

			/* Set in tracking mode */
			SetInTrackingMode();

			iGoodSignCnt++;
		}
	}
}


void CDRMReceiver::Init()
{
	/* Set flags so that we have only one loop in the Run() routine which is
	   enough for initializing all modues */
	bDoInitRun = TRUE;
	ReceiverParam.bRunThread = TRUE;

	/* Set init flags in all modules */
	InitsForAllModules();

	/* Now the actual initialization */
	Run();

	/* Reset flags */
	bDoInitRun = FALSE;
	ReceiverParam.bRunThread = FALSE;
}

void CDRMReceiver::InitReceiverMode()
{
	eReceiverMode = eNewReceiverMode;

	/* Init all modules */
	SetInStartMode();

	/* Reset new mode flag */
	eNewReceiverMode = RM_NONE;
}

void CDRMReceiver::Start()
{
	/* Set run flag so that the thread can work */
	DoNotRec = FALSE;

	ReceiverParam.bRunThread = TRUE;

	Run();
}

void CDRMReceiver::Rec()
{
	DoNotRec = FALSE;
}
void CDRMReceiver::NotRec()
{
	DoNotRec = TRUE;
}

void CDRMReceiver::Stop()
{
	ReceiverParam.bRunThread = FALSE;
	try
	{
		SoundInterface.Close();
	}
	catch (CGenErr GenErr)
	{
	}
}

void CDRMReceiver::SetInStartMode()
{
	/* Load start parameters for all modules */
	StartParameters(ReceiverParam);

	/* Activate acquisition */
	FreqSyncAcq.StartAcquisition();
	TimeSync.StartAcquisition();
	ChannelEstimation.GetTimeSyncTrack()->StopTracking();
	ChannelEstimation.StartSaRaOffAcq();
	ChannelEstimation.GetTimeWiener()->StopTracking();

	SyncUsingPil.StartAcquisition();
	SyncUsingPil.StopTrackPil();

	/* Set flag that no signal is currently received */
	eAcquiState = AS_NO_SIGNAL;

	/* Set flag for receiver state */
	eReceiverState = RS_ACQUISITION;

	/* This flag is to ensure that some functions are called only once
	   after frequency acquisition values was done */
	bWasFreqAcqu = TRUE;

	/* Reset counters for acquisition decision, "good signal" and delayed
	   tracking mode counter */
	iAcquRestartCnt = 0;
	iAcquDetecCnt = 0;
	iGoodSignCnt = 0;
	iDelayedTrackModeCnt = NUM_FAC_DEL_TRACK_SWITCH;

	/* Reset GUI lights */
	PostWinMessage(MS_RESET_ALL);
}

void CDRMReceiver::SetInTrackingMode()
{
	/* We do this with the flag "eReceiverState" to ensure that the following
	   routines are only called once when the tracking is actually started */
	if (eReceiverState == RS_ACQUISITION)
	{
		/* In case the acquisition estimation is still in progress, stop it now
		   to avoid a false estimation which could destroy synchronization */
		TimeSync.StopRMDetAcqu();

		/* Acquisition is done, deactivate it now and start tracking */
		ChannelEstimation.GetTimeWiener()->StartTracking();

		/* Reset acquisition for frame synchronization */
		SyncUsingPil.StopAcquisition();
		SyncUsingPil.StartTrackPil();

		/* Set receiver flag to tracking */
		eReceiverState = RS_TRACKING;
	}
}

void CDRMReceiver::SetInTrackingModeDelayed()
{
	/* The timing tracking must be enabled delayed because it must wait until
	   the channel estimation has initialized its estimation */
	TimeSync.StopTimingAcqu();
	ChannelEstimation.GetTimeSyncTrack()->StartTracking();
}

void CDRMReceiver::StartParameters(CParameter& Param)
{
/*
	Reset all parameters to starting values. This is done at the startup of the
	application and also when the S/N of the received signal is too low and
	no receiption is left -> Reset all parameters
*/

	/* Define with which parameters the receiver should try to decode the
	   signal. If we are correct with our assumptions, the receiver does not
	   need to reinitialize */
	Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_1);

	/* Set initial MLC parameters */
	Param.SetInterleaverDepth(CParameter::SI_SHORT);
	Param.SetMSCCodingScheme(CParameter::CS_2_SM);

	/* Select the service we want to decode. Always zero, because we do not
	   know how many services are transmitted in the signal we want to
	   decode */
	Param.SetCurSelAudioService(0);
	Param.SetCurSelDataService(0);

	/* Set the following parameters to zero states (initial states) --------- */
	Param.ResetServicesStreams();

	/* Protection levels */
	Param.MSCPrLe.iPartB = 1;

	/* Number of audio and data services */
	Param.iNumAudioService = 0;
	Param.iNumDataService = 0;

	/* We start with FAC ID = 0 (arbitrary) */
	Param.iFrameIDReceiv = 0;

	/* Set synchronization parameters */
	Param.rResampleOffset = rInitResampleOffset; /* Initial resample offset */
	Param.rFreqOffsetAcqui = (_REAL) 0.0;
	Param.rFreqOffsetTrack = (_REAL) 0.0;
	Param.iTimingOffsTrack = 0;

	/* Initialization of the modules */
	InitsForAllModules();
}

//Added WriteData.SetInitFlag() DM						 
void CDRMReceiver::InitsForAllModules()
{
	/* Set init flags */
	ReceiveData.SetInitFlag();
	InputResample.SetInitFlag();
	FreqSyncAcq.SetInitFlag();
	TimeSync.SetInitFlag();
	OFDMDemodulation.SetInitFlag();
	SyncUsingPil.SetInitFlag();
	ChannelEstimation.SetInitFlag();
	OFDMCellDemapping.SetInitFlag();
	FACMLCDecoder.SetInitFlag();
	UtilizeFACData.SetInitFlag();
	SymbDeinterleaver.SetInitFlag();
	MSCMLCDecoder.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	AudioSourceDecoder.SetInitFlag();
	DataDecoder.SetInitFlag();
	WriteData.SetInitFlag();
}


/* -----------------------------------------------------------------------------
   Initialization routines for the modules. We have to look into the modules
   and decide on which parameters the modules depend on */
void CDRMReceiver::InitsForWaveMode()
{
	/* After a new robustness mode was detected, give the time synchronization
	   a bit more time for its job */
	iAcquDetecCnt = 0;

	/* Set init flags */
	ReceiveData.SetInitFlag();
	InputResample.SetInitFlag();
	FreqSyncAcq.SetInitFlag();
	TimeSync.SetInitFlag();
	OFDMDemodulation.SetInitFlag();
	SyncUsingPil.SetInitFlag();
	ChannelEstimation.SetInitFlag();
	OFDMCellDemapping.SetInitFlag();
	SymbDeinterleaver.SetInitFlag(); // Because of "iNumUsefMSCCellsPerFrame"
	MSCMLCDecoder.SetInitFlag(); // Because of "iNumUsefMSCCellsPerFrame"
}

void CDRMReceiver::InitsForSpectrumOccup()
{
	/* Set init flags */
	OFDMDemodulation.SetInitFlag();
	SyncUsingPil.SetInitFlag();
	ChannelEstimation.SetInitFlag();
	OFDMCellDemapping.SetInitFlag();
	SymbDeinterleaver.SetInitFlag(); // Because of "iNumUsefMSCCellsPerFrame"
	MSCMLCDecoder.SetInitFlag(); // Because of "iNumUsefMSCCellsPerFrame"
}


/* MSC ---------------------------------------------------------------------- */
void CDRMReceiver::InitsForInterlDepth()
{
	/* Can be absolutely handled seperately */
	SymbDeinterleaver.SetInitFlag();
}

void CDRMReceiver::InitsForMSCCodSche()
{
	/* Set init flags */
	ChannelEstimation.SetInitFlag(); // For decision directed channel estimation needed
	MSCMLCDecoder.SetInitFlag();
	MSCDemultiplexer.SetInitFlag(); // Not sure if really needed, look at code! TODO
}

void CDRMReceiver::InitsForMSC()
{
	/* Set init flags */
	MSCMLCDecoder.SetInitFlag();

	InitsForMSCDemux();
}

void CDRMReceiver::InitsForMSCDemux()
{
	/* Set init flags */
	MSCDemultiplexer.SetInitFlag();
	AudioSourceDecoder.SetInitFlag();
	DataDecoder.SetInitFlag();
}

void CDRMReceiver::InitsForAudParam()
{
	/* Set init flags */
	MSCDemultiplexer.SetInitFlag();
	AudioSourceDecoder.SetInitFlag();
}

void CDRMReceiver::InitsForDataParam()
{
	/* Set init flags */
	MSCDemultiplexer.SetInitFlag();
	DataDecoder.SetInitFlag();
}
