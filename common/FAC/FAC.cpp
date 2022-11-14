/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *  Daz Man 2022
 * 
 * Description:
 *	FAC
 *
 * Modified Nov 14, 2022 to allow QAM4 mode to work in Digital Voice mode
 * by using the unused dummy bit 32.
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

#include "FAC.h"


/* Implementation *************************************************************/

int packlen =  40;
int streamlen =  0;

/******************************************************************************\
* CFACTransmit																   *
\******************************************************************************/

void CFACTransmit::FACParam(CVector<_BINARY>* pbiFACData, CParameter& Parameter)
{
	static int	idatalen;
	bool isaudio = false; //added DM

	idatalen = Parameter.iNumDecodedBitsMSC / SIZEOF__BYTE;

	/* Reset enqueue function */
	(*pbiFACData).ResetBitAccess();


	/* Put FAC parameters on stream */
	/* Channel parameters --------------------------------------------------- */


	/* Identity  2 bit */
	/* Manage index of FAC block in super-frame */
	switch (Parameter.iFrameIDTransm)
	{
	case 0:
		/* Assuming AFS is valid (AFS not used here), if AFS is not valid, the
		   parameter must be 3 (11) */
		(*pbiFACData).Enqueue(3 /* 11 */, 2);
		break;

	case 1:
		(*pbiFACData).Enqueue(1 /* 01 */, 2);
		break;

	case 2:
		(*pbiFACData).Enqueue(2 /* 10 */, 2);
		break;
	}

	/* Spectrum occupancy 1 bit */
	switch (Parameter.GetSpectrumOccup())
	{
	case SO_0:
		(*pbiFACData).Enqueue(0 /* 0 */, 1);
		break;

	case SO_1:
		(*pbiFACData).Enqueue(1 /* 1 */, 1);
		break;
	}

	/* Interleaver depth flag  1 bit*/
	switch (Parameter.eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		(*pbiFACData).Enqueue(0 /* 0 */, 1);
		break;

	case CParameter::SI_SHORT:
		(*pbiFACData).Enqueue(1 /* 1 */, 1);
		break;
	}

	/* MSC mode 1 bit */
	switch (Parameter.eMSCCodingScheme)
	{
	case CParameter::CS_3_SM:
		(*pbiFACData).Enqueue(0 /* 0 */, 1);
		break;
	case CParameter::CS_2_SM:
		(*pbiFACData).Enqueue(1 /* 1 */, 1);
		break;
	case CParameter::CS_1_SM:		// treat as 16 qam
		(*pbiFACData).Enqueue(1 /* 1 */, 1);
		break;
	}

	/* Prot Level B 1 bit */
	switch (Parameter.MSCPrLe.iPartB)
	{
	case 0:
		(*pbiFACData).Enqueue(0 /* 0 */, 1);
		break;

	case 1:
		(*pbiFACData).Enqueue(1 /* 1 */, 1);
		break;
	}

	/* Audio/Data flag 1 bit */
	switch (Parameter.Service[0].eAudDataFlag)
	{
	case CParameter::SF_AUDIO:
		isaudio = true;
		(*pbiFACData).Enqueue(0 /* 0 */, 1);
		/* Audio coding 2 bit */
		switch (Parameter.Service[0].AudioParam.eAudioCoding)
		{
		case CParameter::AC_LPC:
			(*pbiFACData).Enqueue(0 , 2);
			break;
		case CParameter::AC_CELP:
			(*pbiFACData).Enqueue(1 , 2);
			break;
		case CParameter::AC_SPEEX:
			(*pbiFACData).Enqueue(2 , 2);
			break;
		case CParameter::AC_SSTV:
			(*pbiFACData).Enqueue(3 , 2);
			break;
		}
		/* Text flag 1 bit */
		switch (Parameter.Service[0].AudioParam.bTextflag)
		{
		case FALSE:
			(*pbiFACData).Enqueue(0 /* 0 */, 1);
			break;

		case TRUE:
			(*pbiFACData).Enqueue(1 /* 1 */, 1);
			break;
		}
		break;

	case CParameter::SF_DATA:
		(*pbiFACData).Enqueue(1 /* 1 */, 1);
		/* Packet Id */
		(*pbiFACData).Enqueue( 
			(_UINT32BIT) Parameter.Service[0].DataParam.iPacketID, 2);
		/* Extended MSC mode 1 bit */
		if (Parameter.eMSCCodingScheme == CParameter::CS_1_SM)
			(*pbiFACData).Enqueue(1 /* 1 */, 1);  // QAM 4
		else
			(*pbiFACData).Enqueue(0 /* 0 */, 1);  // others
		break;
	}
	

	/* Label 21 bit */
	{
		int	iLenLabel = 0; //init DM
		int iframet = Parameter.iFrameIDTransm;
		const int iLenLabelTmp = Parameter.Service[0].strLabel.length();
		if (iLenLabelTmp > 9)
			iLenLabel = 9;
		else
			iLenLabel = iLenLabelTmp;
		/* Set all characters of label string */
		for (int i = 3*iframet; i < 3*iframet+3; i++)
		{
			char cNewChar;
			if (i >= iLenLabel)
				cNewChar = 0;
			else
				cNewChar = Parameter.Service[0].strLabel[i];
			cNewChar &= 127;
			/* Set character */
			(*pbiFACData).Enqueue((_UINT32BIT) cNewChar, 7);
		}
	}

	// total 31 bit

	//This bit is normally a dummy bit
	if ((isaudio) && (Parameter.eMSCCodingScheme == CParameter::CS_1_SM)) {
	//Added DM TEST for signalling Voice mode is QAM4 =================================
	if (Parameter.eMSCCodingScheme == CParameter::CS_1_SM)
		(*pbiFACData).Enqueue(1, 1);  // QAM 4
	else
		(*pbiFACData).Enqueue(0, 1);  // others
	//Added DM TEST for signalling Voice mode is QAM4 =================================
	}
	// total 32 bits

	/* CRC ------------------------------------------------------------------ */
	/* Calculate the CRC and put at the end of the stream */
	CRCObject.Reset(8);

	(*pbiFACData).ResetBitAccess();

	for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE - 1; i++)
		CRCObject.AddByte((_BYTE) (*pbiFACData).Separate(SIZEOF__BYTE));

	/* Now, pointer in "enqueue"-function is back at the same place, 
	   add CRC */
	(*pbiFACData).Enqueue(CRCObject.GetCRC(), 8);
}


void CFACTransmit::Init(CParameter& Parameter)
{
	int				i = 0; //init DM
	CVector<int>	veciActServ;

	/* Get active services */
	Parameter.GetActiveServices(veciActServ);
	const int iTotNumServices = veciActServ.Size();

	/* Check how many audio and data services present */
	CVector<int>	veciAudioServ(0);
	CVector<int>	veciDataServ(0);
	int				iNumAudio = 0;
	int				iNumData = 0;

	for (i = 0; i < iTotNumServices; i++)
	{
		if (Parameter.Service[veciActServ[i]].
			eAudDataFlag ==	CParameter::SF_AUDIO)
		{
			veciAudioServ.Enlarge(1);
			veciAudioServ[iNumAudio] = i;
			iNumAudio++;
		}
		else
		{
			veciDataServ.Enlarge(1);
			veciDataServ[iNumData] = i;
			iNumData++;
		}
	}


	/* Now check special cases which are defined in 6.3.6-------------------- */
	/* If we have only data or only audio services. When all services are of
	   the same type (e.g. all audio or all data) then the services shall be
	   signalled sequentially */
	if ((iNumAudio == iTotNumServices) || (iNumData == iTotNumServices))
	{
		/* Init repetion vector */
		FACNumRep = iTotNumServices;
		FACRepetition.Init(FACNumRep);

		for (i = 0; i < FACNumRep; i++)
			FACRepetition[i] = veciActServ[i];
	}
}


/******************************************************************************\
* CFACReceive																   *
\******************************************************************************/

string strlabel[3];
int ilabelstate = 0;

_BOOLEAN CFACReceive::FACParam(CVector<_BINARY>* pbiFACData,
	CParameter& Parameter)
{
	/*
		First get new data from incoming data stream, then check if the new
		parameter differs from the old data stored in the receiver. If yes, init
		the modules to the new parameter
	*/
	CParameter::CAudioParam	AudParam{}; //init DM
	CParameter::CDataParam	DataParam{}; //init DM

	/* CRC ------------------------------------------------------------------ */
	/* Check the CRC of this data block */
	CRCObject.Reset(8);

	(*pbiFACData).ResetBitAccess();

	for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE - 1; i++)
		CRCObject.AddByte((_BYTE)(*pbiFACData).Separate(SIZEOF__BYTE));

	if (CRCObject.CheckCRC((*pbiFACData).Separate(8)) == TRUE)
	{
		bool isaudio = false; //added DM
		bool ispec = false;
		bool imscprot = false;
		bool imscqam = false;
		bool imsclowqam = false;
		int	MSCPrLe = 0; //init DM

		/* CRC-check successful, extract data from FAC-stream */
		/* Reset separation function */
		(*pbiFACData).ResetBitAccess();


		/* Channel parameters ----------------------------------------------- */

		Parameter.SetServID(0, 73);
		Parameter.Service[0].iLanguage = 7;
		Parameter.Service[0].iServiceDescr = 3;
		/* Set new parameters in global struct */
		MSCPrLe = 0;

		/* Identity */
		switch ((*pbiFACData).Separate(2))
		{
		case 0: /* 00 */
			Parameter.iFrameIDReceiv = 0;
			break;

		case 1: /* 01 */
			Parameter.iFrameIDReceiv = 1;
			break;

		case 2: /* 10 */
			Parameter.iFrameIDReceiv = 2;
			break;

		case 3: /* 11 */
			Parameter.iFrameIDReceiv = 0;
			break;
		}

		/* Spectrum occupancy */
		switch ((*pbiFACData).Separate(1))
		{
		case 0: /* 0 */
			Parameter.SetSpectrumOccup(SO_0);
			ispec = false;
			break;

		case 1: /* 1 */
			Parameter.SetSpectrumOccup(SO_1);
			ispec = true;
			break;

		}

		/* Interleaver depth flag */
		switch ((*pbiFACData).Separate(1))
		{
		case 0: /* 0 */
			Parameter.SetInterleaverDepth(CParameter::SI_LONG);
			break;

		case 1: /* 1 */
			Parameter.SetInterleaverDepth(CParameter::SI_SHORT);
			break;
		}

		/* MSC mode */
		switch ((*pbiFACData).Separate(1))
		{
		case 0: /* 0 */
			imscqam = true;
			break;

		case 1: /* 1 */
			imscqam = false;
			break;
		}

		/* Prot Level B */
		switch ((*pbiFACData).Separate(1))
		{
		case 0: /* 0 */
			MSCPrLe = 0;
			imscprot = false;
			break;

		case 1: /* 1 */
			MSCPrLe = 1;
			imscprot = true;
			break;
		}
		Parameter.SetMSCProtLev(MSCPrLe);

		if (Parameter.GetWaveMode() == RM_ROBUSTNESS_MODE_B)
		{
			if (ispec)	// 2.5 khz B
			{
				if (imscprot)		// prot level 1
				{
					if (imscqam) {			// QAM 64
						streamlen = 235;
						packlen = 232;
					}
					else {					// QAM 16
						streamlen = 163;
						packlen = 160;
					}
				}
				else				// prot level 0
				{
					if (imscqam) {			// QAM 64
						streamlen = 196;
						packlen = 193;
					}
					else {					// QAM 16
						streamlen = 130;
						packlen = 127;
					} //127
				}
			}
			else		// 2.3 khz B
			{
				if (imscprot) 		// prot level 1
				{
					if (imscqam) {			// QAM 64
						streamlen = 201;
						packlen = 198;
					}
					else {					// QAM 16
						streamlen = 140;
						packlen = 137;
					}
				}
				else 				// prot level 0
				{
					if (imscqam) {			// QAM 64
						streamlen = 168;
						packlen = 165;
					}
					else {					// QAM 16
						streamlen = 112;
						packlen = 109;
					}
				}
			}
		}
		else if (Parameter.GetWaveMode() == RM_ROBUSTNESS_MODE_A)

		{
			if (ispec)	// 2.5 khz A
			{
				if (imscprot)		// prot level 1
				{
					if (imscqam) {			// QAM 64
						streamlen = 313;
						packlen = 310;
					}
					else {					// QAM 16
						streamlen = 218;
						packlen = 215;
					}
				}
				else				// prot level 0
				{
					if (imscqam) {			// QAM 64
						streamlen = 261;
						packlen = 258;
					}
					else {					// QAM 16
						streamlen = 174;
						packlen = 171;
					}
				}
			}
			else		// 2.3 khz A
			{
				if (imscprot) 		// prot level 1
				{
					if (imscqam) {			// QAM 64
						streamlen = 288;
						packlen = 285;
					}
					else {					// QAM 16
						streamlen = 200;
						packlen = 197;
					}
				}
				else 				// prot level 0
				{
					if (imscqam) {			// QAM 64
						streamlen = 240;
						packlen = 237;
					}
					else {					// QAM 16
						streamlen = 160;
						packlen = 157;
					}
				}
			}
		}
		else
		{
			if (ispec)	// 2.5 khz E
			{
				if (imscprot) 		// prot level 1
				{
					if (imscqam) {			// QAM 64
						streamlen = 163;
						packlen = 160;
					}
					else {					// QAM 16
						streamlen = 113;
						packlen = 110;
					}
				}
				else 				// prot level 0
				{
					if (imscqam) {			// QAM 64
						streamlen = 135;
						packlen = 132;
					}
					else {					// QAM 16
						streamlen = 90;
						packlen = 87;
					}
				}
			}
			else
			{
				if (imscprot) 		// prot level 1
				{
					if (imscqam) {			// QAM 64
						streamlen = 149;
						packlen = 146;
					}
					else {					// QAM 16
						streamlen = 103;
						packlen = 100;
					}
				}
				else 				// prot level 0
				{
					if (imscqam) {			// QAM 64
						streamlen = 124;
						packlen = 121;
					}
					else {					// QAM 16
						streamlen = 83;
						packlen = 80;
					}
				}
			}
		}

		/* Audio/Data flag */
		switch ((*pbiFACData).Separate(1))
		{
		case 0: /* 0 */
			Parameter.SetNumOfServices(1, 0);
			Parameter.SetAudDataFlag(0, CParameter::SF_AUDIO);
			isaudio = true;

			/* Load audio parameters class with current parameters */
			AudParam = Parameter.GetAudioParam(0);

			/* Audio coding */

			switch ((*pbiFACData).Separate(2))
			{
			case 0: /* 00 */
				AudParam.eAudioCoding = CParameter::AC_LPC;
				break;
			case 1: /* 01 */
				AudParam.eAudioCoding = CParameter::AC_CELP;
				break;
			case 2: /* 10 */
				AudParam.eAudioCoding = CParameter::AC_SPEEX;
				break;
			case 3: /* 11 */
				AudParam.eAudioCoding = CParameter::AC_SSTV;
				break;
			default: /* reserved */
				return FALSE;
				break;
			}

			/* Text flag */
			switch ((*pbiFACData).Separate(1))
			{
			case 0: /* 0 */
				AudParam.bTextflag = FALSE;
				break;

			case 1: /* 1 */
				AudParam.bTextflag = TRUE;
				break;
			}

			AudParam.iStreamID = 0;

			Parameter.SetAudioParam(0, AudParam);
			break;

		case 1: /* 1 */
			Parameter.SetNumOfServices(0, 1);
			Parameter.SetAudDataFlag(0, CParameter::SF_DATA);

			/* Load data parameters class with current parameters */
			DataParam = Parameter.GetDataParam(0);

			// set default for motslideshow
			DataParam.iStreamID = 0;
			DataParam.ePacketModInd = CParameter::PM_PACKET_MODE;
			DataParam.eDataUnitInd = CParameter::DU_DATA_UNITS;
			DataParam.iPacketID = (*pbiFACData).Separate(2);
			DataParam.eAppDomain = CParameter::AD_DAB_SPEC_APP;
			DataParam.iUserAppIdent = 2;
			// unused bit
			if ((*pbiFACData).Separate(1) == 1)
			{
				imsclowqam = TRUE; //this is for QAM4
				if (Parameter.GetWaveMode() == RM_ROBUSTNESS_MODE_B)
				{
					if (ispec) {	// 2.5 khz B
						streamlen = 78;
						packlen = 75;
					}
					else {		// 2.3 khz B
						streamlen = 67;
						packlen = 64;
					}
				}
				else if (Parameter.GetWaveMode() == RM_ROBUSTNESS_MODE_A)
				{
					if (ispec) {	// 2.5 khz A
						streamlen = 104;
						packlen = 101;
					}
					else {		// 2.3 khz A
						streamlen = 96;
						packlen = 93;
					}
				}
				else
				{
					if (ispec) {	// 2.5 khz A
						streamlen = 54;
						packlen = 51;
					}
					else {		// 2.3 khz A
						streamlen = 49;
						packlen = 46;
					}
				}
			}
			DataParam.iPacketLen = packlen;
			Parameter.SetDataParam(0, DataParam);
			break;
		}
		/*
		// Set MSC coding scheme
		if (imsclowqam)
			Parameter.SetMSCCodingScheme(CParameter::CS_1_SM);
		else if (imscqam)
			Parameter.SetMSCCodingScheme(CParameter::CS_3_SM);
		else
			Parameter.SetMSCCodingScheme(CParameter::CS_2_SM);
		// Set Stream Length
		Parameter.SetStreamLen(0, streamlen);
		*/

		/* Label */
		{
			char	cNewChar = 0; //init DM
			int iframe = Parameter.iFrameIDReceiv;
			/* Reset label string */
			strlabel[iframe] = "";
			for (int i = 0; i < 3; i++)
			{
				/* Get character */
				cNewChar = (*pbiFACData).Separate(7);

				/* Append new character */
				if (cNewChar != 0)
					strlabel[iframe].append(&cNewChar, 1);
			}
			if (iframe == 0) ilabelstate |= 1;
			if (iframe == 1) ilabelstate |= 2;
			if (iframe == 2) ilabelstate |= 4;

			if (ilabelstate == 7)
			{
				Parameter.Service[0].strLabel = "";
				Parameter.Service[0].strLabel += strlabel[0];
				Parameter.Service[0].strLabel += strlabel[1];
				Parameter.Service[0].strLabel += strlabel[2];
				cNewChar = 0;
				Parameter.Service[0].strLabel.append(&cNewChar, 1);
			}

		}
		//Spare bit =================================================================================================
		if ((isaudio) && ((*pbiFACData).Separate(1) == 1))
		{
			Parameter.SetMSCCodingScheme(CParameter::CS_1_SM); //set QAM4 in Voice mode if used DM

			imsclowqam = TRUE; //this is for QAM4
			if (Parameter.GetWaveMode() == RM_ROBUSTNESS_MODE_B)
			{
				if (ispec) {	// 2.5 khz B
					streamlen = 78;
					packlen = 75;
				}
				else {		// 2.3 khz B
					streamlen = 67;
					packlen = 64;
				}
			}
			else if (Parameter.GetWaveMode() == RM_ROBUSTNESS_MODE_A)
			{
				if (ispec) {	// 2.5 khz A
					streamlen = 104;
					packlen = 101;
				}
				else {		// 2.3 khz A
					streamlen = 96;
					packlen = 93;
				}
			}
			else
			{
				if (ispec) {	// 2.5 khz A
					streamlen = 54;
					packlen = 51;
				}
				else {		// 2.3 khz A
					streamlen = 49;
					packlen = 46;
				}
			}
			DataParam.iPacketLen = packlen;
			Parameter.SetDataParam(0, DataParam);
		}
		//===========================================================================================================

		//This code is moved from higher up, so that modes aren't set more than once DM

		// Set MSC coding scheme
		if (imsclowqam)
			Parameter.SetMSCCodingScheme(CParameter::CS_1_SM);
		else if (imscqam)
			Parameter.SetMSCCodingScheme(CParameter::CS_3_SM);
		else
			Parameter.SetMSCCodingScheme(CParameter::CS_2_SM);

		// Set Stream Length
		Parameter.SetStreamLen(0, streamlen);
		//===========================================================================================================

		/* CRC is ok, return TRUE */
		return TRUE;
	}
	else
	{
		ilabelstate = 0;
		/* Data is corrupted, do not use it. Return failure as FALSE */
		return FALSE;
	}
}
