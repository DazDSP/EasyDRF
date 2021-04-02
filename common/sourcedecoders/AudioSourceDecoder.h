/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *
 * Description:
 *	See AudioSourceDecoder.cpp
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

#if !defined(AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_)
#define AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../Modul.h"
#include "../CRC.h"
#include "../TextMessage.h"
//#include "../resample/Resample.h"
#include "../datadecoding/DataDecoder.h"

/* Definitions ****************************************************************/
/* Forgetting factor for audio blocks in case CRC was wrong */
#define FORFACT_AUD_BL_BAD_CRC			((_REAL) 0.6)


/* Classes ********************************************************************/
class CAudioSourceEncoder : public CTransmitterModul<_SAMPLE, _BINARY>
{
public:
	CAudioSourceEncoder();
	virtual ~CAudioSourceEncoder();

	void SetTextMessage(const string& strText);
	void ClearTextMessage();

	void SetPicFileName(const string& strFileName, const string& strFileNamenoDir, CVector<short>  vecsToSend)
		{DataEncoder.GetSliShowEnc()->AddFileName(strFileName, strFileNamenoDir, vecsToSend);}
	void ClearPicFileNames()
		{DataEncoder.GetSliShowEnc()->ClearAllFileNames();}

	void SetTheStartDelay(int thestartdelay)
		{ DataEncoder.GetSliShowEnc()->SetMyStartDelay(thestartdelay);} //edited DM
	int GetPicCnt()
		{return DataEncoder.GetSliShowEnc()->GetPicCnt(); }
	int GetPicPerc()
		{return DataEncoder.GetSliShowEnc()->GetPicPerc(); }
	int GetNoOfPic()
		{return DataEncoder.GetSliShowEnc()->GetNoOfPic(); }
	int GetPicSegmAct() 
		{return DataEncoder.GetSliShowEnc()->GetPicSegmAct(); }
	int GetPicSegmTot() 
		{return DataEncoder.GetSliShowEnc()->GetPicSegmTot(); }					 

protected:
	CTextMessageEncoder TextMessage{};
	_BOOLEAN			bUsingTextMessage{};
	CDataEncoder		DataEncoder{};
	int					iTotPacketSize{};
	_BOOLEAN			bIsDataService{};

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class CAudioSourceDecoder : public CReceiverModul<_BINARY, _SAMPLE>
{
public:
	CAudioSourceDecoder();
	virtual ~CAudioSourceDecoder();

	int getdecodperc();

protected:
	enum EInitErr {ET_ALL, ET_AAC}; /* ET: Error type */
	class CInitErr 
	{
	public:
		CInitErr(EInitErr eNewErrType) : eErrType(eNewErrType) {}
		EInitErr eErrType{};
	};

	/* General */
	_BOOLEAN			DoNotProcessData{};
	_BOOLEAN			IsLPCAudio{};
	_BOOLEAN			IsSPEEXAudio{};
	_BOOLEAN			IsCELPAudio{};
	_BOOLEAN			IsSSTVData{};

	/* Text message */
	_BOOLEAN			bTextMessageUsed{};
	CTextMessageDecoder	TextMessage{};
	CVector<_BINARY>	vecbiTextMessBuf{};

	int					iTotalFrameSize{};

	_BOOLEAN			bAudioIsOK{};
	_BOOLEAN			bAudioWasOK{};


	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_)
