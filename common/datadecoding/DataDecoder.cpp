/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Data module (using multimedia information carried in DRM stream)
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

#include "DataDecoder.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Encoder                                                                      *
\******************************************************************************/
void CDataEncoder::GeneratePacket(CVector<_BINARY>& vecbiPacket)
{
	int			i = 0;
	_BOOLEAN	bLastFlag = 0;

	/* Init size for whole packet, not only body */
	vecbiPacket.Init(iTotalPacketSize);
	vecbiPacket.ResetBitAccess();

	/* Calculate remaining data size to be transmitted */
	const int iRemainSize = vecbiCurDataUnit.Size() - iCurDataPointer;


	/* Header --------------------------------------------------------------- */
	/* First flag */
	if (iCurDataPointer == 0)
		vecbiPacket.Enqueue((uint32_t) 1, 1);
	else
		vecbiPacket.Enqueue((uint32_t) 0, 1);

	/* Last flag */
	if (iRemainSize > iPacketLen)
	{
		vecbiPacket.Enqueue((uint32_t) 0, 1);
		bLastFlag = FALSE;
	}
	else
	{
		vecbiPacket.Enqueue((uint32_t) 1, 1);
		bLastFlag = TRUE;
	}

	/* Packet Id */
	vecbiPacket.Enqueue((uint32_t) iPacketID, 2);

	/* Padded packet indicator (PPI) */
	if (iRemainSize < iPacketLen)
		vecbiPacket.Enqueue((uint32_t) 1, 1);
	else
		vecbiPacket.Enqueue((uint32_t) 0, 1);

	/* Continuity index (CI) */
	vecbiPacket.Enqueue((uint32_t) iContinInd, 3);

	/* Increment index modulo 8 (1 << 3) */
	iContinInd++;
	if (iContinInd == 8)
		iContinInd = 0;


	/* Body ----------------------------------------------------------------- */
	if (iRemainSize >= iPacketLen)
	{
		if (iRemainSize == iPacketLen)
		{
			/* Last packet */
			for (i = 0; i < iPacketLen; i++)
				vecbiPacket.Enqueue(vecbiCurDataUnit.Separate(1), 1);
		}
		else
		{
			for (i = 0; i < iPacketLen; i++)
			{
				vecbiPacket.Enqueue(vecbiCurDataUnit.Separate(1), 1);
				iCurDataPointer++;
			}
		}
	}
	else
	{
		/* Padded packet. If the PPI is 1 then the first byte shall indicate
		   the number of useful bytes that follow, and the data field is
		   completed with padding bytes of value 0x00 */
		vecbiPacket.Enqueue((uint32_t) (iRemainSize / SIZEOF__BYTE), SIZEOF__BYTE);

		/* Data */
		for (i = 0; i < iRemainSize; i++)
			vecbiPacket.Enqueue(vecbiCurDataUnit.Separate(1), 1);

		/* Padding */
		for (i = 0; i < iPacketLen - iRemainSize; i++)
			vecbiPacket.Enqueue(vecbiCurDataUnit.Separate(1), 1);
	}

	/* If this was the last packet, get data for next data unit */
	if (bLastFlag == TRUE)
	{
		/* Generate new data unit */
		MOTSlideShowEncoder.GetDataUnit(vecbiCurDataUnit);
		vecbiCurDataUnit.ResetBitAccess();

		/* Reset data pointer and continuity index */
		iCurDataPointer = 0;
	}


	/* CRC ------------------------------------------------------------------ */
	CCRC CRCObject;

	/* Reset bit access */
	vecbiPacket.ResetBitAccess();

	/* Calculate the CRC and put it at the end of the segment */
	CRCObject.Reset(16);

	/* "byLengthBody" was defined in the header */
	for (i = 0; i < (iTotalPacketSize / SIZEOF__BYTE - 2); i++)
		CRCObject.AddByte(vecbiPacket.Separate(SIZEOF__BYTE));

	/* Now, pointer in "enqueue"-function is back at the same place, add CRC */
	vecbiPacket.Enqueue(CRCObject.GetCRC(), 16);
}

int CDataEncoder::Init(CParameter& Param)
{
	/* Init packet length and total packet size (the total packet length is
	   three bytes longer as it includes the header and CRC fields) */

// TODO we only use always the first service right now
const int iCurSelDataServ = 0;

	const int iSegLen = Param.Service[iCurSelDataServ].DataParam.iPacketLen - 11;
	iPacketLen = Param.Service[iCurSelDataServ].DataParam.iPacketLen * SIZEOF__BYTE;
	iTotalPacketSize = iPacketLen + 24 /* CRC + header = 24 bits */;

	iPacketID = Param.Service[iCurSelDataServ].DataParam.iPacketID;

	/* Init DAB MOT encoder object */
	MOTSlideShowEncoder.Init(iSegLen);

	/* Generate first data unit */
	MOTSlideShowEncoder.GetDataUnit(vecbiCurDataUnit);
	vecbiCurDataUnit.ResetBitAccess();

	/* Reset pointer to current position in data unit and continuity index */
	iCurDataPointer = 0;
	iContinInd = 0;

	/* Return total packet size */
	return iTotalPacketSize;
}


/******************************************************************************\
* Decoder                                                                      *
\******************************************************************************/
void CDataDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i = 0, j = 0; //init DM
	int			iPacketID = 0; //init DM;
	int			iNewContInd = 0; //init DM
	int			iNewPacketDataSize = 0; //init DM
	int			iOldPacketDataSize = 0; //init DM
	int			iNumSkipBytes = 0; //init DM
	_BINARY		biFirstFlag = 0; //init DM
	_BINARY		biLastFlag = 0; //init DM
	_BINARY		biPadPackInd = 0; //init DM
	CCRC		CRCObject;

	/* Check if something went wrong in the initialization routine */
	if (DoNotProcessData == TRUE)
		return;


	/* CRC check for all packets -------------------------------------------- */
	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();

	for (j = 0; j < iNumDataPackets; j++)
	{
		/* Check the CRC of this packet */
		CRCObject.Reset(16);

		/* "- 2": 16 bits for CRC at the end */
		for (i = 0; i < iTotalPacketSize - 2; i++)
			CRCObject.AddByte((_BYTE) (*pvecInputData).Separate(SIZEOF__BYTE));

		/* Store result in vector and show CRC in multimedia window */
		if (CRCObject.CheckCRC((*pvecInputData).Separate(16)) == TRUE)
		{
			veciCRCOk[j] = 1; /* CRC ok */
			PostWinMessage(MS_MSC_CRC, 0); /* Green light */
		}
		else
		{
			veciCRCOk[j] = 0; /* CRC wrong */
			PostWinMessage(MS_MSC_CRC, 2); /* Red light */
		}
	}


	/* Extract packet data -------------------------------------------------- */
	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();

	for (j = 0; j < iNumDataPackets; j++)
	{
		/* Check if CRC was ok */
		if (veciCRCOk[j] == 1)
		{
			/* Read header data --------------------------------------------- */
			/* First flag */
			biFirstFlag = (_BINARY) (*pvecInputData).Separate(1);

			/* Last flag */
			biLastFlag = (_BINARY) (*pvecInputData).Separate(1);

			/* Packet ID */
			iPacketID = (int) (*pvecInputData).Separate(2);

			/* Padded packet indicator (PPI) */
			biPadPackInd = (_BINARY) (*pvecInputData).Separate(1);

			/* Continuity index (CI) */
			iNewContInd = (int) (*pvecInputData).Separate(3);


			/* Act on parameters given in header */
			/* Continuity index: this 3-bit field shall increment by one
			   modulo-8 for each packet with this packet Id */
			if ((iContInd[iPacketID] + 1) % 8 != iNewContInd) //this detects if there are missing packets? DM
				DataUnit[iPacketID].bOK = FALSE;

			/* Store continuity index */
			iContInd[iPacketID] = iNewContInd;

			/* Reset flag for data unit ok when receiving the first packet of
			   a new data unit */
			if (biFirstFlag == TRUE)
			{
				DataUnit[iPacketID].Reset(); //clears and resets vecbiData DM

				DataUnit[iPacketID].bOK = TRUE;
			}


			/* If all packets are received correctely, data unit is ready */
			if (biLastFlag == TRUE)
				if (DataUnit[iPacketID].bOK == TRUE)
					DataUnit[iPacketID].bReady = TRUE;


			/* Data field --------------------------------------------------- */
			/* Get size of new data block */
			if (biPadPackInd == TRUE)
			{
				/* Padding is present: the first byte gives the number of
				   useful data bytes in the data field. */
				iNewPacketDataSize = (int) (*pvecInputData).Separate(SIZEOF__BYTE) * SIZEOF__BYTE;

				if (iNewPacketDataSize > iMaxPacketDataSize)
				{
					/* Error, reset flags */
					DataUnit[iPacketID].bOK = FALSE;
					DataUnit[iPacketID].bReady = FALSE;

					/* Set values to read complete packet size */
					iNewPacketDataSize = iNewPacketDataSize;
					iNumSkipBytes = 2; /* Only CRC has to be skipped */
				}
				else
				{
					/* Number of unused bytes ("- 2" because we also have the
					   one byte which stored the size, the other byte is the
					   header) */
					iNumSkipBytes = iTotalPacketSize - 2 - iNewPacketDataSize / SIZEOF__BYTE;
				}

				/* Packets with no useful data are permitted if no packet
				   data is available to fill the logical frame. The PPI
				   shall be set to 1 and the first byte of the data field
				   shall be set to 0 to indicate no useful data. The first
				   and last flags shall be set to 1. The continuity index
				   shall be incremented for these empty packets */
				if ((biFirstFlag == TRUE) && (biLastFlag == TRUE) && (iNewPacketDataSize == 0))
				{
					/* Packet with no useful data, reset flag */
					DataUnit[iPacketID].bReady = FALSE;
				}
			}
			else
			{
				iNewPacketDataSize = iMaxPacketDataSize;

				/* All bytes are useful bytes, only CRC has to be skipped */
				iNumSkipBytes = 2;
			}

			/* Add new data to data unit vector (bit-wise copying) */
			iOldPacketDataSize = DataUnit[iPacketID].vecbiData.Size();

			DataUnit[iPacketID].vecbiData.Enlarge(iNewPacketDataSize);

			/* Read useful bits */
			for (i = 0; i < iNewPacketDataSize; i++)
				DataUnit[iPacketID].vecbiData[iOldPacketDataSize + i] = (_BINARY) (*pvecInputData).Separate(1);

			/* Read bytes which are not used */
			for (i = 0; i < iNumSkipBytes; i++)
				(*pvecInputData).Separate(SIZEOF__BYTE);


			/* Use data unit ------------------------------------------------ */
			if (DataUnit[iPacketID].bReady == TRUE)
			{
				/* Decode all IDs regardless whether activated or not
				   (iPacketID == or != iServPacketID) */
				/* Only DAB multimedia is supported */
				switch (eAppType)
				{
				case AT_MOTSLISHOW: /* MOTSlideshow */
					/* Packet unit decoding */
					MOTSlideShow[iPacketID].AddDataUnit(DataUnit[iPacketID].vecbiData);
					break;

				case AT_JOURNALINE:
					break;
				}

				/* Packet was used, reset it now for new filling with new data
				   (this will also reset the flag
				   "DataUnit[iPacketID].bReady") */
				DataUnit[iPacketID].Reset();
			}
		}
		else
		{
			/* Skip incorrect packet */
			for (i = 0; i < iTotalPacketSize; i++)
				(*pvecInputData).Separate(SIZEOF__BYTE);
		}
	}
}

void CDataDecoder::InitInternal(CParameter& ReceiverParam)
{
	int iTotalNumInputBits = 0; //init DM
	int iTotalNumInputBytes = 0; //init DM
	int	iCurDataStreamID = 0; //init DM
	int iCurSelDataServ = 0; //init DM

	/* Init error flag */
	DoNotProcessData = FALSE;

	/* Get current selected data service */
	iCurSelDataServ = ReceiverParam.GetCurSelDataService();

	/* Get current data stream ID */
	iCurDataStreamID = ReceiverParam.Service[iCurSelDataServ].DataParam.iStreamID;

	/* Get number of total input bits (and bytes) for this module */
	iTotalNumInputBits = ReceiverParam.iNumDataDecoderBits;
	iTotalNumInputBytes = iTotalNumInputBits / SIZEOF__BYTE;

	/* Init application type (will be overwritten by correct type later */
	eAppType = AT_NOT_SUP;

	/* Check, if service is activated. Also, only packet services can be decoded */
	if ((iCurDataStreamID != STREAM_ID_NOT_USED) && (ReceiverParam.Service[iCurSelDataServ].DataParam.ePacketModInd == CParameter::PM_PACKET_MODE))
	{
		/* Calculate total packet size. DRM standard: packet length: this
		   field indicates the length in bytes of the data field of each
		   packet specified as an unsigned binary number (the total packet
		   length is three bytes longer as it includes the header and CRC
		   fields) */
		iTotalPacketSize = ReceiverParam.Service[iCurSelDataServ].DataParam.iPacketLen + 3;

		/* Check total packet size, could be wrong due to wrong SDC */
		if ((iTotalPacketSize <= 0) || (iTotalPacketSize > iTotalNumInputBytes))
		{
			/* Set error flag */
			DoNotProcessData = TRUE;
		}
		else
		{
			/* Maximum number of bits for the data part in a packet
			   ("- 3" because two bits for CRC and one for the header) */
 			iMaxPacketDataSize = (iTotalPacketSize - 3) * SIZEOF__BYTE;
	
			/* Number of data packets in one data block */
			iNumDataPackets = iTotalNumInputBytes / iTotalPacketSize;

			/* Get the packet ID of the selected service */
			iServPacketID = ReceiverParam.Service[iCurSelDataServ].DataParam.iPacketID;

			/* Get application domain of selected service */
			CParameter::EApplDomain eServAppDomain = ReceiverParam.Service[iCurSelDataServ].DataParam.eAppDomain;

			/* Get application identifier of current selected service, only used with DAB */
			int iDABUserAppIdent = ReceiverParam.Service[iCurSelDataServ].DataParam.iUserAppIdent;

			/* Set application type of the data service */
			if (eServAppDomain == CParameter::AD_DAB_SPEC_APP)
			{
				switch (iDABUserAppIdent)
				{
				case 2: /* MOTSlideshow */
					eAppType = AT_MOTSLISHOW;
					break;

				/* The preliminary 11-bit user Application Type ID for the
				   NewsService Journaline shall be 0x44A */
				case 0x44A: /* Journaline */
					eAppType = AT_JOURNALINE;
					break;
				}
			}

			/* Init vector for storing the CRC results for each packet */
			veciCRCOk.Init(iNumDataPackets);

			/* Reset data units for all possible data IDs */
			for (int i = 0; i < MAX_NUM_PACK_PER_STREAM; i++)
			{
				DataUnit[i].Reset();

				/* Reset continuity index (CI) */
				iContInd[i] = 0;
			}
		}
	}
	else
		DoNotProcessData = TRUE;

	/* Set input block size */
	iInputBlockSize = iTotalNumInputBits;
}

_BOOLEAN CDataDecoder::GetSlideShowPicture(CMOTObject& NewPic)
{
	_BOOLEAN bReturn = FALSE;

	/* Lock resources */
	Lock();

	/* Check if data service is SlideShow application */
	if ((DoNotProcessData == FALSE) && (eAppType == AT_MOTSLISHOW))
		bReturn = MOTSlideShow[iServPacketID].GetPicture(NewPic);

	/* Release resources */
	Unlock();

	return bReturn;
}

_BOOLEAN CDataDecoder::GetSlideShowPartActSegs(CVector<_BINARY>& vbSegs)
{
	return MOTSlideShow[0].GetActSegments(vbSegs);
}

_BOOLEAN CDataDecoder::GetSlideShowPartPicture(CMOTObject& NewPic)
{
	_BOOLEAN bReturn = FALSE;

	/* Lock resources */
	Lock();

	/* Check if data service is SlideShow application */
	bReturn = MOTSlideShow[0].GetPartPicture(NewPic);

	/* Release resources */
	Unlock();

	return bReturn;
}

_BOOLEAN CDataDecoder::GetSlideShowBSR(int * iNumSeg, string * bsrname, char * path)
{	
	return MOTSlideShow[0].GetPartBSR(iNumSeg,bsrname,path);
}

int CDataDecoder::GetTotSize(void) 
{ 
	return MOTSlideShow[iServPacketID].GetTotSize(); 
}
int CDataDecoder::GetActSize(void) 
{ 
	return MOTSlideShow[iServPacketID].GetActSize(); 
}
int CDataDecoder::GetActPos(void) 
{ 
	return MOTSlideShow[iServPacketID].GetActPos(); 
}
