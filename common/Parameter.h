/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See Parameter.cpp
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

#if !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "GlobalDefinitions.h"
#include "ofdmcellmapping/CellMappingTable.h"
#include "matlib/Matlib.h"


#define DEFiInputBlockSize 38400 //define buffer size of transmit audio input buffer for 400mS @ 48000kHz stereo


/* Classes ********************************************************************/
class CParameter : public CCellMappingTable
{
public:
	CParameter() : bRunThread(FALSE), Stream(MAX_NUM_STREAMS), iChanEstDelay(0),
		bUsingMultimedia(TRUE) {}
	virtual ~CParameter() {}

	/* Enumerations --------------------------------------------------------- */
	/* SF: Service Flag */
	enum ETyOServ {SF_AUDIO, SF_DATA};

	/* PM: Packet Mode */
	enum EPackMod {PM_SYNCHRON_STR_MODE, PM_PACKET_MODE};

	/* DU: Data Unit */
	enum EDatUnit {DU_SINGLE_PACKETS, DU_DATA_UNITS};

	/* AD: Application Domain */
	enum EApplDomain {AD_DRM_SPEC_APP, AD_DAB_SPEC_APP, AD_OTHER_SPEC_APP};

	/* AC: Audio Coding */
	enum EAudCod {AC_LPC, AC_CELP, AC_SPEEX, AC_SSTV};

	/* CS: Coding Scheme */
	enum ECodScheme {CS_1_SM, CS_2_SM, CS_3_SM};

	/* SI: Symbol Interleaver */
	enum ESymIntMod {SI_LONG, SI_SHORT};

	/* CT: Channel Type */
	enum EChanType {CT_MSC, CT_FAC};

	/* ST: Simulation Type */
	enum ESimType {ST_NONE, ST_BITERROR, ST_MSECHANEST, ST_BER_IDEALCHAN};


	/* Classes -------------------------------------------------------------- */
	class CAudioParam
	{
	public:
		CAudioParam() : strTextMessage("") {}

		/* Text-message */
		string		strTextMessage; /* Max length is (8 * 16 Bytes) */	

		int			iStreamID; /* Stream Id of the stream which carries the audio service */

		EAudCod		eAudioCoding; /* This field indicated the source coding system */
		_BOOLEAN	bTextflag; /* Indicates whether a text message is present or not */


/* TODO: Copy operator. Now, default copy operator is used! */

		/* This function is needed for detection changes in the class */
		_BOOLEAN operator!=(const CAudioParam AudioParam)
		{
			if (iStreamID != AudioParam.iStreamID) return TRUE;
			if (eAudioCoding != AudioParam.eAudioCoding) return TRUE;
			if (bTextflag != AudioParam.bTextflag) return TRUE;

			return FALSE;
		}
	};

	class CDataParam
	{
	public:
		int			iStreamID; /* Stream Id of the stream which carries the data service */

		EPackMod	ePacketModInd; /* Packet mode indicator */


		/* In case of packet mode ------------------------------------------- */
		EDatUnit	eDataUnitInd; /* Data unit indicator */
		int			iPacketID; /* Packet Id (2 bits) */
		int			iPacketLen; /* Packet length */

		// "DAB specified application" not yet implemented!!!
		EApplDomain eAppDomain; /* Application domain */
		int			iUserAppIdent; /* User application identifier, only DAB */

/* TODO: Copy operator. Now, default copy operator is used! */

		/* This function is needed for detection changes in the class */
		_BOOLEAN operator!=(const CDataParam DataParam)
		{
			if (iStreamID != DataParam.iStreamID) return TRUE;
			if (ePacketModInd != DataParam.ePacketModInd) return TRUE;
			if (DataParam.ePacketModInd == PM_PACKET_MODE)
			{
				if (eDataUnitInd != DataParam.eDataUnitInd) return TRUE;
				if (iPacketID != DataParam.iPacketID) return TRUE;
				if (iPacketLen != DataParam.iPacketLen) return TRUE;
				if (eAppDomain != DataParam.eAppDomain) return TRUE;
				if (DataParam.eAppDomain == AD_DAB_SPEC_APP)
					if (iUserAppIdent != DataParam.iUserAppIdent) return TRUE;
			}
			return FALSE;
		}
	};

	class CService
	{
	public:
		CService() : strLabel("") {}

		_BOOLEAN IsActive() {return iServiceID != SERV_ID_NOT_USED;}

		_UINT32BIT	iServiceID;
		int			iLanguage;
		ETyOServ	eAudDataFlag;
		int			iServiceDescr;

		/* Label of the service */
		string		strLabel;

		/* Audio parameters */
		CAudioParam	AudioParam;

		/* Data parameters */
		CDataParam	DataParam;
	};

	class CStream
	{
	public:
		CStream() : iLenPartB(0) {}

		int	iLenPartB; /* Data length for part B */

		_BOOLEAN operator!=(const CStream Stream)
		{
			if (iLenPartB != Stream.iLenPartB) return TRUE;
			return FALSE;
		}
	};

	class CMSCProtLev
	{
	public:
		int	iPartB; /* MSC protection level for part B */

		CMSCProtLev& operator=(const CMSCProtLev& NewMSCProtLev)
		{
			iPartB = NewMSCProtLev.iPartB;
			return *this; 
		}
	};

	void			ResetServicesStreams();
	void			GetActiveServices(CVector<int>& veciActServ);
	void			GetActiveStreams(CVector<int>& veciActStr);
	int				GetNumActiveServices();
	void			InitCellMapTable(const ERobMode eNewWaveMode, const ESpecOcc eNewSpecOcc);

	void			SetNumDecodedBitsMSC(const int iNewNumDecodedBitsMSC);
	void			SetNumBitsHieraFrTot(const int iNewNumBitsHieraFrTot);
	void			SetNumAudioDecoderBits(const int iNewNumAudioDecoderBits);
	void			SetNumDataDecoderBits(const int iNewNumDataDecoderBits);

	_BOOLEAN		SetWaveMode(const ERobMode eNewWaveMode);
	ERobMode		GetWaveMode() const {return eRobustnessMode;}

	void			SetCurSelAudioService(const int iNewService);
	int				GetCurSelAudioService() const {return iCurSelAudioService;}
	void			SetCurSelDataService(const int iNewService);
	int				GetCurSelDataService() const {return iCurSelDataService;}
	void			EnableMultimedia(const _BOOLEAN bFlag);
	_BOOLEAN		GetEnableMultimedia() const {return bUsingMultimedia;}

	_REAL			GetDCFrequency() const {return SOUNDCRD_SAMPLE_RATE * (rFreqOffsetAcqui + rFreqOffsetTrack);}
	_REAL			GetSampFreqEst() const {return rResampleOffset;}

	_REAL			GetBitRate(int iServiceID);
	_BOOLEAN		IsEEP(int iServiceID); /* Is service equal error protection */


	/* Parameters controlled by FAC ----------------------------------------- */
	void			SetInterleaverDepth(const ESymIntMod eNewDepth);
	ESymIntMod		GetInterleaverDepth() {return eSymbolInterlMode;}

	void			SetMSCCodingScheme(const ECodScheme eNewScheme);

	void			SetSpectrumOccup(ESpecOcc eNewSpecOcc);
	ESpecOcc		GetSpectrumOccup() const {return eSpectOccup;}

	void			SetNumOfServices(const int iNNumAuSe, const int iNNumDaSe);
	int				GetTotNumServices() {return iNumAudioService + iNumDataService;}

	void			SetAudDataFlag(const int iServID, const ETyOServ iNewADaFl);
	void			SetServID(const int iServID, const _UINT32BIT iNewServID);


	/* Symbol interleaver mode (long or short interleaving) */
	ESymIntMod		eSymbolInterlMode; 

	ECodScheme		eMSCCodingScheme; /* MSC coding scheme */


	int				iNumAudioService;
	int				iNumDataService;


	void SetAudioParam(const int iShortID, const CAudioParam NewAudParam);
	CAudioParam GetAudioParam(const int iShortID)
		{return Service[iShortID].AudioParam;}
	void SetDataParam(const int iShortID, const CDataParam NewDataParam);
	CDataParam GetDataParam(const int iShortID)
		{return Service[iShortID].DataParam;}

	void SetMSCProtLev(const int NewMSCPrLe);
	void SetStreamLen(const int iStreamID, const int iNewLenPartB);

	/* Protection levels for MSC */
	CMSCProtLev			MSCPrLe;

	CVector<CStream>	Stream;
	CService			Service[MAX_NUM_SERVICES];

	/* These values are used to set input and output block sizes of some
	   modules */
	int					iNumBitsHierarchFrameTotal;
	int					iNumDecodedBitsMSC;
	int					iNumAudioDecoderBits; /* Number of input bits for audio module */
	int					iNumDataDecoderBits; /* Number of input bits for data decoder module */

	/* Identifies the current frame. This parameter is set by FAC */
	int					iFrameIDTransm;
	int					iFrameIDReceiv;


	/* Synchronization ------------------------------------------------------ */
	_REAL				rFreqOffsetAcqui;
	_REAL				rFreqOffsetTrack;

	_REAL				rResampleOffset;

	int					iTimingOffsTrack;

	_BOOLEAN			bUseFilter;
	_BOOLEAN			bOnlyPicture;

	/* General -------------------------------------------------------------- */

	int					iChanEstDelay;

	int					iNumTaps;
	_REAL				rGainCorr;
	int					iOffUsfExtr;

	_BOOLEAN			bRunThread;
	_BOOLEAN			bUsingMultimedia;

protected:
	/* Current selected audio service for processing */
	int					iCurSelAudioService;
	int					iCurSelDataService;

	ERobMode			eRobustnessMode; /* E.g.: Mode A, B, C or D */
	ESpecOcc			eSpectOccup;
};


#endif // !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
