/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DRMSignalIO.cpp
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

#if !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
#define DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_

#include "Parameter.h"
#include "Modul.h"
#include <math.h>
#include "matlib/Matlib.h"
#include "TransmitterFilter.h"

#include "../sound/sound.h"


/* Definitions ****************************************************************/
#define	METER_FLY_BACK				15

/* Length of vector for input spectrum. We use approx. 0.2 sec
   of sampled data for spectrum calculation, this is 2^13 = 8192 to 
   make the FFT work more efficient */
#define NUM_SMPLS_4_INPUT_SPECTRUM	8192

/* Use raw 16 bit data or in text form for file format for DRM data. Defining
   the following macro will enable the raw data option */
#define FILE_DRM_USING_RAW_DATA

/* If this flag is defined, both input channels are mixed together, therefore
   no right or left channel choice must be made */
//#define MIX_INPUT_CHANNELS

/* Chose recording channel: 0: Left, 1: Right, disabled if previous flag is
   set! */
#define RECORDING_CHANNEL			0


/* Classes ********************************************************************/

class CSignalLevelMeter
{
public:
	CSignalLevelMeter() : rCurLevel((_REAL) 0.0) {}
	virtual ~CSignalLevelMeter() {}

	void Init(_REAL rStartVal) {rCurLevel = Abs(rStartVal);}
	void Update(_REAL rVal);
	void Update(CVector<_REAL> vecrVal);
	void Update(CVector<_SAMPLE> vecsVal);
	_REAL Level();

protected:
	_REAL rCurLevel;
};


class CTransmitData : public CTransmitterModul<_COMPLEX, _COMPLEX>
{
public:
	enum EOutFormat {OF_REAL_VAL /* real valued */, OF_IQ /* I / Q */, OF_EP /* envelope / phase */};

	CTransmitData(CSound* pNS) : pSound(pNS), eOutputFormat(OF_REAL_VAL), rDefCarOffset((_REAL) VIRTUAL_INTERMED_FREQ) {}
	void SetIQOutput(const EOutFormat eFormat) {eOutputFormat = eFormat;}
	EOutFormat GetIQOutput() {return eOutputFormat;}
	virtual ~CTransmitData();

	void SetCarOffset(const CReal rNewCarOffset)
		{rDefCarOffset = rNewCarOffset;}

protected:
	CSound*			pSound;
	CVector<short>	vecsDataOut;
	int				iBlockCnt;
	int				iNumBlocks;
	EOutFormat		eOutputFormat;

	CReal			rDefCarOffset;
	CRealVector		rvecA;
	CRealVector		rvecB;
	CRealVector		rvecZReal; /* State memory real part */
	CRealVector		rvecZImag; /* State memory imaginary part */
	CRealVector		rvecDataReal;
	CRealVector		rvecDataImag;

	CReal			rNormFactor;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& Parameter);
};

class CHanningWindow
{
public:
	CHanningWindow() : bIsInit(FALSE) {}
	virtual ~CHanningWindow() {}

	void Init(void);

	CRealVector				vecrHannWind;
	_BOOLEAN				bIsInit;
};

class CReceiveData : public CReceiverModul<_REAL, _REAL>
{
public:
	CReceiveData(CSound* pNS) : bFippedSpectrum(FALSE), pFileReceiver(NULL), bUseSoundcard(TRUE), bNewUseSoundcard(TRUE), pSound(pNS), vecrInpData(NUM_SMPLS_4_INPUT_SPECTRUM, (_REAL)0.0) {} //added DM
//	CReceiveData(CSound* pNS) : pFileReceiver(NULL), pSound(pNS), vecrInpData(NUM_SMPLS_4_INPUT_SPECTRUM, (_REAL) 0.0) {}
	virtual ~CReceiveData();

	_REAL GetLevelMeter() {return SignalLevelMeter.Level();}
	void GetInputSpec(CVector<_REAL>& vecrData);
	
	//added DM
	void SetFlippedSpectrum(const _BOOLEAN bNewF) { bFippedSpectrum = bNewF; }
	_BOOLEAN GetFlippedSpectrum() { return bFippedSpectrum; }

	void SetUseSoundcard(const _BOOLEAN bNewUS)
	{
		bUseSoundcard = bNewUS;
		SetInitFlag();
	}
	//added DM end

protected:
	CSignalLevelMeter		SignalLevelMeter;
	CHanningWindow			HanningWindow;
	
	FILE*					pFileReceiver;

	CSound*					pSound;
	CVector<_SAMPLE>		vecsSoundBuffer;

	CShiftRegister<_REAL>	vecrInpData;

	_BOOLEAN				bFippedSpectrum; //added DM
	_BOOLEAN				bUseSoundcard; //added DM
	_BOOLEAN				bNewUseSoundcard; //added DM

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
