/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	MSC audio/data demultiplexer
 *
 *
 * - (6.2.3.1) Multiplex frames (DRM standard):
 * The multiplex frames are built by placing the logical frames from each
 * non-hierarchical stream together. The logical frames consist, in general, of
 * two parts each with a separate protection level. The multiplex frame is
 * constructed by taking the data from the higher protected part of the logical
 * frame from the lowest numbered stream (stream 0 when hierarchical modulation
 * is not used, or stream 1 when hierarchical modulation is used) and placing
 * it at the start of the multiplex frame. Next the data from the higher
 * protected part of the logical frame from the next lowest numbered stream is
 * appended and so on until all streams have been transferred. The data from
 * the lower protected part of the logical frame from the lowest numbered
 * stream (stream 0 when hierarchical modulation is not used, or stream 1 when
 * hierarchical modulation is used) is then appended, followed by the data from
 * the lower protected part of the logical frame from the next lowest numbered
 * stream, and so on until all streams have been transferred. The higher
 * protected part is designated part A and the lower protected part is
 * designated part B in the multiplex description. The multiplex frame will be
 * larger than or equal to the sum of the logical frames from which it is
 * formed. The remainder, if any, of the multiplex frame shall be filled with
 * 0s. These bits shall be ignored by the receiver.
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

#include "MSCMultiplexer.h"


/* Implementation *************************************************************/
void CMSCDemultiplexer::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i;

	/* Audio ---------------------------------------------------------------- */
	/* Extract audio data from input-stream */
	/* Higher protected part */
	for (i = 0; i < AudStreamPos.iLenHigh; i++)
		(*pvecOutputData)[i] = (*pvecInputData)[i + AudStreamPos.iOffsetHigh];

	/* Lower protected part */
	for (i = 0; i < AudStreamPos.iLenLow; i++)
		(*pvecOutputData)[i + AudStreamPos.iLenHigh] =
			(*pvecInputData)[i + AudStreamPos.iOffsetLow];


	/* Data ----------------------------------------------------------------- */
	/* Extract audio data from input-stream */
	/* Higher protected part */
	for (i = 0; i < DataStreamPos.iLenHigh; i++)
		(*pvecOutputData2)[i] = (*pvecInputData)[i + DataStreamPos.iOffsetHigh];

	/* Lower protected part */
	for (i = 0; i < DataStreamPos.iLenLow; i++)
		(*pvecOutputData2)[i + DataStreamPos.iLenHigh] =
			(*pvecInputData)[i + DataStreamPos.iOffsetLow];
}

void CMSCDemultiplexer::InitInternal(CParameter& ReceiverParam)
{
	int	iCurDataStreamID;

	/* Audio ---------------------------------------------------------------- */
	/* Check if current selected service is an audio service and get stream
	   position */
	if (ReceiverParam.Service[ReceiverParam.GetCurSelAudioService()].
		eAudDataFlag == CParameter::SF_AUDIO)
	{
		GetStreamPos(ReceiverParam, ReceiverParam.
			Service[ReceiverParam.GetCurSelAudioService()].AudioParam.iStreamID,
			AudStreamPos);
	}
	else
	{
		/* This is not an audio stream, zero the lengths */
		AudStreamPos.iLenHigh = 0;
		AudStreamPos.iLenLow = 0;
	}

	/* Set audio output block size */
	iOutputBlockSize = AudStreamPos.iLenHigh + AudStreamPos.iLenLow;

	/* Set number of output bits for audio decoder in global struct */
	ReceiverParam.SetNumAudioDecoderBits(iOutputBlockSize);


	/* Data ----------------------------------------------------------------- */
	/* If multimedia is not used, set stream ID to "not used" which leads to
	   an output size of "0" -> no output data generated */
	if (ReceiverParam.bUsingMultimedia)
		iCurDataStreamID = ReceiverParam.
			Service[ReceiverParam.GetCurSelDataService()].DataParam.iStreamID;
	else
		iCurDataStreamID = STREAM_ID_NOT_USED;

	/* Get stream position of current selected data service */
	GetStreamPos(ReceiverParam, iCurDataStreamID, DataStreamPos);

	/* Set data output block size */
	iOutputBlockSize2 = DataStreamPos.iLenHigh + DataStreamPos.iLenLow;

	/* Set number of output bits for data decoder in global struct */
	ReceiverParam.SetNumDataDecoderBits(iOutputBlockSize2);


	/* Set input block size */
	iInputBlockSize = ReceiverParam.iNumDecodedBitsMSC;
}

void CMSCDemultiplexer::GetStreamPos(CParameter& Param, const int iStreamID,
									 CStreamPos& StPos)
{
	int				i;
	CVector<int>	veciActStreams;

	if (iStreamID != STREAM_ID_NOT_USED)
	{
		/* Length of higher and lower protected part of audio stream (number
		   of bits) */
		StPos.iLenHigh = 0;
		StPos.iLenLow = Param.Stream[iStreamID].iLenPartB *	SIZEOF__BYTE;


		/* Byte-offset of higher and lower protected part of audio stream --- */
		/* Get active streams */
		Param.GetActiveStreams(veciActStreams);

		/* Get start offset for lower protected parts in stream. Since lower
		   protected part comes after the higher protected part, the offset
		   must be shifted initially by all higher protected part lengths
		   (iLenPartA of all streams are added) 6.2.3.1 */
		StPos.iOffsetLow = 0;

		/* Real start position of the streams */
		StPos.iOffsetHigh = 0;
		for (i = 0; i < veciActStreams.Size(); i++)
		{
			if (veciActStreams[i] < iStreamID)
			{
				StPos.iOffsetLow += Param.Stream[i].iLenPartB * SIZEOF__BYTE;
			}
		}


		/* Possibility check ------------------------------------------------ */
		/* Test, if parameters have possible values */
		if ((StPos.iOffsetHigh + StPos.iLenHigh > Param.iNumDecodedBitsMSC) ||
			(StPos.iOffsetLow + StPos.iLenLow > Param.iNumDecodedBitsMSC))
		{
			/* Something is wrong, set everything to zero */
			StPos.iOffsetLow = 0;
			StPos.iOffsetHigh = 0;
			StPos.iLenLow = 0;
			StPos.iLenHigh = 0;
		}
	}
	else
	{
		/* Error, set everything to zero */
		StPos.iOffsetLow = 0;
		StPos.iOffsetHigh = 0;
		StPos.iLenLow = 0;
		StPos.iLenHigh = 0;
	}
}
