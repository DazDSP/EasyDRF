/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *
 * Description:
 *	MOT Slide Show application
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

#define ZLIB_WINAPI
#define ZLIB_DLL
#define ZLIB_INTERNAL

#include <windows.h> //added DM
#include "MOTSlideShow.h"
#include "../../zlib.h"  //added DM
#include "../../getfilenam.h"
#include "../../RS-defs.h" //added DM for RS code
#include "../RS/RS-coder.h"
#include "../../7zTypes.h"
#include "../../LzmaLib.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Encoder                                                                      *
\******************************************************************************/
void CMOTSlideShowEncoder::GetDataUnit(CVector<_BINARY>& vecbiNewData)
{
	/* Get new data group from MOT encoder. If the last MOT object was
	   completely transmitted, this functions returns true. In this case, put
	   a new picture to the MOT encoder object */
	if (MOTDAB.GetDataGroup(vecbiNewData) == TRUE)
		AddNextPicture();
}

void CMOTSlideShowEncoder::Init(int iSegSize)
{
	/* Reset picutre counter for browsing in the vector of file names. Start
	   with first picture */
	iPictureCnt = 0;

	MOTDAB.Reset(iSegSize);

	iSegmentSize = iSegSize;

	AddNextPicture();
}

void CMOTSlideShowEncoder::AddNextPicture()
{
	CVector<short> dummy;

	/* Make sure at least one picture is in container */
	if (vecMOTPicture.Size() > 0)
	{
		/* Get current file name */
		MOTDAB.SetMOTObject(vecMOTPicture[iPictureCnt],vecMOTSegments[iPictureCnt]);

		/* Set file counter to next picture, test for wrap around */
		iPictureCnt++;
		if (iPictureCnt == vecMOTPicture.Size())
			iPictureCnt = 1;
	}
}

void CMOTSlideShowEncoder::AddFileName(const string& strFileName, const string& strFileNamenoDir, CVector<short>  vecsToSend)
{
	/* Only ContentSubType "JFIF" (JPEG) and ContentSubType "PNG" are allowed
	   for SlideShow application (not tested here!) */
	/* For Ham-DRM this doesn't matter - any file works! DM */
	/* Try to open file binary */

	int iOldNumObj;
	FILE * pFiBody = fopen(strFileName.c_str(), "rb");

	if (pFiBody != NULL)
	{
		//		_BYTE byIn; //this is the original file buffer - only 1 byte long DM
		//=============================================================================================================================
		const uLongf BUFSIZE = 524288; //512k should be enough for anything practical - HEAP STORAGE
		_BYTE* buffer1T = new _BYTE[BUFSIZE*2]; //File read buffer - now 1M to fit RS encoding
		_BYTE* buffer2T = new _BYTE[BUFSIZE*2]; //zlib & interleaver buffer - now 1M to fit RS encoding
		uLongf filesize;
		unsigned int dd;
		int result;
		filesize = 0;

		// obtain file size : (there might be a more elegant way to do this...)
		fseek(pFiBody, 0, SEEK_END);
		filesize = ftell(pFiBody);
		if (filesize > BUFSIZE) filesize = BUFSIZE; //prevent buffer overrun - limit to 512k
		rewind(pFiBody);

		//Daz Man: Before we make the header, we need to figure out what the filename extension is going to be
		LPSTR dx1 = const_cast<char*>(strFileNamenoDir.c_str()); //the filename that is being sent
		const string& dx2 = ".lz"; //the 2nd filename extension that denotes LZMA compression is used
		string strFileNamenoDirX; //the variable that holds the modified filename
		_BOOLEAN compressit = !checkext(dx1); //check list of file types
//		compressit = FALSE; //TEST ===================================================================================================================

		if (compressit) {
			strFileNamenoDirX = strFileNamenoDir.c_str() + dx2; //attach extra file extension to denote compression is used
		}
		else {
			strFileNamenoDirX = strFileNamenoDir.c_str(); //copy normal filename
		}
		
		//Only add the new header if using the new RS mode
		int HeaderSize = 0;
		if (ECCmode > 3) {
			//Daz Man:
			//Write a header for all files, containing the filename, the filesize and the header size to guarantee this data is available if the file decodes ok.
			//write header into buffer2T before file copy or gzip write, then read it out directly or RS code it into buffer1T
			string EZHeaderID = "EasyDRFHeader/|000000" + strFileNamenoDirX; //reserve space for numerics using zeroes
			HeaderSize = size(EZHeaderID); //length of header - add to the filesize used in the header
			int i = 15; //first byte after version number
			EZHeaderID[i++] = (2); //version number
			EZHeaderID[i++] = (HeaderSize & 0xFF); //byte 1 of header size
			EZHeaderID[i++] = (HeaderSize & 0xFF00) >> 8; //byte 2 of header size
			EZHeaderID[i++] = ((filesize) & 0xFF); //byte 1 of file size
			EZHeaderID[i++] = ((filesize) & 0xFF00) >> 8; //byte 2 of file size
			EZHeaderID[i++] = ((filesize) & 0xFF0000) >> 16; //byte 3 of file size

			//Now write new header into buffer2T
			i = 0;
			while (i < HeaderSize) {
				buffer2T[i] = EZHeaderID[i]; //this is normally buffer2T
				i++;
			}
		}

		//Daz Man:
		//if the input data is compressible (selected file types) then send data to buffer1T
		//else send data to buffer2T
		//add the text file zlib compressor in here - read the data from buffer1T and put compressed data into buffer2T
		//keep the filename data intact - just append the extra .gz extension to strFileNamenoDir
		//change the existing routines below to read from the buffer output
		//
		//test the filename extension
		if (compressit) {
			//read into buffer1T if it's compressible
			result = fread(buffer1T, 1, filesize, pFiBody); //read the file to send into buffer1T if it is text
			//uLongf ds = BUFSIZE; //tell zlib the buffer size we are using
			//start at buffer2T + Headersize to avoid overwriting the header (if no header is used, Headersize is zero)
			//compress2(buffer2T + HeaderSize, &ds, buffer1T, filesize, 9); //zlib compress text data and compressible files (use the headerless filesize)
			//BYTE* buffer2Th = buffer2T + HeaderSize;
			unsigned int ds = BUFSIZE*2;

#define LZMA_PROPS_SIZE 5;
				unsigned propsSize = LZMA_PROPS_SIZE;
				//set these accordingly:
				int level = 9; //Maximum compression
				unsigned int dictSize = 1 << 25; //24
				int lc = 3;
				int lp = 0;
				int pb = 2;
				int fb = 255; // 32; //block size
				int numThreads = 1;
				//save original filesize after props, as LZMA isn't accurate on data size
				int i = 0;
				(buffer2T + HeaderSize + propsSize)[i] = filesize & 0xFF; //byte 1
				i++; //next
				(buffer2T + HeaderSize + propsSize)[i] = (filesize & 0x00FF00) >> 8; //byte 2
				i++; //next
				(buffer2T + HeaderSize + propsSize)[i] = (filesize & 0x00FF0000) >> 16; //byte 3

				//                     dest to write to      dlen  src read  src size outprops, size, 
				int res = LzmaCompress(buffer2T+HeaderSize+propsSize+3, &ds, buffer1T, filesize, buffer2T+HeaderSize, &propsSize, level, dictSize, lc, lp, pb, fb, numThreads);
				//if RS is not used, there is no new header, and HeaderSize is 0
				
				//compressed file data size is in ds, and add props data size also
				filesize = ds + propsSize+3; //update filesize with the new compressed file data size AND the LZMA props AND the new 3-byte filesize data
		}
		else {
			//read into buffer2T directly for all else
			//start at buffer2T + Headersize to avoid overwriting the header (if no header is used, Headersize is zero)
			result = fread(buffer2T+HeaderSize, 1, filesize, pFiBody); //this is normally buffer2T
		}

		//if (result != filesize) { fputs("Reading error", stderr); exit(3); } //does it need error checking? NO - this could leak memory by not deleting the buffer arrays

		fclose(pFiBody); //close file
		filesize = filesize + HeaderSize; //add header if it's non-zero
//=============================================================================================================================
		//RS encoding can go here, with header added first  DM
		//Only execute this if ECCmode is > 3

		if (ECCmode > 3) {

			//Put RS coding routine here - Daz Man 2021
			//Compressed or uncompressed data (+header) is in buffer2T
			//RS encode the data from buffer2T and put it in buffer1T
			//Then read out buffer1T to the sending routine next
			//or read out buffer2T if RS is not used

			//RS encode here
			//filesize = filesize + HeaderSize; //add size of new header in - already done now...
			int lasterror = 0;
			if (ECCmode == 4) {
				//RS1
				lasterror = rs1encode(buffer2T, buffer1T, filesize); //
				filesize = ceil((float)filesize / 224) * 255; //allow for bigger data size
			}
			else if (ECCmode == 5) {
				//RS2
				lasterror = rs2encode(buffer2T, buffer1T, filesize); //
				filesize = ceil((float)filesize / 192) * 255; //allow for bigger data size
			}
			else if (ECCmode == 6) {
				//RS3
				lasterror = rs3encode(buffer2T, buffer1T, filesize); //
				filesize = ceil((float)filesize / 160) * 255; //allow for bigger data size
			}
			else if (ECCmode == 7) {
				//RS4
				lasterror = rs4encode(buffer2T, buffer1T, filesize); //
				filesize = ceil((float)filesize / 128) * 255; //allow for bigger data size
			}

			//compute the data exactly for the transmission, but don't let the RS decoder decode junk
			//make sure the number of RS blocks to be decoded is the same as what was encoded
			//Special interleaver test
			//it is critical that the same filesize be used for interleaving and deinterleaving
			bool rev = 0;
			distribute(buffer1T, buffer2T, filesize, rev); //output is put back into buffer2T
		}

		//data should be in buffer 2 now

//=============================================================================================================================
//		FILE* pFiBody = fopen(strFileName.c_str(), "rb"); //Open file - Now we read from buffer only - OLD CODE DM

		//Add code to truncate filename if it's longer than 79 characters - this is already done later - but it needs to be more elegant, so it doesn't cut the extensions off... TODO DM

		if (vecMOTPicture.Size() == 0)
		{
			int i = 0, k = 0; //init DM
			int actsize = 0;
			for (i=0;i<the_startdelay;i++)
			{
				/* Enlarge vector storing the picture objects */
				iOldNumObj = vecMOTPicture.Size();
				vecMOTPicture.Enlarge(1);
				vecMOTSegments.Enlarge(1);
	
				/* Store file name and format string */
//				vecMOTPicture[iOldNumObj].strName = strFileNamenoDir; //edited DM
				vecMOTPicture[iOldNumObj].strName = strFileNamenoDirX; //use updated filename instead, in case it needs a .gz extension
				vecMOTPicture[iOldNumObj].strNameandDir = strFileName;
	
				/* Fill body data with content of selected file */
				vecMOTPicture[iOldNumObj].vecbRawData.Init(0);
				vecMOTSegments[iOldNumObj].Init(the_startdelay);
				vecMOTPicture[iOldNumObj].bIsLeader = (i == 0); //mark as leader if i == 0 DM
				for (k=0;k<the_startdelay;k++)
					vecMOTSegments[iOldNumObj][k] = k;
	
				dd = 0;
				//During Lead-In only
				//If no RS coding is used, read from buffer2T
				//If RS coding is used, read from buffer1T but write the new header first
				if (ECCmode > 3) {
					while (dd < filesize)  //Read from buffer up to data size This is a read for the leadin - not all of it is used
					{
						/* Add one byte = SIZEOF__BYTE bits */
						vecMOTPicture[iOldNumObj].vecbRawData.Enlarge(SIZEOF__BYTE);
						vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)buffer2T[dd], SIZEOF__BYTE); //from new buffer2T
//						vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)buffer1T[dd], SIZEOF__BYTE); //from new buffer1T
						dd++; //next
					}
				}
				else {
					while (dd < filesize)  //Read from buffer up to data size - This is a read for the leadin - not all of it is used
					{
					/* Add one byte = SIZEOF__BYTE bits */
						vecMOTPicture[iOldNumObj].vecbRawData.Enlarge(SIZEOF__BYTE);
						vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)buffer2T[dd], SIZEOF__BYTE); //from new buffer2T
						dd++; //next
					}
				}

				actsize += iSegmentSize;
				actsize += vecMOTPicture[iOldNumObj].vecbRawData.Size() / SIZEOF__BYTE;
				if (actsize >= iSegmentSize*the_startdelay) i = the_startdelay;
			}
		}

		/* Enlarge vector storing the picture objects */
		iOldNumObj = vecMOTPicture.Size();
		vecMOTPicture.Enlarge(1);
		vecMOTSegments.Enlarge(1);

		/* Store file name and format string */
//		vecMOTPicture[iOldNumObj].strName = strFileNamenoDir;
		vecMOTPicture[iOldNumObj].strName = strFileNamenoDirX; //use updated filename DM
		vecMOTPicture[iOldNumObj].strNameandDir = strFileName;

		/* Fill body data with content of selected file */
		const int vecsegsize = vecsToSend.Size();
		vecMOTPicture[iOldNumObj].vecbRawData.Init(0);
		vecMOTSegments[iOldNumObj].Init(vecsegsize);
		for (int k=0;k<vecsegsize;k++)
			vecMOTSegments[iOldNumObj][k] = vecsToSend[k];
		vecMOTPicture[iOldNumObj].bIsLeader = FALSE;

		dd = 0;
		if (ECCmode > 3) {
			//FOR RS CODING
			while (dd < filesize)  //Read from buffer up to data size
			{
				/* Add one byte = SIZEOF__BYTE bits */
				vecMOTPicture[iOldNumObj].vecbRawData.Enlarge(SIZEOF__BYTE);
				vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)buffer2T[dd], SIZEOF__BYTE); //from new buffer2T
//				vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)buffer1T[dd], SIZEOF__BYTE); //from new buffer1T
				dd++; //next
			}
		}
		else {
			//NO RS CODING
			while (dd < filesize)  //Read from buffer up to data size
			{
				/* Add one byte = SIZEOF__BYTE bits */
				vecMOTPicture[iOldNumObj].vecbRawData.Enlarge(SIZEOF__BYTE);
				vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)buffer2T[dd], SIZEOF__BYTE); //from new buffer2T
				dd++; //next
			}
		}

		//remove the buffer arrays from the heap DM
		delete[] buffer1T;
		delete[] buffer2T;
	}
}

void CMOTSlideShowEncoder::SetMyStartDelay(int delay)
{
	the_startdelay = delay;
}
int CMOTSlideShowEncoder::GetPicPerc(void)
{
	int segct = 0, totseg = 0; //init DM
	segct = MOTDAB.GetPicSegmAct();
	totseg = MOTDAB.GetPicSegmTot();
	if (segct == 0) return 0;
	if (totseg == 0) return 100;
	return (100 * segct) / totseg;
}



/******************************************************************************\
* Decoder                                                                      *
\******************************************************************************/
void CMOTSlideShowDecoder::AddDataUnit(CVector<_BINARY>& vecbiNewData)
{
	/* Add new data group (which is in one DRM data unit if SlideShow
	   application is used) and check if new MOT object is ready after adding
	   this new data group */
	if (MOTDAB.AddDataGroup(vecbiNewData) == TRUE)
	{
		/* Get new received SlideShow picture */
		MOTDAB.GetMOTObject(MOTPicture);
		bNewPicture = TRUE; /* Set flag for new picture */
	}
}

_BOOLEAN CMOTSlideShowDecoder::GetPicture(CMOTObject& NewPic)
{
	const int iRawDataSize = MOTPicture.vecbRawData.Size();

	/* Init output object */
	NewPic.Reset();

	/* Only copy picture content if picture is available */
	if (iRawDataSize != 0)
	{
		NewPic = MOTPicture;
	}

	/* Check if this is an old or a new picture and return result */
	_BOOLEAN bWasNewPicture = FALSE;
	if (bNewPicture == TRUE)
	{
		bNewPicture = FALSE;
		bWasNewPicture = TRUE;
	}

	return bWasNewPicture;
}

_BOOLEAN CMOTSlideShowDecoder::GetPartPicture(CMOTObject& NewPic)
{
	/* Init output object */
	NewPic.Reset();

	/* Only copy picture content if picture is available */
	return MOTDAB.GetActMOTObject(NewPic); //edited DM
}


_BOOLEAN CMOTSlideShowDecoder::GetActSegments(CVector<_BINARY>& NewSeg)
{
	return MOTDAB.GetActMOTSegs(NewSeg);
}

int ihash;

_BOOLEAN CMOTSlideShowDecoder::GetPartBSR(int * iNumSeg, string * bsrname, char * path)
{
	return MOTDAB.GetActBSR(iNumSeg,bsrname,path,&ihash);
}
