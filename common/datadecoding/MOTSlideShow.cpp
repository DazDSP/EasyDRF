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

void CMOTSlideShowEncoder::AddFileName(const string& strFileName,
									   const string& strFileNamenoDir,
									   CVector<short>  vecsToSend)
{
	/* Only ContentSubType "JFIF" (JPEG) and ContentSubType "PNG" are allowed
	   for SlideShow application (not tested here!) */
	/* Try to open file binary */

	int iOldNumObj;
	FILE * pFiBody = fopen(strFileName.c_str(), "rb");

	if (pFiBody != NULL)
	{
		//		_BYTE byIn; //this is the original file buffer - only 1 byte long DM
		//=============================================================================================================================
		const uLongf BUFSIZE = 524288; //512k should be enough for anything practical - HEAP STORAGE
		_BYTE* buffer1 = new _BYTE[BUFSIZE]; //File read buffer
		_BYTE* buffer2 = new _BYTE[BUFSIZE]; //zlib compression buffer
		uLongf filesize;
		unsigned int dd;
		int result;
		filesize = 0;

		// obtain file size : (there might be a more elegant way to do this...)
		fseek(pFiBody, 0, SEEK_END);
		filesize = ftell(pFiBody);
		if (filesize > BUFSIZE) filesize = BUFSIZE; //prevent buffer overrun - limit to 512k - Shouldn't this be BUFSIZE-1?
		rewind(pFiBody);

		//Daz Man:
		//if the input data is compressible (selected file types) then send data to buffer1
		//else send data to buffer2
		//add the text file zlib compressor in here - read the data from buffer1 and put compressed data into buffer2
		//keep the filename data intact - just append the extra .gz extension to strFileNamenoDir
		//change the existing routines below to read from the buffer output
		//
		LPSTR dx1 = const_cast<char*>(strFileNamenoDir.c_str()); //the filename that is being sent
		//LPSTR ds = strFileNamenoDir;
		const string& dx2 = ".gz"; //the 2nd filename extension that denotes zlib compression is used
		string strFileNamenoDirX; //the variable that holds the modified filename
		//test the filename extension
		if (!checkext(dx1)) {
			//read into buffer1 if it's text
			result = fread(buffer1, 1, filesize, pFiBody); //read the file to send into buffer1 if it is text
			//add zlib compression here
			uLongf ds = BUFSIZE; //tell zlib the buffer size we are using
			compress2(buffer2, &ds, buffer1, filesize, 9); //zlib compress text data and compressible files
			filesize = ds; //update filesize with the new compressed file data size
			strFileNamenoDirX = strFileNamenoDir.c_str() + dx2; //attach extra file extension to denote compression was used
		}
		else {
			//read into buffer2 directly for all else
			result = fread(buffer2, 1, filesize, pFiBody);
			strFileNamenoDirX = strFileNamenoDir.c_str(); //copy normal filename
		}

		//if (result != filesize) { fputs("Reading error", stderr); exit(3); } //does it need error checking?

		fclose(pFiBody); //close file
//=============================================================================================================================
//RS encoding can go here DM
//=============================================================================================================================
//		FILE* pFiBody = fopen(strFileName.c_str(), "rb"); //Open file - Now we read from buffer only DM

		if (vecMOTPicture.Size() == 0)
		{
			int i,k;
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
				vecMOTPicture[iOldNumObj].bIsLeader = (i == 0);
				for (k=0;k<the_startdelay;k++)
					vecMOTSegments[iOldNumObj][k] = k;
	
				dd = 0;
				while (dd < filesize)  //Read from buffer up to size - This is a read for the leadin - not all of it is used
//				while (fread((void*) &byIn, size_t(1), size_t(1), pFiBody) != size_t(0)) //edited DM
				{
					/* Add one byte = SIZEOF__BYTE bits */
					vecMOTPicture[iOldNumObj].vecbRawData.Enlarge(SIZEOF__BYTE);
//					vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t) byIn, SIZEOF__BYTE);
					vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)buffer2[dd], SIZEOF__BYTE); //from new buffer2
//					vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)whichbuffer[dd], SIZEOF__BYTE); //from buffer
					dd++; //next
				}
	
				/* Close the file afterwards */
//				fclose(pFiBody);
//				pFiBody = fopen(strFileName.c_str(), "rb");

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
		while (dd < filesize)  //Read from buffer up to size
//		while (fread((void*) &byIn, size_t(1), size_t(1), pFiBody) != size_t(0))
		{
			/* Add one byte = SIZEOF__BYTE bits */
			vecMOTPicture[iOldNumObj].vecbRawData.Enlarge(SIZEOF__BYTE);
//			vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t) byIn, SIZEOF__BYTE);
			vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)buffer2[dd], SIZEOF__BYTE); //from new buffer2 DM
//			vecMOTPicture[iOldNumObj].vecbRawData.Enqueue((uint32_t)whichbuffer[dd], SIZEOF__BYTE); //from new buffer
			dd++; //next
		}

		/* Close the file afterwards */
//		fclose(pFiBody);

		//remove the buffer arrays from the heap DM
		delete[] buffer1;
		delete[] buffer2;
	}
}

void CMOTSlideShowEncoder::SetMyStartDelay(int delay)
{
	the_startdelay = delay;
}
int CMOTSlideShowEncoder::GetPicPerc(void)
{
	int segct,totseg;
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
	return MOTDAB.GetActMOTObject(NewPic);
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
