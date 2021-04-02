/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See Data.cpp
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

#if !defined(DATA_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DATA_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "Parameter.h"
#include "Modul.h"
#include "FAC/FAC.h"
#include "AudioFile.h"
#include <time.h>
#include "DRMSignalIO.h" /* For signal meter */


/* Definitions ****************************************************************/


/* Classes ********************************************************************/
class CReadData : public CTransmitterModul<_SAMPLE, _SAMPLE>
{
public:
	CReadData(CSound* pNS) :
#ifdef WRITE_TRNSM_TO_FILE
		iNumTransBlocks(DEFAULT_NUM_SIM_BLOCKS), iCounter(0),
#endif
		pSound(pNS) {}
	virtual ~CReadData() {}

	_REAL GetLevelMeter() {return SignalLevelMeter.Level();}

#ifdef WRITE_TRNSM_TO_FILE
	void SetNumTransBlocks(const int iNewNum) {iNumTransBlocks = iNewNum;}
#endif


protected:
	CSound*				pSound;
	CVector<_SAMPLE>	vecsSoundBuffer;
	CSignalLevelMeter	SignalLevelMeter;


	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class CWriteData : public CReceiverModul<_SAMPLE, _SAMPLE>
{
public:
	CWriteData(CSound* pNS) : bMuteAudio(FALSE), bDoWriteWaveFile(FALSE), pSound(pNS) {}
	virtual ~CWriteData() {}

	void StartWriteWaveFile(const string strFileName);
	_BOOLEAN GetIsWriteWaveFile() {return bDoWriteWaveFile;}
	void StopWriteWaveFile();
	void MuteAudio(_BOOLEAN bNewMA) {bMuteAudio = bNewMA;}
	_BOOLEAN GetMuteAudio() {return bMuteAudio;}

protected:
	CSound*		pSound;
	_BOOLEAN	bMuteAudio;
	CWaveFile	WaveFileAudio;
	_BOOLEAN	bDoWriteWaveFile;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};

/* FAC ---------------------------------------------------------------------- */
class CGenerateFACData : public CTransmitterModul<_BINARY, _BINARY>
{
public:
	CGenerateFACData() {}
	virtual ~CGenerateFACData() {}

protected:
	CFACTransmit FACTransmit;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class CUtilizeFACData : public CReceiverModul<_BINARY, _BINARY>
{
public:
	CUtilizeFACData() : bCRCOk(FALSE) {}
	virtual ~CUtilizeFACData() {}


	_BOOLEAN GetCRCOk() const {return bCRCOk;}

protected:
	CFACReceive FACReceive;

	_BOOLEAN	bCRCOk;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};




#endif // !defined(DATA_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
