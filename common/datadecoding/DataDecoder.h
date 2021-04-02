/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DataDecoder.cpp
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

#if !defined(DATADECODER_H__3B0BA660_CA3452363E7A0D31912__INCLUDED_)
#define DATADECODER_H__3B0BA660_CA3452363E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../Modul.h"
#include "../CRC.h"
#include "../Vector.h"
#include "MOTSlideShow.h"


/* Definitions ****************************************************************/
/* Maximum number of packets per stream */
#define MAX_NUM_PACK_PER_STREAM					4


/* Classes ********************************************************************/
/* Encoder ------------------------------------------------------------------ */
class CDataEncoder
{
public:
	CDataEncoder() {}
	virtual ~CDataEncoder() {}

	int Init(CParameter& Param);
	void GeneratePacket(CVector<_BINARY>& vecbiPacket);

	CMOTSlideShowEncoder* GetSliShowEnc() {return &MOTSlideShowEncoder;}

protected:
	CMOTSlideShowEncoder	MOTSlideShowEncoder;
	CVector<_BINARY>		vecbiCurDataUnit;

	int						iPacketLen{}; //init DM
	int						iTotalPacketSize{}; //init DM
	int						iCurDataPointer{}; //init DM
	int						iPacketID{}; //init DM
	int						iContinInd{}; //init DM
};


/* Decoder ------------------------------------------------------------------ */
class CDataDecoder : public CReceiverModul<_BINARY, _BINARY>
{
public:
	CDataDecoder() : iServPacketID(0), DoNotProcessData(TRUE),
		eAppType(AT_NOT_SUP) {}
	virtual ~CDataDecoder() {}
	enum EAppType {AT_NOT_SUP, AT_MOTSLISHOW, AT_JOURNALINE};

	_BOOLEAN GetSlideShowPicture(CMOTObject& NewPic);
	_BOOLEAN GetSlideShowPartPicture(CMOTObject& NewPic);
	_BOOLEAN GetSlideShowPartActSegs(CVector<_BINARY>& vbSegs);
	_BOOLEAN GetSlideShowBSR(int * iNumSeg, string * bsrname, char * path);
	EAppType GetAppType() {return eAppType;}
	int GetTotSize(void);
	int GetActSize(void);
	int GetActPos(void);

protected:
	class CDataUnit
	{
	public:
		CVector<_BINARY>	vecbiData{}; //init DM
		_BOOLEAN			bOK{}; //init DM
		_BOOLEAN			bReady{}; //init DM

		void Reset()
		{
			vecbiData.Init(0);
			bOK = FALSE;
			bReady = FALSE;
		}
	};

	int						iTotalPacketSize{}; //init DM
	int						iNumDataPackets{}; //init DM
	int						iMaxPacketDataSize{}; //init DM
	int						iServPacketID{}; //init DM
	CVector<int>			veciCRCOk{}; //init DM

	_BOOLEAN				DoNotProcessData{}; //init DM

	int						iContInd[MAX_NUM_PACK_PER_STREAM]{}; //init DM
	CDataUnit				DataUnit[MAX_NUM_PACK_PER_STREAM]{}; //init DM
	CMOTSlideShowDecoder	MOTSlideShow[MAX_NUM_PACK_PER_STREAM]{}; //init DM

	EAppType				eAppType{}; //init DM

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(DATADECODER_H__3B0BA660_CA3452363E7A0D31912__INCLUDED_)
