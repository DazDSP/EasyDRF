/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Doyle Richard
 *  Francesco Lanza
 *	Daz Man
 * 
 * Description:
 *	DAB MOT interface implementation
 *
 * 12/22/2003 Doyle Richard
 *  - Header extension decoding
 *
 * 10 September 2021 Daz Man
 *  - Protocol changes for better robustness
 *  - Additional buffer to access data if header fails
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
#define WIN32_LEAN_AND_MEAN        
#include <windows.h>
#include "DABMOT.h"
#include "picpool.h"
#include "../bsr.h"
#include "../../RS-defs.h"

/* Implementation *************************************************************/
/******************************************************************************\
* Encoder                                                                      *
\******************************************************************************/

int bsr_transID = 0;

void CMOTDABEnc::SetMOTObject(CMOTObject& NewMOTObject, CVector<short> vecsDataIn)
{
	int				i;
	unsigned char	xorfnam,addfnam;
	//unsigned char flen;
	CMOTObjectRaw	MOTObjectRaw;

	/* Get some necessary parameters of object */
	const int iPicSizeBits = NewMOTObject.vecbRawData.Size();
	const int iPicSizeBytes = iPicSizeBits / SIZEOF__BYTE;
	const string strFileName = NewMOTObject.strName;

	/* File name size is restricted (in this implementation) to 128 (2^7) bytes.
	   If file name is longer, cut it. TODO: better solution: set Ext flag in
	   "ContentName" header extension to allow larger file names */

	//Daz Man: Filenames of any length (plus any other required data) can be sent with the file by appending a header to it.
	//This is now implemented in the RS modes to guarantee filename and size delivery.

	int iFileNameSize = strFileName.size();
	if (iFileNameSize > 80)
		iFileNameSize = 80;  // max for unpartitionned header

	if (strcmp(strFileName.c_str(),"bsr.bin") == 0)
	{
		if (NewMOTObject.bIsLeader) 
		{
			bsr_transID++;
			if (bsr_transID >= 3) bsr_transID = 0;
		}
		iTransportID = bsr_transID;
	}
	else
	{
		xorfnam = 0;
		addfnam = 0;
		//flen = iPicSizeBytes & 0x0ff;
		for (i=0;i<iFileNameSize;i++)
		{
			xorfnam ^= strFileName[i];
			addfnam += strFileName[i];
			addfnam ^= (unsigned char)i;
		}
		//xorfnam ^= flen;
		/* Init Transport ID */
		iTransportID = 256*(int)addfnam + (int)xorfnam;
		if (iTransportID <= 2) iTransportID += iFileNameSize;
		storesentfilename(NewMOTObject.strNameandDir,NewMOTObject.strName,iTransportID);
	}

#undef storeid
#ifdef storeid
	FILE * logfile = fopen("transid_tx.txt","a+t");
	if (logfile != NULL)
	{
		fprintf(logfile,"\ntid: %d   fnam: %s",iTransportID,strFileName.c_str());
		fclose(logfile);
	}
#endif

	/* Copy actual raw data of object */
	MOTObjectRaw.Body.vecbiData.Init(iPicSizeBits);
	MOTObjectRaw.Body.vecbiData = NewMOTObject.vecbRawData;

	/* Get content type and content sub type of object. We use the format string
	   to get these informations about the object */
	int iContentType = 0; /* Set default value (general data) */
	int iContentSubType = 0; /* Set default value (gif) */

	/* Get ending string which declares the type of the file. Make lowercase */


	iContentType = 2;
	iContentSubType = 1;


	/* Header --------------------------------------------------------------- */ //This is the file header that contains the filename and file size DM
	/* Header size (including header extension) */
	const int iHeaderSize = 7 /* Header core  */ +
		5 /* TriggerTime */ +
		3 + iFileNameSize /* ContentName (header + actual name) */ +
		2 /* VersionNumber */;

	/* Allocate memory and reset bit access */
	MOTObjectRaw.Header.vecbiData.Init(iHeaderSize * SIZEOF__BYTE);
	MOTObjectRaw.Header.vecbiData.ResetBitAccess();

	/* BodySize: This 28-bit field, coded as an unsigned binary number,
	   indicates the total size of the body in bytes */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t)iPicSizeBytes, 28); //The file size is sent in the file header here DM
	EncFileSize = iPicSizeBytes; //Also set it here to send in the segment header DM (try to do this through the classes for neatness TODO DM)

	/* HeaderSize: This 13-bit field, coded as an unsigned binary number,
	   indicates the total size of the header in bytes */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) iHeaderSize, 13);

	/* ContentType: This 6-bit field indicates the main category of the body's
	   content */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) iContentType, 6);	

	/* ContentSubType: This 9-bit field indicates the exact type of the body's
	   content depending on the value of the field ContentType */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) iContentSubType, 9);


	/* Header extension ----------------------------------------------------- */
	/* MOT Slideshow application: Only the MOT parameter ContentName is
	   mandatory and must be used for each slide object that will be handled by
	   the MOT decoder and the memory management of the Slide Show terminal */

	/* TriggerTime: This parameter specifies the time for when the presentation
	   takes place. The TriggerTime activates the object according to its
	   ContentType. The value of the parameter field is coded in the UTC
	   format */

	/* PLI (Parameter Length Indicator): This 2-bit field describes the total
	   length of the associated parameter. In this case:
	   1 0 total parameter length = 5 bytes; length of DataField is 4 bytes */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 2, 2);

	/* ParamId (Parameter Identifier): This 6-bit field identifies the
	   parameter. 1 0 1 (dec: 5) -> TriggerTime */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 5, 6);

	/* Validity flag = 0: "Now", MJD and UTC shall be ignored and be set to 0.
	   Set MJD and UTC to zero. UTC flag is also zero -> short form */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 32);



	/* VersionNumber: If several versions of an object are transferred, this
	   parameter indicates its VersionNumber. The parameter value is coded as an
	   unsigned binary number, starting at 0 and being incremented by 1 modulo
	   256 each time the version changes. If the VersionNumber differs, the
	   content of the body was modified */
	/* PLI
	   0 1 total parameter length = 2 bytes, length of DataField is 1 byte */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 1, 2);

	/* ParamId (Parameter Identifier): This 6-bit field identifies the
	   parameter. 1 1 0 (dec: 6) -> VersionNumber */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 6, 6);

	/* Version number data field */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 8);



	/* ContentName: The DataField of this parameter starts with a one byte
	   field, comprising a 4-bit character set indicator (see table 3) and a
	   4-bit Rfa field. The following character field contains a unique name or
	   identifier for the object. The total number of characters is determined
	   by the DataFieldLength indicator minus one byte */

	/* PLI
	   1 1 total parameter length depends on the DataFieldLength indicator */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 3, 2);

	/* ParamId (Parameter Identifier): This 6-bit field identifies the
	   parameter. 1 1 0 0 (dec: 12) -> ContentName */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 12, 6);

	/* Ext (ExtensionFlag): This 1-bit field specifies the length of the
	   DataFieldLength Indicator.
	   0: the total parameter length is derived from the next 7 bits */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 1);

	/* DataFieldLength Indicator: This field specifies as an unsigned binary
	   number the length of the parameter's DataField in bytes. The length of
	   this field is either 7 or 15 bits, depending on the setting of the
	   ExtensionFlag */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) (1 /* header */ + iFileNameSize /* actual data */), 7);

	/* Character set indicator (0 0 0 0 complete EBU Latin based repertoire) */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 4);

	/* Rfa 4 bits */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 4);

	/* Character field */
	for (i = 0; i < iFileNameSize; i++)
		MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) strFileName[i], 8); //This is where the filename is sent in the file header DM


	/* Generate segments ---------------------------------------------------- */ //This is the segment header, sent with each file segment DM
	/* Header (header should not be partitioned! TODO) */
	const int iPartiSizeHeader = 98; /* Bytes */ // mode B, 2.3 khz packlen - 11

	PartitionUnits(MOTObjectRaw.Header.vecbiData, MOTObjSegments.vvbiHeader, iPartiSizeHeader, 1);

	/* Body */
	const int iPartiSizeBody = iSegmentSize; /* Bytes */ // TEST 116
	const int noofseg =	(int) ceil((_REAL) MOTObjectRaw.Body.vecbiData.Size() / (SIZEOF__BYTE * iPartiSizeBody)); 
	const int iNumSegments = vecsDataIn.Size();

	MOTObjSegments.vecbiToSend.Init(noofseg);
	if (iNumSegments == 0)
	{
		for (i=0;i<noofseg;i++)
			MOTObjSegments.vecbiToSend[i] = 1;
	}
	else
	{
		int segct = 0;
		for (i=0;i<noofseg;i++)
		{
			if (vecsDataIn[segct] == i)
			{
				MOTObjSegments.vecbiToSend[i] = 1;
				segct++;
				if (segct == iNumSegments) i = noofseg;
			}
			else
				MOTObjSegments.vecbiToSend[i] = 0;
		}
	}

	PartitionUnits(MOTObjectRaw.Body.vecbiData, MOTObjSegments.vvbiBody, iPartiSizeBody, 0);
}

void CMOTDABEnc::PartitionUnits(CVector<_BINARY>& vecbiSource,
								CVector<CVector<_BINARY> >& vecbiDest,
								const int iPartiSize,
								int ishead)
{
	int	i, j;
	int	iActSegSize;
	_BOOLEAN skipseg;
	_BOOLEAN lastseg;

	/* Divide the generated units in partitions */
	const int iSourceSize = vecbiSource.Size() / SIZEOF__BYTE;
	const int iNumSeg =	(int) ceil((_REAL) iSourceSize / iPartiSize); /* Bytes */
	int iSizeLastSeg = iSourceSize - (int) floor((_REAL) iSourceSize / iPartiSize) * iPartiSize;

	if (iSizeLastSeg == 0) iSizeLastSeg = iPartiSize;

	//EncSegSize = iPartiSize; //Save this globally DM ============================================================================================

	iTotSegm = iNumSeg; // for percent display

	/* Init memory for destination vector, reset bit access of source */
	vecbiDest.Init(iNumSeg);
	vecbiSource.ResetBitAccess();

	for (i = 0; i < iNumSeg; i++)
	{
		/* All segments except the last one must have the size
		   "iPartSizeHeader" */
		if (i < iNumSeg - 1)
			iActSegSize = iPartiSize;
		else
			iActSegSize = iSizeLastSeg;


		if (ishead == 0)
		{
			skipseg = (MOTObjSegments.vecbiToSend[i] == 0);
			lastseg = (i == iNumSeg - 1);
			if (lastseg) skipseg = FALSE;
		}
		else skipseg = FALSE;

		if (! skipseg)
		{

			/* Add segment data ------------------------------------------------- */
			/* Header */

			/* Allocate memory for body data and segment header bits (16) */
			vecbiDest[i].Init(iActSegSize * SIZEOF__BYTE + 16);
			vecbiDest[i].ResetBitAccess();
	
			/* Segment header */
			/* RepetitionCount: This 3-bit field indicates, as an unsigned
			   binary number, the remaining transmission repetitions for the
			   current object.
			   In our current implementation, no repetitions used. TODO */
			vecbiDest[i].Enqueue((uint32_t) 0, 3);
	
			/* SegmentSize: This 13-bit field, coded as an unsigned binary
			   number, indicates the size of the segment data field in bytes */
			vecbiDest[i].Enqueue((uint32_t) iActSegSize, 13);

			/* Body */	
			for (j = 0; j < iActSegSize * SIZEOF__BYTE; j++)
				vecbiDest[i].Enqueue(vecbiSource.Separate(1), 1);
		}
		else
		{
			unsigned char dummychar;
			vecbiDest[i].Init(0);
			vecbiDest[i].ResetBitAccess();
			for (j = 0; j < iActSegSize * SIZEOF__BYTE; j++)
				dummychar = vecbiSource.Separate(1);
		}
	}
}

void CMOTDABEnc::GenMOTObj(CVector<_BINARY>& vecbiData, CVector<_BINARY>& vecbiSeg, const _BOOLEAN bHeader, const int iSegNum, const int iTranspID, const _BOOLEAN bLastSeg)
{
	int		i;
	CCRC	CRCObject;
	
	/* Standard settings for this implementation */
	const _BOOLEAN bCRCUsed = TRUE; /* CRC */
	const _BOOLEAN bSegFieldUsed = TRUE; /* segment field */
	const _BOOLEAN bUsAccFieldUsed = TRUE; /* user access field */
	const _BOOLEAN bTransIDFieldUsed = TRUE; /* transport ID field */

// TODO: Better solution!
/* Total length of object in bits */
int iTotLenMOTObj = 16 /* group header */;
if (bSegFieldUsed == TRUE)
	iTotLenMOTObj += 16;
if (bUsAccFieldUsed == TRUE)
{
	iTotLenMOTObj += 8;
	if (bTransIDFieldUsed == TRUE)
		iTotLenMOTObj += 16;
}

iTotLenMOTObj += vecbiSeg.Size();
if (bCRCUsed == TRUE)
iTotLenMOTObj += 16;

	/* Init data vector */
	vecbiData.Init(iTotLenMOTObj);
	vecbiData.ResetBitAccess();


	/* MSC data group header ------------------------------------------------ */
	/* Extension flag: this 1-bit flag shall indicate whether the extension
	   field is present, or not. Not used right now -> 0 */
	vecbiData.Enqueue((uint32_t) 0, 1);

	/* CRC flag: this 1-bit flag shall indicate whether there is a CRC at the
	   end of the MSC data group */
	if (bCRCUsed == TRUE)
		vecbiData.Enqueue((uint32_t) 1, 1);
	else
		vecbiData.Enqueue((uint32_t) 0, 1);

	/* Segment flag: this 1-bit flag shall indicate whether the segment field is
	   present, or not */
	if (bSegFieldUsed == TRUE)
		vecbiData.Enqueue((uint32_t) 1, 1);
	else
		vecbiData.Enqueue((uint32_t) 0, 1);

	/* User access flag: this 1-bit flag shall indicate whether the user access
	   field is present, or not. We always use this field -> 1 */
	if (bUsAccFieldUsed == TRUE)
		vecbiData.Enqueue((uint32_t) 1, 1);
	else
		vecbiData.Enqueue((uint32_t) 0, 1);

	/* Data group type: this 4-bit field shall define the type of data carried
	   in the data group data field. Data group types:
	   3: MOT header information
	   4: MOT data */
	if (bHeader == TRUE)
		vecbiData.Enqueue((uint32_t) 3, 4);
	else
		vecbiData.Enqueue((uint32_t) 4, 4);

	/* Continuity index: the binary value of this 4-bit field shall be
	   incremented each time a MSC data group of a particular type, with a
	   content different from that of the immediately preceding data group of
	   the same type, is transmitted */
	if (bHeader == TRUE)
	{
		vecbiData.Enqueue((uint32_t) iContIndexHeader, 4);

		/* Increment modulo 16 */
		iContIndexHeader++;
		if (iContIndexHeader == 16)
			iContIndexHeader = 0;
	}
	else
	{
		vecbiData.Enqueue((uint32_t) iContIndexBody, 4);

		/* Increment modulo 16 */
		iContIndexBody++;
		if (iContIndexBody == 16)
			iContIndexBody = 0;
	}

	/* Repetition index: the binary value of this 4-bit field shall signal the
	   remaining number of repetitions of a MSC data group with the same data
	   content, occurring in successive MSC data groups of the same type.
	   No repetition used in this implementation right now -> 0, TODO */

	   //Modified to send Total Segment Count in serial data form in RS modes DM =============================================================================================
	if (LeadIn > 3) {
	//Daz Man: Using repetition index bit 0 to send total segment count in serial form over 16 segments
	//Bit to be sent corresponds to the segment number AND 15 (Bit 0 is sent on segment 0, through bit 15 on segment 15, then it starts over)
	//A decoder bit check register has the corresponding bit cleared each time it arrives. This checks that all bits have been received before the value is used.
	//	vecbiData.Enqueue((uint32_t)0, 3); //one less bit... DM
	//	unsigned int b = (iTotSegm >> (iSegNum & 0x0F)) & 1; //Shift iTotSegm left by lower 4 bits of iSegNum, and mask to LSB
	//	vecbiData.Enqueue((uint32_t)b, 1); //send the bit

		//send 4 bits at a time:
		//unsigned int a = vecbiSource.Size() / SIZEOF__BYTE; //to send data size in bytes instead
		//unsigned int b = (EncFileSize >> ((iSegNum & 0x07) << 2)) & 0x0F; //Shift EncFileSize left by lower 4 bits of iSegNum, and mask to LSB << sends file size
	    unsigned int b = (EncFileSize >> ((iSegNum % 7) << 2)) & 0x0F; //Shift EncFileSize left by lower 4 bits of iSegNum, and mask to LSB << sends file size
		//unsigned int b = (iTotSegm >> ((iSegNum & 0x03) << 2)) & 0x0F; //Shift iTotSegm left by lower 4 bits of iSegNum, and mask to LSB << sends total segments
		vecbiData.Enqueue((uint32_t)b, 4); //send the bits

	}
	else vecbiData.Enqueue((uint32_t) 0, 4); //Original code sent 4 bits DM


	/* Extension field: this 16-bit field shall be used to carry the Data Group
	  Conditional Access (DGCA) when general data or MOT data uses conditional
	  access (Data group types 0010 and 0101, respectively). The DGCA contains
	  the Initialization Modifier (IM) and additional Conditional Access (CA)
	  information. For other Data group types, the Extension field is reserved
	  for future additions to the Data group header.
	  Extension field is not used in this implementation! */

	/* Session header ------------------------------------------------------- */
	/* Segment field */
	if (bSegFieldUsed == TRUE)
	{
		/* Last: this 1-bit flag shall indicate whether the segment number field
		   is the last or whether there are more to be transmitted */
		if (bLastSeg == TRUE)
			vecbiData.Enqueue((uint32_t) 1, 1);
		else
			vecbiData.Enqueue((uint32_t) 0, 1);

		/* Segment number: this 15-bit field, coded as an unsigned binary number
		   (in the range 0 to 32767), shall indicate the segment number.
		   NOTE: The first segment is numbered 0 and the segment number is
		   incremented by one at each new segment */
		vecbiData.Enqueue((uint32_t) iSegNum, 15);
	}

	/* User access field */
	if (bUsAccFieldUsed == TRUE)
	{
		/* Rfa (Reserved for future addition): this 3-bit field shall be
		   reserved for future additions */ //This is now used for sending the RS encoding level being used on the current file, in case the original file header is lost DM
//		vecbiData.Enqueue((uint32_t) 0, 3); //Original code was just set to zero DM
		i = LeadIn -3;			//calculate RS level used DM - RS Range is 1-5, allowing for extra levels or new types of error correction in the future DM
		if (i < 0) { i = 0; }	//limit to 0 DM
		vecbiData.Enqueue((uint32_t) i, 3); //modified to send RS level with each segment DM

		/* Transport Id flag: this 1-bit flag shall indicate whether the
		   Transport Id field is present, or not */
		if (bTransIDFieldUsed == TRUE)
			vecbiData.Enqueue((uint32_t) 1, 1);
		else
			vecbiData.Enqueue((uint32_t) 0, 1);

		/* Length indicator: this 4-bit field, coded as an unsigned binary
		   number (in the range 0 to 15), shall indicate the length n in bytes
		   of the Transport Id and End user address fields.
		   We do not use end user address field, only transport ID -> 2 */
		if (bTransIDFieldUsed == TRUE)
			vecbiData.Enqueue((uint32_t) 2, 4);
		else                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
			vecbiData.Enqueue((uint32_t) 0, 4);

		/* Transport Id (Identifier): this 16-bit field shall uniquely identify
		   one data object (file and header information) from a stream of such
		   objects, It may be used to indicate the object to which the
		   information carried in the data group belongs or relates */
		if (bTransIDFieldUsed == TRUE)
			vecbiData.Enqueue((uint32_t) iTranspID, 16);
	
	}

	/* MSC data group data field -------------------------------------------- */
	vecbiSeg.ResetBitAccess();

	for (i = 0; i < vecbiSeg.Size(); i++)
		vecbiData.Enqueue(vecbiSeg.Separate(1), 1);


	/* MSC data group CRC --------------------------------------------------- */
	/* The data group CRC shall be a 16-bit CRC word calculated on the data
	   group header, the session header and the data group data field. The
	   generation shall be based on the ITU-T Recommendation X.25.
	   At the beginning of each CRC word calculation, all shift register stage
	   contents shall be initialized to "1". The CRC word shall be complemented
	   (1's complement) prior to transmission */
	if (bCRCUsed == TRUE)
	{
		/* Reset bit access */
		vecbiData.ResetBitAccess();

		/* Calculate the CRC and put it at the end of the segment */
		CRCObject.Reset(16);

		/* "byLengthBody" was defined in the header */
		for (i = 0; i < iTotLenMOTObj / SIZEOF__BYTE - 2 /* CRC */; i++)
			CRCObject.AddByte((_BYTE) vecbiData.Separate(SIZEOF__BYTE));

		/* Now, pointer in "enqueue"-function is back at the same place, 
		   add CRC */
		vecbiData.Enqueue(CRCObject.GetCRC(), 16);
	}
}

_BOOLEAN CMOTDABEnc::GetDataGroup(CVector<_BINARY>& vecbiNewData)
{
	_BOOLEAN bLastSegment;

	/* Init return value. Is overwritten in case the object is ready */
	_BOOLEAN bObjectDone = FALSE;

	if (bCurSegHeader == TRUE)
	{
		/* Check if this is last segment */
		if (iSegmCnt == MOTObjSegments.vvbiHeader.Size() - 1)
			bLastSegment = TRUE;
		else
			bLastSegment = FALSE;

		/* Generate MOT object for header */
		GenMOTObj(vecbiNewData, MOTObjSegments.vvbiHeader[iSegmCnt], TRUE,
			iSegmCnt, iTransportID, bLastSegment);

		iSegmCnt++;
		if (iSegmCnt == MOTObjSegments.vvbiHeader.Size())
		{
			/* Reset counter */
			iSegmCnt = 0;

			/* Header is ready, transmit body now */
			bCurSegHeader = FALSE;
		}
	}
	else
	{
		/* Check that body size is not zero */
		if (iSegmCnt < MOTObjSegments.vvbiBody.Size())
		{

			_BOOLEAN skipseg;
			_BOOLEAN lastseg;

			skipseg = (MOTObjSegments.vvbiBody[iSegmCnt].Size() == 0);
			lastseg = (iSegmCnt == MOTObjSegments.vvbiBody.Size() - 1);
			if (lastseg) skipseg = FALSE;
			while (skipseg)
			{
				iSegmCnt++;
				skipseg = (MOTObjSegments.vecbiToSend[iSegmCnt] == 0);
				lastseg = (iSegmCnt == MOTObjSegments.vvbiBody.Size() - 1);
				if (lastseg) skipseg = FALSE;
			}

			/* Check if this is last segment */
			if (iSegmCnt == MOTObjSegments.vvbiBody.Size() - 1)
				bLastSegment = TRUE;
			else
				bLastSegment = FALSE;

			/* Generate MOT object for Body */
			GenMOTObj(vecbiNewData, MOTObjSegments.vvbiBody[iSegmCnt], FALSE, iSegmCnt, iTransportID, bLastSegment);

			iSegmCnt++;
		}

		if (iSegmCnt == MOTObjSegments.vvbiBody.Size())
		{
			/* Reset counter */
			iSegmCnt = 0;
			iTxPictCnt++;

			/* Body is ready, transmit header from next object */
			bCurSegHeader = TRUE;

			/* Signal that old object is done */
			bObjectDone = TRUE;
		}
	}

	/* Return status of object transmission */
	return bObjectDone;
}

void CMOTDABEnc::Reset(int iSegLen)
{
	/* Reset continuity indices */
	iContIndexHeader = 0;
	iContIndexBody = 0;
	iTransportID = 0;
	iTxPictCnt = 0;
	/* Init counter for segments */
	iSegmCnt = 0;
	/* Init segment length */
	iSegmentSize = iSegLen;

	/* Init flag which shows what is currently generated, header or body */
	bCurSegHeader = TRUE; /* Start with header */

	/* Add a "null object" so that at least one valid object can be processed */
	CMOTObject NullObject;
	CVector<short>  NullToSend;
	SetMOTObject(NullObject,NullToSend);
	MOTObjSegments.vecbiToSend.Init(0);
}

int CMOTDABEnc::GetPicCount(void)
{
	if (iTxPictCnt >= 100) iTxPictCnt = 0;
	return iTxPictCnt;
}

/******************************************************************************\
* Decoder                                                                      *
\******************************************************************************/


int iLastGoodTransportID = -1;

CPicPool PicPool;

_BOOLEAN CMOTDABDec::AddDataGroup(CVector<_BINARY>& vecbiNewData)
{
	int			i = 0; //init DM
	int			j = 0; //j for junk reads... DM
	int			iSegmentNum = 0; //init DM
	int			iLenIndicat = 0; //init DM
	int			iLenGroupDataField = 0; //init DM
	int			iSegmentSize = 0; //init DM
	int			iTransportID = 0; //init DM
	_BINARY		biLastFlag = 0; //init DM
	_BINARY		biTransportIDFlag = 0; //init DM
	CCRC		CRCObject;
	_BOOLEAN	bCRCOk = FALSE; //init DM

	/* Init return value with "not ready". If not MOT object is ready after
	   adding this new data group, this flag is overwritten with "TRUE" */
	_BOOLEAN bMOTObjectReady = FALSE;


	/* Get length of data unit */
	iLenGroupDataField = vecbiNewData.Size(); //Does this mean the routine automatically adapts to different size headers? (it should - the header contains it's size) DM


	/* CRC check ------------------------------------------------------------ */
	/* We do the CRC check at the beginning no matter if it is used or not
	   since we have to reset bit access for that */
	   /* Reset bit extraction access */
	vecbiNewData.ResetBitAccess();

	/* Check the CRC of this packet */
	CRCObject.Reset(16);

	/* "- 2": 16 bits for CRC at the end */
	for (i = 0; i < iLenGroupDataField / SIZEOF__BYTE - 2; i++)
		CRCObject.AddByte((_BYTE)vecbiNewData.Separate(SIZEOF__BYTE));

	bCRCOk = CRCObject.CheckCRC(vecbiNewData.Separate(16));
	CRCOK = bCRCOk; //global DM

	/* MSC data group header ------------------------------------------------ */
	/* Reset bit extraction access */
	vecbiNewData.ResetBitAccess();

	/* Extension flag */
	const _BINARY biExtensionFlag = (_BINARY)vecbiNewData.Separate(1);

	/* CRC flag */
	const _BINARY biCRCFlag = 1; //set CRC ON always! DM
	j = (_BINARY)vecbiNewData.Separate(1); //junk read, to keep bit sync DM

	/* Segment flag */
	const _BINARY biSegmentFlag = (_BINARY)vecbiNewData.Separate(1);

	/* User access flag */
	const _BINARY biUserAccFlag = (_BINARY)vecbiNewData.Separate(1);

	/* Data group type */
	const int iDataGroupType = (int)vecbiNewData.Separate(4);

	/* Continuity index (not yet used) */
	j = vecbiNewData.Separate(4); //edited DM

	/* Repetition index (not yet used) */
	//Daz Man - this is now used to send the total segment count in serial form, four bits per data segment
	//vecbiNewData.Separate(4); //Original code DM
//	vecbiNewData.Separate(3); //one less bit DM
//	unsigned int b = vecbiNewData.Separate(1); //Separate data bit here, and process it lower down the page where the segment number has been decoded DM
	//Use all 4 bits now:
	unsigned int b = vecbiNewData.Separate(4); //Separate data bits here, and process it lower down the page where the segment number has been decoded DM - 4 bits now!


	/* Extension field (not used) */
	if (biExtensionFlag == TRUE)
		vecbiNewData.Separate(16);

	/* Session header ------------------------------------------------------- */
	/* Segment field */
	if (biSegmentFlag == TRUE)
	{
		/* Last */
		biLastFlag = (_BINARY)vecbiNewData.Separate(1);

		/* Segment number */
		iSegmentNum = (int)vecbiNewData.Separate(15);

		//=================================================================================
		//Segment erasure information DM
		//keep a record of the CRC status of each packet, and write it into a packed binary global buffer
		//Good segments are marked as 1
		//During segment decoding, the erasures are segment data (one bit per segment)
		//During RS decoding, the segment data is converted into bitmapped byte data (one bit per byte)
		//To do this, each bit needs to be converted into a number of bytes equal to the segment size that was used
		//
		//bCRCOk is the boolean CRC flag

		//OR sets a bit
		//AND the complement of the bit clears the bit
		if (bCRCOk == TRUE) {
			int d = iSegmentNum;
			int e = d & 7;
			//Erasure array bounds checking:
			d = (d >> 3) & 1023; //limit to 0-1023
			if (erasureswitch > 2) erasureswitch = 0; //limit
			erasures[erasureswitch][d] |= 1 << e; //set the bit in the appropriate array
		}
		//The above erasure data could be used to build a graphical good/bad segment graph like EasyPal - DONE - DM

		//=================================================================================
	}

	//Header is 48 bytes to here....

	/* User access field */
	if (biUserAccFlag == TRUE)
	{
		/* Rfa (Reserved for future addition) */
//		vecbiNewData.Separate(3); //original DM
		RxRSlevel = (int) vecbiNewData.Separate(3); //now used for detecting RS coding even if the file header fails DM ===============================================

		/* Transport Id flag */
		biTransportIDFlag = (_BINARY) vecbiNewData.Separate(1);

		/* Length indicator */
		iLenIndicat = (int) vecbiNewData.Separate(4);

		/* Transport Id */
		if (biTransportIDFlag == 1) {
			iTransportID = (int)vecbiNewData.Separate(16);
			
			//Header is 72 bytes to here...
						
			if (bCRCOk == TRUE) {
				//Reset the segtotal registers when the Transport ID changes DM =======================================================================
				if ((iTransportID > 0) && (DecTransportID != iTransportID)) {
					DecTransportID = iTransportID; //Save globally DM
					DecTotalSegs = 0; //reset
					SerialFileSize = 0; //reset 
					DecCheckReg = 0b00001111111111111111111111111111; //reset 28 bits now!
					DecSegSize = 0; //reset

					erasureflags = erasureflags || (1 << erasureswitch); //mark block as used for background erasing

					//Also, swap the erasure buffer to write to each time the Transport ID changes
						erasureswitch = (erasureswitch + 1); //next
					if (erasureswitch > 2) { erasureswitch = 0; }

					/*
					//clear the next buffer here - this can be done here or in Dialog.cpp in the background
					for (int d = 0; d < 1024; d++) {
						erasures[erasureswitch][d] = 0; //clear mem before using it again
					}
					*/ 

					//should the file size be reset here also?
					//HdrFileSize = 0; //
					CompTotalSegs = 0; //
				}
			}
		}

		/* End user address field (not used) */
		int iLenEndUserAddress;

		if (biTransportIDFlag == 1)
			iLenEndUserAddress = (iLenIndicat - 2) * SIZEOF__BYTE;
		else
			iLenEndUserAddress = iLenIndicat * SIZEOF__BYTE;

		j = vecbiNewData.Separate(iLenEndUserAddress); //edited DM
	}

//===========================================================================================================================================================
//Daz Man - Decode the NEW serial data stream with the Total Segment Count, now that both RxRSlevel and iSegmentNum data has arrived * Only using RS mode
//** NOTE: This is a backup method of transferring the RS level used, and the (file) size data, that still works even if the header is missed. Added by DM
	if ((biSegmentFlag == TRUE) && ((biCRCFlag == FALSE) || ((biCRCFlag == TRUE) && (bCRCOk == TRUE))))
	{

		//4 bits at a time
		//unsigned int c = (iSegmentNum & 0x07) << 2; //Mask off lower 3 bits (count 0-7) scale up by 4x
		unsigned int c = (iSegmentNum % 7) << 2; //Modulo to get a wrapping pointer (count 0-6) scale up by 4x
		if (DecCheckReg != 0) {
			//only compute this if we don't have the answer yet
			b = b << c; //shift the bit to its correct position...  0 = bit 0,1,2,3 sent, then 4,5,6,7 shift 4, then 8,9,10,11 shift 8, then 12,13,14,15 shift 12
			SerialFileSize = SerialFileSize | b; //add new bits by logical OR << NEW 32 bit filesize
//			SerialSegTotal = SerialSegTotal | b; //add new bit by logical OR << OLD
			DecCheckReg = DecCheckReg & ~(0x0F << c); //shift 1111 to the corresponding bit positions and clear the bits in the decoder check reg
		}
		if ((DecCheckReg == 0) && (DecSegSize != 0)) {
			DecFileSize = SerialFileSize; //grab new complete data
			//DecTotalSegs = SerialSegTotal; //grab new complete data
			DecTotalSegs = (int)ceil((_REAL)DecFileSize / DecSegSize); //compute total segments from file size

		}

	}
//===========================================================================================================================================================

	/* MSC data group data field -------------------------------------------- */
	/* If CRC is not used enter if-block, if CRC flag is used, it must be ok to
	   enter the if-block */
	if ((biCRCFlag == FALSE) || ((biCRCFlag == TRUE) && (bCRCOk == TRUE)))
	{
		int indexsave = 0; //added DM

		/* Segmentation header ---------------------------------------------- */
		/* Repetition count (not used) */
		j = vecbiNewData.Separate(3); //edited DM

		/* Segment size */
		iSegmentSize = (int)vecbiNewData.Separate(13);


		if ((iSegmentSize > 0) && (iSegmentNum > 0) && (biLastFlag == 0) && (DecSegSize == 0)) {
			//this only updates when the transmit ID changes (new file) DM
			//that prevents the last (smaller) segment giving a false reading DM
			DecSegSize = iSegmentSize; //global copy DM =============================================================================
			//also, save the segment size that was used for each erasure buffer DM
			erasuressegsize[erasureswitch] = iSegmentSize; //save to the correct array - this should be made to only update once... (although the LAST segment is SMALLER - but in RS mode, data is rounded up to 255 byte blocks)
			//DMfilename[0] = 0; //clear filename?
			strcpy(DMdecodestat, "WAIT"); //RS decoder stats message
		}

		//Header is 88 bytes to here

		/* Get MOT data ----------------------------------------------------- */
		/* Segment number and user access data is needed */
		if ((biSegmentFlag == TRUE) && (biUserAccFlag == TRUE) && (biTransportIDFlag == 1))
		{
			/* Distinguish between header and body */
			/* Header information, i.e. the header core and the header
			   extension, are transferred in Data Group type 3 */
			if (iDataGroupType == 3)
			{
				/* Header --------------------------------------------------- */
				if (iSegmentNum == 0)
				{
					/* Header */

					/* The first segment was received, reset header */

					DMRSpsize = 0; //added DM ==============================================

					if (MOTObjectRaw.iTransportID != iTransportID)
					{
						// store in pool
						PicPool.storeinpool(MOTObjectRaw);
						PicPool.getfrompool(iTransportID, MOTObjectRaw);
					}

					MOTObjectRaw.Header.Reset();

					/* The first occurrence of a Data Group type 3 containing
					   header information is referred as the beginning of the
					   object transmission. Set new transport ID */
					MOTObjectRaw.iTransportID = iTransportID;

					/* Init flag for header ok */
					MOTObjectRaw.Header.bOK = TRUE;

					/* Add new segment data */
					MOTObjectRaw.Header.Add(vecbiNewData, iSegmentSize, iSegmentNum);

				}
				else
				{
					MOTObjectRaw.Header.Reset();
					MOTObjectRaw.BodyRx.Reset();
					MOTObjectRaw.iTransportID = iTransportID;
				}

				/* Test if last flag is active */
				if (biLastFlag == TRUE)
				{
					if (MOTObjectRaw.Header.bOK == TRUE)
					{
						/* This was the last segment and all segments were ok.
						   Mark header as ready */
						MOTObjectRaw.Header.bReady = TRUE;
					}
				}
			}
			else if (iDataGroupType == 4)
			{
				/* Body data segments are transferred in Data Group type 4 */
				/* Body ----------------------------------------------------- */
				{

					/* Check transport ID */
					if (MOTObjectRaw.iTransportID == iTransportID)
					{
						if (MOTObjectRaw.iSegmentSize == 0)			// was not initialized
						{
							MOTObjectRaw.iSegmentSize = iSegmentSize;
						}

						if ((MOTObjectRaw.iSegmentSize != iSegmentSize) && (biLastFlag == FALSE))
						{
							// mode change, remove all.
							PicPool.poolremove(iTransportID);
							PicPool.getfrompool(iTransportID, MOTObjectRaw);
						}

						/* Init flag for body ok */
						MOTObjectRaw.BodyRx.bOK = TRUE;

						MOTObjectRaw.BodyRx.Add(vecbiNewData, iSegmentSize, iSegmentNum); //This is where the incoming segment bits get added DM
						MOTObjectRaw.iActSegment = iSegmentNum;

#define NEWCODE TRUE
#if NEWCODE
						//NEW CODE DM =================================
						//This code copies the bit data in byte form to the new RS buffer, even if segment 0 (header) is missing
						//WORKING 2:45am 10th Sep 2021
						//only execute if the CRC is good
							if (bCRCOk) {
								if (iSegmentSize > DMRSpsize) {
									DMRSpsize = iSegmentSize;
								};
								MOTObjectRaw.BodyRx.vvbiSegment[iSegmentNum].ResetBitAccess(); //Start reading from zero
								unsigned int j = (iSegmentNum * DMRSpsize); //compute - the base index is the previous segment size * segment number (to avoid an error on the last segment, which is smaller)
								DMRSpsize = iSegmentSize; //update
								unsigned int k = MOTObjectRaw.BodyRx.vvbiSegment[iSegmentNum].size() / 8; //find total bits for this segment /8
								unsigned int a = 0;
								unsigned char dat = 0;
								const unsigned limit = size(GlobalDMRxRSData); //get the buffer size
								if (k > 0) {
									for (unsigned int i = 0; i < k; i++)
									{
										//a bounds check on the computed index might be wise... DM
										a = j + i;
										if (a > limit) { a = limit - 1; } //bounds limit
										GlobalDMRxRSData[a] = MOTObjectRaw.BodyRx.vvbiSegment[iSegmentNum].Separate(8); //grab a byte each time
									}
								}
							}
						//}
						//END NEW CODE DM =================================
#endif //NEWCODE
					}
					else
					{
						// store in pool - when the ID changes (new file)
						PicPool.storeinpool(MOTObjectRaw);
						PicPool.getfrompool(iTransportID, MOTObjectRaw);  //why? DM
						if (biLastFlag == FALSE) MOTObjectRaw.iSegmentSize = iSegmentSize;
					}
				}
				/* Test if last flag is active */
				if (biLastFlag == TRUE)
				{
					// set tot seg number
					MOTObjectRaw.BodyRx.iTotSegments = iSegmentNum + 1;
				}

				if (MOTObjectRaw.BodyRx.bOK == TRUE)
				{
					/* This was the last segment and all segments were ok.
					   Mark body as ready */
					if (MOTObjectRaw.BodyRx.iTotSegments >= 0)
					{
						if (MOTObjectRaw.BodyRx.iDataSegNum >= MOTObjectRaw.BodyRx.iTotSegments)
						{
							MOTObjectRaw.BodyRx.bReady = TRUE;
						}
					}
				}
			}

			/* Use MOT object ----------------------------------------------- */
			/* Test if MOT object is ready */
			if ((MOTObjectRaw.Header.bReady == TRUE) && (MOTObjectRaw.BodyRx.bReady == TRUE))
			{
				int mysegsiz;
				BOOL allfull = TRUE;
				// Copy BodyRx to Body
				MOTObjectRaw.Body.Reset();
				for (int i = 0; i < MOTObjectRaw.BodyRx.iTotSegments; i++)
				{
					mysegsiz = MOTObjectRaw.BodyRx.vvbiSegment[i].Size();
					if (mysegsiz <= 0) allfull = FALSE;
					MOTObjectRaw.BodyRx.vvbiSegment[i].ResetBitAccess();
					MOTObjectRaw.Body.Add(MOTObjectRaw.BodyRx.vvbiSegment[i], mysegsiz / SIZEOF__BYTE, i);
				}

				if (allfull)
				{
					// remove from pool
					PicPool.poolremove(MOTObjectRaw.iTransportID);
					DecodeObject(MOTObjectRaw);

					/* Set flag that new object was successfully decoded */
					bMOTObjectReady = TRUE;

					/* Reset raw MOT object */
					MOTObjectRaw.Header.Reset();
					MOTObjectRaw.BodyRx.Reset();

					iLastGoodTransportID = MOTObjectRaw.iTransportID;
				}
				else
				{
					MOTObjectRaw.BodyRx.bReady = FALSE;
					bMOTObjectReady = FALSE;
				}
			}

		}
	}

	if ((iSegmentNum == 0) && (bCRCOk)) {
		GetName(MOTObjectRaw); //added to read the filename and size from the old header DM ======================================
	}

	/* Return status of MOT object decoding */
	return bMOTObjectReady;
}

//string strName; //this is where the received filename is saved for the GetName routine DM
string strName2 = {0}; //this is where the received filename is saved for the GetName routine DM

void GetName(CMOTObjectRaw& MOTObjectRaw)
{
	int				i;
	unsigned char	ucDatafield;
	MOTObjectRaw.Header.vecbiData.ResetBitAccess();
	HdrFileSize = (int) MOTObjectRaw.Header.vecbiData.Separate(28); //Read file size from header

	const int iHeaderSize = (int) MOTObjectRaw.Header.vecbiData.Separate(13);
	i = (int) MOTObjectRaw.Header.vecbiData.Separate(15);
	int iSizeRec = iHeaderSize - 7;
	while (iSizeRec > 0)
	{
		int iPLI = (int) MOTObjectRaw.Header.vecbiData.Separate(2);
		switch(iPLI)
		{
		case 0:
			iSizeRec -= 1; 
			break;
		case 1:
			i = (int)MOTObjectRaw.Header.vecbiData.Separate(14);
			iSizeRec -= 2; 
			break;
		case 2:
			i = (int)MOTObjectRaw.Header.vecbiData.Separate(18);
			i = (int)MOTObjectRaw.Header.vecbiData.Separate(20);
			iSizeRec -= 5; 
			break;
		case 3:
			i = (int)MOTObjectRaw.Header.vecbiData.Separate(6);
			iSizeRec -= 1; 
			unsigned char ucExt = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(1);
			int iDataFieldLen = 0;
			if (ucExt == 0)
			{
				iDataFieldLen = (int)MOTObjectRaw.Header.vecbiData.Separate(7);
				iSizeRec -= 1;
			}
			else
			{
				iDataFieldLen = (int)MOTObjectRaw.Header.vecbiData.Separate(15);
				iSizeRec -= 2;
			}

			//Updated code
			int j = 0;
			i = 0;
			if (iDataFieldLen > 80) { iDataFieldLen = 80; } //bounds check DM
			while (i < iDataFieldLen) {
				ucDatafield = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(8);
				if (ucDatafield != 0) {
					strName2 += ucDatafield;		//Read incoming filename DM
					DMfilename[j] = ucDatafield; //save here too DM
					j++;
				}
				i++;
			}
			DMfilename[j] = 0; //null terminate DM

			//Original code:
			/*
			strName2 = "";
			for (i = 0; i < iDataFieldLen; i++)
			{
				ucDatafield = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(8);
				if (ucDatafield != 0) strName2 += ucDatafield;
				DMfilename
			}
			*/
			break;
		}
	}
}
	
_BOOLEAN	CMOTDABDec::GetActBSR(int * iNumSeg, string * bsr_name, char * path, int * iHash)
{
	FILE * bsr;
	char filenam[300];

	if (iLastGoodTransportID == MOTObjectRaw.iTransportID)
	{
		return FALSE;
	}
	else if (MOTObjectRaw.BodyRx.iDataSegNum <= 0)
	{
		return FALSE;
	}
	else
	{
		int segsize = 116; //why is this set here? A default? Appears that way... DM
		int i;
		int sct = 0;
		wsprintf(filenam,"%s%s",path,"bsr.bin");
		bsr = fopen(filenam,"wt");
		fprintf(bsr,"%d\n",MOTObjectRaw.iTransportID);
		*iHash = MOTObjectRaw.iTransportID;
		if (MOTObjectRaw.Header.bReady == TRUE)
		{
			GetName(MOTObjectRaw); //Getname reads the filename directly from the saved serial header bits DM
			fprintf(bsr,"H_OK\n");
			*bsr_name = strName2;
		}
		else
		{
			fprintf(bsr,"%s\n","H_NO");
			*bsr_name = "No_Header";
		}
		// Search segment size
		for (i = 0; i < MOTObjectRaw.BodyRx.vvbiSegment.Size(); i++)
		{
			segsize = MOTObjectRaw.BodyRx.vvbiSegment[i].Size();
			if (segsize > 0) i = MOTObjectRaw.BodyRx.vvbiSegment.Size();
		}
		fprintf(bsr,"%d\n",segsize/SIZEOF__BYTE);
	
		// Copy BodyRx to Body
		MOTObjectRaw.Body.Reset();
		for (i = 0; i < MOTObjectRaw.BodyRx.vvbiSegment.Size(); i++)
		{
			if (MOTObjectRaw.BodyRx.vvbiSegment[i].Size() <= 0)
			{
				sct++;
				fprintf(bsr,"%d\n",i);
			}
		}
		fclose(bsr);
		*iNumSeg = sct;
		return TRUE;
	}
}

_BOOLEAN	CMOTDABDec::GetActMOTSegs(CVector<_BINARY>& vSegs)
{
	int i, size;
	size = MOTObjectRaw.BodyRx.vvbiSegment.Size();
	if (size >= 650) size = 650;
	vSegs.Init(size);
	Mutex.Lock();
	for (i = 0; i < size; i++)
		vSegs[i] = (MOTObjectRaw.BodyRx.vvbiSegment[i].Size() > 0);
	Mutex.Unlock();
	return TRUE;
}

_BOOLEAN	CMOTDABDec::GetActMOTObject(CMOTObject& NewMOTObject)
{
	if (iLastGoodTransportID == MOTObjectRaw.iTransportID)
		return FALSE;
	if (MOTObjectRaw.Header.bReady == TRUE) //modified DM
	{
		int segsize = 116;
		int i;

		/* Lock resources */
		Mutex.Lock();

		// Search segment size
		for (i = 0; i < MOTObjectRaw.BodyRx.vvbiSegment.Size(); i++)
		{
			segsize = MOTObjectRaw.BodyRx.vvbiSegment[i].Size();
			if (segsize > 0) i = MOTObjectRaw.BodyRx.vvbiSegment.Size();
		}

		// Copy BodyRx to Body
		MOTObjectRaw.Body.Reset();
		for (i = 0; i < MOTObjectRaw.BodyRx.vvbiSegment.Size(); i++)
		{
			if (MOTObjectRaw.BodyRx.vvbiSegment[i].Size() > 0)
			{
				MOTObjectRaw.BodyRx.vvbiSegment[i].ResetBitAccess();
				MOTObjectRaw.Body.Add(MOTObjectRaw.BodyRx.vvbiSegment[i], MOTObjectRaw.BodyRx.vvbiSegment[i].Size()/SIZEOF__BYTE, i);
			}
			else
			{
				CVector<_BINARY>	DummyData;
				DummyData.Init(segsize);
				MOTObjectRaw.Body.Add(DummyData,DummyData.Size()/SIZEOF__BYTE,i);
			}

		}
		//DecodeObject(MOTObjectRaw); //added DM
		NewMOTObject = MOTObject;

		/* Release resources */
		Mutex.Unlock();

		return TRUE;
	}
	return FALSE;
}

void CMOTDABDec::DecodeObject(CMOTObjectRaw& MOTObjectRaw)
{
	int				i;
	int				iDaSiBytes;
	unsigned char	ucParamId;
	unsigned char	ucDatafield;

	/* Header --------------------------------------------------------------- */ //File header (containing Filename, size etc)
	MOTObjectRaw.Header.vecbiData.ResetBitAccess();

	/* HeaderSize and BodySize */
	const int iBodySize = (int) MOTObjectRaw.Header.vecbiData.Separate(28); //this is the incoming file total size in bytes from the file header DM
	
	HdrFileSize = iBodySize; //Added DM

	CompTotalSegs = (int)ceil((_REAL)iBodySize / DecSegSize); //Added DM

	const int iHeaderSize = (int) MOTObjectRaw.Header.vecbiData.Separate(13);

	/* 7 bytes for header core */
	int iSizeRec = iHeaderSize - 7;

	/* Content type and content sup-type */
	//const int iContentType = (int)MOTObjectRaw.Header.vecbiData.Separate(6);
	int iContentType = (int)MOTObjectRaw.Header.vecbiData.Separate(6);
	//const int iContentSubType = (int)MOTObjectRaw.Header.vecbiData.Separate(9);
	int iContentSubType = (int)MOTObjectRaw.Header.vecbiData.Separate(9);

	/* Use all header extension data blocks */
	while (iSizeRec > 0)
	{
		/* PLI (Parameter Length Indicator) */
		int iPLI = (int) MOTObjectRaw.Header.vecbiData.Separate(2);

		switch (iPLI)
		{
		case 0:
			/* Total parameter length = 1 byte; no DataField
			   available */
			ucParamId = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(6);

			/* TODO: Use "ucParamId" */

			iSizeRec -= 1; /* 6 + 2 (PLI) bits */
			break;

		case 1:
			/* Total parameter length = 2 bytes, length of DataField
			   is 1 byte */
			ucParamId = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(6);

			ucDatafield = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(8);

			/* TODO: Use information in data field */

			iSizeRec -= 2; /* 6 + 8 + 2 (PLI) bits */
			break;

		case 2:
			/* Total parameter length = 5 bytes; length of DataField
			   is 4 bytes */
			ucParamId = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(6);

			for (i = 0; i < 4; i++)
			{
				ucDatafield = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(8);

				/* TODO: Use information in data field */
			}
			iSizeRec -= 5; /* 6 + 4 * 8 + 2 (PLI) bits */
			break;

		case 3:
			/* Total parameter length depends on the DataFieldLength
			   indicator (the maximum parameter length is
			   32770 bytes) */
			ucParamId = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(6);

			iSizeRec -= 1; /* 2 (PLI) + 6 bits */

			/* Ext (ExtensionFlag): This 1-bit field specifies the
			   length of the DataFieldLength Indicator and is coded
			   as follows:
			   - 0: the total parameter length is derived from the
					next 7 bits;
			   - 1: the total parameter length is derived from the
					next 15 bits */
			unsigned char ucExt = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(1);

			int iDataFieldLen = 0;

			/* Get data field length */
			if (ucExt == 0)
			{
				iDataFieldLen = (int)MOTObjectRaw.Header.vecbiData.Separate(7);

				iSizeRec -= 1;
			}
			else
			{
				iDataFieldLen = (int)MOTObjectRaw.Header.vecbiData.Separate(15);

				iSizeRec -= 2;
			}

			/* Get data */
			MOTObject.strName = "";
			i = 0;
			int j = 0;
			while (i < iDataFieldLen) {
				ucDatafield = (unsigned char)MOTObjectRaw.Header.vecbiData.Separate(8);
				if (ucDatafield != 0) {
					MOTObject.strName += ucDatafield;		//Read incoming filename DM
					DMfilename[j] = ucDatafield; //save here too DM
					j++;
				}
				i++;
			}
			DMfilename[j] = 0; //null terminate DM
			break;
		}

	}

	/* Body ----------------------------------------------------------------- */
	/* We only use images */
	if (iContentType == 2 /* image */)
	{
		/* Check range of content sub type */
		if (iContentSubType < 4)
		{
			/* Set up MOT picture ------------------------------------------- */
			/* Reset bit access to extract data from body */
			MOTObjectRaw.Body.vecbiData.ResetBitAccess();

			/* Data size in bytes */
			iDaSiBytes = MOTObjectRaw.Body.vecbiData.Size() / SIZEOF__BYTE;

			/* Copy TransportID */
			MOTObject.iTransportID = MOTObjectRaw.iTransportID;

			/* Copy data */
			MOTObject.vecbRawData.Init(iDaSiBytes);
			for (int i = 0; i < iDaSiBytes;	i++)
				MOTObject.vecbRawData[i] = (_BYTE) MOTObjectRaw.Body.vecbiData.Separate(SIZEOF__BYTE); //DM This reads bytes of data from the segment buffers out to the output buffer, 1 byte at a time
		}
	}
}

void CMOTObjectRaw::CDataUnit::Add(CVector<_BINARY>& vecbiNewData, const int iSegmentSize, const int iSegNum)
{
	/* Add new data (bit-wise copying) */
	const int iOldDataSize = vecbiData.Size();
	const int iNewEnlSize = iSegmentSize * SIZEOF__BYTE;

	vecbiData.Enlarge(iNewEnlSize);

	/* Read useful bits. We have to use the "Separate()" function since we have
	   to start adding at the current bit position of "vecbiNewData" */
	for (int i = 0; i < iNewEnlSize; i++)
		vecbiData[iOldDataSize + i] = (_BINARY) vecbiNewData.Separate(1);

	/* Set new segment number */
	iDataSegNum = iSegNum;
}

void CMOTObjectRaw::CDataUnit::Reset()
{
	vecbiData.Init(0);
	bOK = FALSE;
	bReady = FALSE;
	iDataSegNum = -1;
}

void CMOTObjectRaw::CDataUnitRx::Add(CVector<_BINARY>& vecbiNewData, const int iSegmentSize, const int iSegNum)
{
	int i;
	/* Add new data (bit-wise copying) */
	const int iNewDataSize = iSegmentSize * SIZEOF__BYTE;
	const int iOldEnlSize  = vvbiSegment.Size();
	const int iNewEnlSize  = iSegNum+1;

	if (iNewEnlSize > iOldEnlSize)
		vvbiSegment.Enlarge(iNewEnlSize-iOldEnlSize);

	vvbiSegment[iSegNum].Init(iNewDataSize);

	/* Read useful bits. We have to use the "Separate()" function since we have
	   to start adding at the current bit position of "vecbiNewData" */
	for (i = 0; i < iNewDataSize; i++)
		vvbiSegment[iSegNum][i] = (_BINARY) vecbiNewData.Separate(1);

	/* Set new segment number */
	iDataSegNum = 0;
	for (i = 0; i < vvbiSegment.Size(); i++)
		if (vvbiSegment[i].Size() >= 1) iDataSegNum++;

}

void CMOTObjectRaw::CDataUnitRx::Reset()
{
	vvbiSegment.Init(0); 
	bOK = FALSE;
	bReady = FALSE;
	iDataSegNum = 0;
	iTotSegments = -1;
}

