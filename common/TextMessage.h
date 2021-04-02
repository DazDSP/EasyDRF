/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See TextMessage.cpp
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

#if !defined(TEXT_MESSAGE_H__3B00_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define TEXT_MESSAGE_H__3B00_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "GlobalDefinitions.h"
#include "CRC.h"
#include "Vector.h"


/* Definitions ****************************************************************/
/* The text message may comprise up to 8 segments ... The body shall contain
   16 bytes of character data ... 
   We add "+ 1" since the entry "0" is not used */
#define MAX_NUM_SEG_TEXT_MESSAGE		(8 + 1)
#define BYTES_PER_SEG_TEXT_MESS			16

#define TOT_NUM_BITS_PER_PIECE			((BYTES_PER_SEG_TEXT_MESS /* Max body */ \
										+ 2 /* Header */ + 2 /* CRC */) * SIZEOF__BYTE)


/* Classes ********************************************************************/
/* Encoder ------------------------------------------------------------------ */
class CTextMessage
{
public:
	CTextMessage() : iNumSeg(0) {}
	virtual ~CTextMessage() {}

	CVector<_BINARY>&	operator[](const int iI) {return vvbiSegment[iI];}

	void				SetText(const string& strMessage, const _BINARY biToggleBit);
	inline int			GetNumSeg() const {return iNumSeg;}
	inline int			GetSegSize(const int iSegID) const
							{return vvbiSegment[iSegID].Size() / SIZEOF__BYTE;}

protected:
	CVector<CVector<_BINARY> >	vvbiSegment;
	int							iNumSeg;
};

class CTextMessageEncoder
{
public:
	CTextMessageEncoder() : iNumMess(0), vecstrText(0) {}
	virtual ~CTextMessageEncoder() {}

	void Encode(CVector<_BINARY>& pData);
	void SetMessage(const string& strMessage);
	void ClearAllText();

protected:
	CTextMessage	CurTextMessage{}; //init DM
	CVector<string>	vecstrText{}; //init DM
	int				iSegCnt{}; //init DM
	int				iByteCnt{}; //init DM
	int				iNumMess{}; //init DM
	int				iMessCnt{}; //init DM
	_BINARY			biToggleBit{}; //init DM
};


/* Decoder ------------------------------------------------------------------ */
class CTextMessSegment
{
public:
	CTextMessSegment() : byData(BYTES_PER_SEG_TEXT_MESS), bIsOK(FALSE),
		iNumBytes(0) {}

	CVector<_BYTE>	byData{}; //init DM
	_BOOLEAN		bIsOK{}; //init DM
	int				iNumBytes{}; //init DM
};

class CTextMessageDecoder
{
public:
	CTextMessageDecoder() {}
	virtual ~CTextMessageDecoder() {}

	void Decode(CVector<_BINARY>& pData);
	void Init(string* pstrNewPText);

protected:
	void ReadHeader();
	void ClearText();
	void SetText();
	void ResetSegments();

	CVector<_BINARY>	biStreamBuffer{}; //init DM

	string*				pstrText{}; //init DM

	_BINARY				biCommandFlag{}; //init DM

	_BINARY				biFirstFlag{}; //init DM
	_BINARY				biLastFlag{}; //init DM
	_BYTE				byCommand{}; //init DM
	_BYTE				bySegmentID{}; //init DM
	_BINARY				biToggleBit{}; //init DM
	_BYTE				byLengthBody{}; //init DM
	int					iBitCount{}; //init DM
	int					iNumSegments{}; //init DM

	_BINARY				biOldToggleBit{}; //init DM

	CTextMessSegment	Segment[MAX_NUM_SEG_TEXT_MESSAGE];

	CCRC				CRCObject{}; //init DM
};


#endif // !defined(TEXT_MESSAGE_H__3B00_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
