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

#if !defined(DABMOT_H__3B0UBVE98732KJVEW363E7A0D31912__INCLUDED_)
#define DABMOT_H__3B0UBVE98732KJVEW363E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Vector.h"
#include "../CRC.h"


/* Classes ********************************************************************/
class CMOTObjectRaw
{
public:
	class CDataUnit
	{
	public:
		CDataUnit() {Reset();}

		void Reset();
		void Add(CVector<_BINARY>& vecbiNewData,
			const int iSegmentSize, const int iSegNum);

		CVector<_BINARY>	vecbiData;
		_BOOLEAN			bOK, bReady;
		int					iDataSegNum;
	};

	class CDataUnitRx
	{
	public:
		CDataUnitRx() {Reset();}

		void Reset();
		void Add(CVector<_BINARY>& vecbiNewData,
			const int iSegmentSize, const int iSegNum);

		CVector<CVector<_BINARY> > vvbiSegment;
		_BOOLEAN			bOK, bReady{}; //init DM
		int					iDataSegNum{}; //init DM
		int					iTotSegments{}; //init DM
	};

	int			iTransportID{}; //init DM
	int			iSegmentSize{}; //init DM
	int			iActSegment{}; //init DM
	CDataUnit	Header;
	CDataUnit	Body;
	CDataUnitRx	BodyRx;
};

class CMOTObject
{
public:
	CMOTObject() {Reset();}
	CMOTObject(const CMOTObject& NewObj) : vecbRawData(NewObj.vecbRawData),
		strName(NewObj.strName) {}

	inline CMOTObject& operator=(const CMOTObject& NewObj)
	{
		strName = NewObj.strName;
		strNameandDir = NewObj.strNameandDir;
		vecbRawData.Init(NewObj.vecbRawData.Size());
		vecbRawData = NewObj.vecbRawData;
		return *this;
	}

	void Reset()
	{
		vecbRawData.Init(0);
		strName = "";
		strNameandDir = "";
		bIsLeader = FALSE;
	}

	CVector<_BYTE>	vecbRawData{}; //init DM
	string			strName{}; //init DM
	string			strNameandDir{}; //init DM
	_BOOLEAN		bIsLeader{}; //init DM
	int				iTransportID{}; //init DM
};


/* Encoder ------------------------------------------------------------------ */
class CMOTDABEnc
{
public:
	CMOTDABEnc() {}
	virtual ~CMOTDABEnc() {}

	void Reset(int iSegLen);
	_BOOLEAN GetDataGroup(CVector<_BINARY>& vecbiNewData);
	void SetMOTObject(CMOTObject& NewMOTObject,CVector<short> vecsDataIn);
	int GetPicCount(void);
	int GetPicSegmAct(void) { return iSegmCnt; };
	int GetPicSegmTot(void) { return iTotSegm; };
protected:
	class CMOTObjSegm
	{
	public:
		CVector<CVector<_BINARY> > vvbiHeader;
		CVector<CVector<_BINARY> > vvbiBody;
		CVector<_BINARY>		   vecbiToSend;
	};

	void GenMOTSegments(CMOTObjSegm& MOTObjSegm);
	void PartitionUnits(CVector<_BINARY>& vecbiSource,
						CVector<CVector<_BINARY> >& vecbiDest,
						const int iPartiSize,
						int ishead);

	void GenMOTObj(CVector<_BINARY>& vecbiData, CVector<_BINARY>& vecbiSeg,
				   const _BOOLEAN bHeader, const int iSegNum,
				   const int iTranspID, const _BOOLEAN bLastSeg);

	CMOTObject		MOTObject;
	CMOTObjSegm		MOTObjSegments;

	int				iSegmCnt{}; //init DM
	int				iTotSegm{}; //init DM
	int				iTxPictCnt{}; //init DM
	_BOOLEAN		bCurSegHeader{}; //init DM

	int				iContIndexHeader{}; //init DM
	int				iContIndexBody{}; //init DM

	int				iTransportID{}; //init DM
	int				iSegmentSize{}; //init DM
};


/* Decoder ------------------------------------------------------------------ */
class CMOTDABDec
{
public:
	CMOTDABDec() {}
	virtual ~CMOTDABDec() {}

	_BOOLEAN	AddDataGroup(CVector<_BINARY>& vecbiNewData);
	_BOOLEAN	GetActMOTSegs(CVector<_BINARY>& vSegs);
	_BOOLEAN	GetActMOTObject(CMOTObject& NewMOTObject);
	_BOOLEAN	GetActBSR(int * iNumSeg, string * bsr_name, char * path, int * iHash);
	void		GetMOTObject(CMOTObject& NewMOTObject)
					{NewMOTObject = MOTObject; /* Simply copy object */}
	int GetObjectTotSize() { return MOTObjectRaw.BodyRx.vvbiSegment.Size(); }
	int GetObjectActSize() 
	{ 
		if (MOTObjectRaw.BodyRx.iDataSegNum >= 0) 
			return MOTObjectRaw.BodyRx.iDataSegNum; 
		else  return 0;
	}
	int GetObjectActPos()  { return MOTObjectRaw.iActSegment; }

protected:
	void DecodeObject(CMOTObjectRaw& MOTObjectRaw);

	CMutex			Mutex;

	CMOTObject		MOTObject;
	CMOTObjectRaw	MOTObjectRaw;
};


#endif // !defined(DABMOT_H__3B0UBVE98732KJVEW363E7A0D31912__INCLUDED_)
