/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *
 * Description:
 *	See DABMOT.cpp
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

#if !defined(MOTSLIDESHOW_H__3B0UBVE98732KJVEW363LIHGEW982__INCLUDED_)
#define MOTSLIDESHOW_H__3B0UBVE98732KJVEW363LIHGEW982__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Vector.h"
#include "../CRC.h"
#include "DABMOT.h"

extern int SetSegSize;

/* Classes ********************************************************************/
/* Encoder ------------------------------------------------------------------ */
class CMOTSlideShowEncoder
{
public:
	CMOTSlideShowEncoder() : vecMOTPicture(0), vecMOTSegments(0) { SetMyStartDelay(12); Init(116); } //
	virtual ~CMOTSlideShowEncoder() {}

	void Init(int iSegSize);

	void GetDataUnit(CVector<_BINARY>& vecbiNewData);

	void AddFileName(const string& strFileName, 
					 const string& strFileNamenoDir,
					 CVector<short>  vecsToSend);
	void ClearAllFileNames() {vecMOTPicture.Init(0); vecMOTSegments.Init(0); }

	void SetMyStartDelay(int delay);

	int GetNoOfPic(void) { return vecMOTPicture.Size(); };
	int GetPicCnt(void) { return MOTDAB.GetPicCount(); };
	int GetPicPerc(void);
	int GetPicSegmAct(void) { return MOTDAB.GetPicSegmAct(); };
	int GetPicSegmTot(void) { return MOTDAB.GetPicSegmTot(); };

protected:
	void AddNextPicture();

	CMOTDABEnc			MOTDAB;

	CVector<CMOTObject>	vecMOTPicture;
	CVector<CVector<short> >  vecMOTSegments;
	int					iPictureCnt;
	int					the_startdelay;
	int					iSegmentSize;
};


/* Decoder ------------------------------------------------------------------ */
class CMOTSlideShowDecoder
{
public:
	CMOTSlideShowDecoder() : bNewPicture(FALSE) {}
	virtual ~CMOTSlideShowDecoder() {}

	void AddDataUnit(CVector<_BINARY>& vecbiNewData);
	_BOOLEAN GetPicture(CMOTObject& NewPic);
	_BOOLEAN GetPartPicture(CMOTObject& NewPic);
	_BOOLEAN GetActSegments(CVector<_BINARY>& NewSeg);
	_BOOLEAN GetPartBSR(int * iNumSeg, string * bsrname, char * path);

	int GetTotSize(void) { return MOTDAB.GetObjectTotSize(); }; //Original code - this fails after the first file.. Why? DM It's related to the way the incoming raw data array is sized...
	int GetActSize(void) { return MOTDAB.GetObjectActSize(); };
	int GetActPos(void)  { return MOTDAB.GetObjectActPos(); };


protected:
	_BOOLEAN	bNewPicture;
	CMOTObject	MOTPicture;
	CMOTDABDec	MOTDAB;
};


#endif // !defined(MOTSLIDESHOW_H__3B0UBVE98732KJVEW363LIHGEW982__INCLUDED_)
