/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	
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

#include "Data.h"

/* Implementation *************************************************************/
/******************************************************************************\
* MSC data			Added back in - DM
\******************************************************************************/


#define no_dc_tap 10
//#define MIX_INPUT_CHANNELS

double rxdcsum = 0.0;
double the_rxdcsum = 0.0;
double rxdcsumbuf[no_dc_tap] = {0.0};
int rxdcsumbufpt = 0;
 
_REAL rxaverdc = 0.0;

/* Transmitter -------------------------------------------------------------- */
void CReadData::ProcessDataInternal(CParameter& TransmParam)
{
	_REAL temp;
	rxdcsum = 0.0;

	if (TransmParam.bOnlyPicture) return;

	/* Get data from sound interface */
	pSound->Read(vecsSoundBuffer);

#ifdef MIX_INPUT_CHANNELS
	/* Write data to output buffer */
	for (int i = 0; i < iOutputBlockSize; i++) //+=2
	{
		const int i2 = i * 2;
		const int iLeftChan = vecsSoundBuffer[i2];
		const int iRightChan = vecsSoundBuffer[i2 + 1];

		temp = (_REAL)((iLeftChan + iRightChan) / 2); //@
		rxdcsum += temp;
		(*pvecOutputData)[i] = temp - rxaverdc;
		//(*pvecOutputData)[i] = (*pvecOutputData)[i+1] = temp - rxaverdc;
	}
	the_rxdcsum -= rxdcsumbuf[rxdcsumbufpt];
	rxdcsumbufpt++;
	if (rxdcsumbufpt >= no_dc_tap) rxdcsumbufpt = 0;
	rxdcsumbuf[rxdcsumbufpt] = rxdcsum / (double)iOutputBlockSize;
	//rxdcsumbuf[rxdcsumbufpt] = 2.0 * rxdcsum / (double)iOutputBlockSize;  
	the_rxdcsum += rxdcsumbuf[rxdcsumbufpt];

	rxaverdc = the_rxdcsum;
#else
	/* Write data to output buffer */
	for (int i = 0; i < iOutputBlockSize; i++) //
		(*pvecOutputData)[i] = vecsSoundBuffer[i * 2]; //was [i * 2] DM
#endif

	/* Update level meter */
	SignalLevelMeter.Update((*pvecOutputData));
}

void CReadData::InitInternal(CParameter& TransmParam)
{
	/* Define block-size for output, an audio frame always corresponds to 400 ms. We use always stereo blocks */
//	iOutputBlockSize = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE * (_REAL) 0.4);/* 400 ms */ // * 2 /* stereo */); //Original code
	iOutputBlockSize = DEFiInputBlockSize/2; //<-- in Parameter.h

	/* Init sound interface and intermediate buffer */
	if (!TransmParam.bOnlyPicture)
		pSound->InitRecording((iOutputBlockSize * 2), FALSE);
	vecsSoundBuffer.Init(iOutputBlockSize * 2);
}

/* Receiver ----------------------------------------------------------------- */
void CWriteData::ProcessDataInternal(CParameter& ReceiverParam)
{
	/* Send data to sound interface if audio is not muted */
	if ((bMuteAudio == FALSE) && (!ReceiverParam.bOnlyPicture))
	{
		if (pSound->Write((*pvecInputData)) == FALSE)
			PostWinMessage(MS_IOINTERFACE, 0); /* green light */ //=============================== CHECK
		else
			PostWinMessage(MS_IOINTERFACE, 1); /* yellow light */ //=============================== CHECK
	}

	/* Write data as wave in file */
	if (bDoWriteWaveFile == TRUE)
	{
		for (int i = 0; i < iInputBlockSize; i += 2)
			WaveFileAudio.AddStereoSample((*pvecInputData)[i] /* left */, (*pvecInputData)[i + 1] /* right */);
	}
}

void CWriteData::InitInternal(CParameter& ReceiverParam)
{
	/* Define block-size for input, an audio frame always corresponds to 400 ms.
	   We use always stereo blocks */
	iInputBlockSize = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *	(_REAL) 0.4 * 2 );  //@@ /* 400 ms *//* stereo */ 

	/* Init sound interface */
	if (!ReceiverParam.bOnlyPicture)
		pSound->InitPlayback(iInputBlockSize);
}

void CWriteData::StartWriteWaveFile(const string strFileName)
{
	/* No Lock(), Unlock() needed here */
	if (bDoWriteWaveFile == FALSE)
	{
		WaveFileAudio.Open(strFileName);
		bDoWriteWaveFile = TRUE;
	}
}

void CWriteData::StopWriteWaveFile()
{
	Lock();

	WaveFileAudio.Close();
	bDoWriteWaveFile = FALSE;

	Unlock();
}


/******************************************************************************\
* FAC data																	   *
\******************************************************************************/
/* Transmitter */
void CGenerateFACData::ProcessDataInternal(CParameter& TransmParam)
{
	FACTransmit.FACParam(pvecOutputData, TransmParam);
}

void CGenerateFACData::InitInternal(CParameter& TransmParam)
{
	FACTransmit.Init(TransmParam);

	/* Define block-size for output */
	iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
}

/* Receiver */
void CUtilizeFACData::ProcessDataInternal(CParameter& ReceiverParam)
{
	/* Do not use received FAC data in case of simulation */
	{
		bCRCOk = FACReceive.FACParam(pvecInputData, ReceiverParam);

		if (bCRCOk == TRUE)
		{
			PostWinMessage(MS_FAC_CRC, 0);
		}
		else
		{
			PostWinMessage(MS_FAC_CRC, 2);
		}
	}

	if (bCRCOk == FALSE)
	{
		/* If FAC CRC check failed we should increase the frame-counter 
		   manually. If only FAC data was corrupted, the others can still
		   decode if they have the right frame number. In case of simulation
		   no FAC data is used, we have to increase the counter here */
		ReceiverParam.iFrameIDReceiv++;

		if (ReceiverParam.iFrameIDReceiv == NUM_FRAMES_IN_SUPERFRAME)
			ReceiverParam.iFrameIDReceiv = 0;
	}
}

void CUtilizeFACData::InitInternal(CParameter& ReceiverParam)
{

// This should be in FAC class in an Init() routine which has to be defined, this
// would be cleaner code! TODO
/* Init frame ID so that a "0" comes after increasing the init value once */
	ReceiverParam.iFrameIDReceiv = NUM_FRAMES_IN_SUPERFRAME - 1;

	/* Reset flag */
	bCRCOk = FALSE;

	/* Define block-size for input */
	iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;
}

