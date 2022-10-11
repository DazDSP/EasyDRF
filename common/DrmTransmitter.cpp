/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *
 * Description:
 *	DRM-transmitter
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

#include "DrmTransmitter.h"
#include "ptt.h"

/* Implementation *************************************************************/

BOOL DoNotSend = TRUE;

void CDRMTransmitter::Start()
{
	/* Set run flag */
	TransmParam.bRunThread = TRUE;

	DoNotSend = TRUE;

	/* Initialization of the modules */
	Init();

	/* Start the transmitter run routine */
	Run();
}

void CDRMTransmitter::Send()
{
	DoNotSend = FALSE;
}
void CDRMTransmitter::NotSend()
{
	DoNotSend = TRUE;
}

void CDRMTransmitter::Stop()
{
	TransmParam.bRunThread = FALSE;
}

void CDRMTransmitter::Run()
{
/*
 The hand over of data is done via an intermediate-buffer. The calling
 convention is always "input-buffer, output-buffer". Additional, the
 DRM-parameters are fed to the function
*/
	while (TransmParam.bRunThread)
	{
		if (DoNotSend)
		{
			Sleep(200);
		}
		else
		{
			/* MSC ****************************************************************/
			/* Read the source signal (Audio Input from Mike) */
			ReadData.ReadData(TransmParam, DataBuf);

			/* Audio source encoder */
			AudioSourceEncoder.ProcessData(TransmParam, DataBuf, AudSrcBuf); //Added DM - This naming convention is confusing here. DataBuf is the audio input, and AudSrcBuf is the data output!
//			AudioSourceEncoder.ProcessData(TransmParam, AudSrcBuf); //no audio input for DLL version DM

			/* MLC-encoder */
			MSCMLCEncoder.ProcessData(TransmParam, AudSrcBuf, MLCEncBuf);
	
			/* Convolutional interleaver */
			SymbInterleaver.ProcessData(TransmParam, MLCEncBuf, IntlBuf);

			/* FAC ****************************************************************/
			GenerateFACData.ReadData(TransmParam, GenFACDataBuf);
			FACMLCEncoder.ProcessData(TransmParam, GenFACDataBuf, FACMapBuf);

			/* Mapping of the MSC, FAC and pilots on the carriers ************/
			OFDMCellMapping.ProcessData(TransmParam, IntlBuf, FACMapBuf, CarMapBuf);

			/* OFDM-modulation ****************************************************/
			OFDMModulation.ProcessData(TransmParam, CarMapBuf, OFDMModBuf);
	
			/* Transmit the signal ************************************************/
			TransmitData.WriteData(TransmParam, OFDMModBuf);
		}
	}
	try{ SoundInterface.Close(); }
	catch (CGenErr) { throw; }
}

void CDRMTransmitter::Init()
{
	try
	{
		/* Defines number of cells, important! */
		OFDMCellMapping.Init(TransmParam, CarMapBuf);
		MSCMLCEncoder.Init(TransmParam, MLCEncBuf);
		SymbInterleaver.Init(TransmParam, IntlBuf);
		GenerateFACData.Init(TransmParam, GenFACDataBuf);
		FACMLCEncoder.Init(TransmParam, FACMapBuf);
		OFDMModulation.Init(TransmParam, OFDMModBuf);
		AudioSourceEncoder.Init(TransmParam, AudSrcBuf);
		ReadData.Init(TransmParam, DataBuf); //Added DM ========================									  
		TransmitData.Init(TransmParam);
	}
	catch (CGenErr) { throw; }
}

void CDRMTransmitter::StartParameters(CParameter& Param)
{
	/* Init streams */
	Param.ResetServicesStreams();

	/* Init frame ID counter (index) */
	Param.iFrameIDTransm = 0;

	/**************************************************************************/
	/* Robustness mode and spectrum occupancy. Available transmission modes:
	   RM_ROBUSTNESS_MODE_A: Gaussian channels, with minor fading,
	   RM_ROBUSTNESS_MODE_B: Time and frequency selective channels, with longer
	   delay spread,
	   Available bandwidths:
	   SO_0: 4.5 kHz, SO_1: 5 kHz, SO_2: 9 kHz, SO_3: 10 kHz, SO_4: 18 kHz,
	   SO_5: 20 kHz */
	Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_0);

	/* Protection levels for MSC. Depend on the modulation scheme. Look at
	   TableMLC.h, iCodRateCombMSC16SM, iCodRateCombMSC64SM,
	   iCodRateCombMSC64HMsym, iCodRateCombMSC64HMmix for available numbers */
	Param.MSCPrLe.iPartB = 0;

	/* Either one audio or one data service can be chosen */
	_BOOLEAN bIsAudio = FALSE; //does this need external access? - NO everything assumes data mode DM
	
	/* In the current version only one service and one stream is supported. The
	   stream IDs must be 0 in both cases */
if (bIsAudio == TRUE)
	{
		/* Audio */
		Param.iNumAudioService = 1;
		Param.iNumDataService = 0;

		Param.Service[0].eAudDataFlag = CParameter::SF_AUDIO;
		Param.Service[0].AudioParam.iStreamID = 0;

		/* Text message */
		Param.Service[0].AudioParam.bTextflag = FALSE;

		/* Programme Type code (see TableFAC.h, "strTableProgTypCod[]") */
		Param.Service[0].iServiceDescr = 15; /* 15 -> other music */
	}
	else
	{
		/* Data */
		Param.iNumAudioService = 0;
		Param.iNumDataService = 1;

		Param.Service[0].eAudDataFlag = CParameter::SF_DATA;
		Param.Service[0].DataParam.iStreamID = 0;

		/* Init SlideShow application */
		Param.Service[0].DataParam.iPacketLen = 109; /* TEST */ //(v1 was 127) DM ============
		Param.Service[0].DataParam.eDataUnitInd = CParameter::DU_DATA_UNITS;
		Param.Service[0].DataParam.eAppDomain = CParameter::AD_DAB_SPEC_APP;

		Param.Service[0].iServiceDescr = 0;
	}

	/* Init service parameters, 24 bit unsigned integer number */
	Param.Service[0].iServiceID = 73;

	/* Service label data. Up to 16 bytes defining the label using UTF-8
	   coding */
	Param.Service[0].strLabel = "undef";

	/* Language (see TableFAC.h, "strTableLanguageCode[]") */
	Param.Service[0].iLanguage = 7; 

	/* Interleaver mode of MSC service. Long interleaving (2 s): SI_LONG,
	   short interleaving (400 ms): SI_SHORT */
	Param.eSymbolInterlMode = CParameter::SI_SHORT;

	/* MSC modulation scheme. Available modes:
	   16-QAM standard mapping (SM): CS_2_SM,
	   64-QAM standard mapping (SM): CS_3_SM,
	   64-QAM symmetrical hierarchical mapping (HMsym): CS_3_HMSYM,
	   64-QAM mixture of the previous two mappings (HMmix): CS_3_HMMIX */
	Param.eMSCCodingScheme = CParameter::CS_2_SM;

	/* Set desired intermediate frequency (IF) in Hertz */
	SetCarOffset(350); /* Default */

}
