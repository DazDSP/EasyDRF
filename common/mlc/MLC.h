/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See MLC.cpp
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

#if !defined(MLC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define MLC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Modul.h"
#include "../Parameter.h"
#include "../tables/TableMLC.h"
#include "../tables/TableCarMap.h"
#include "ConvEncoder.h"
#include "ViterbiDecoder.h"
#include "Metric.h"
#include "BitInterleaver.h"
#include "QAMMapping.h"
#include "EnergyDispersal.h"


/* Classes ********************************************************************/
class CMLC
{
public:
	CMLC() : eChannelType(CParameter::CT_MSC), iN_mux(0) {}
	virtual ~CMLC() {}

	void CalculateParam(CParameter& Parameter, int iNewChannelType);

protected:
	int	iLevels{}; //init DM
	/* No input bits for each level. First index: Level, second index:
	   Protection level.
	   For three levels: [M_0,l  M_1,l  M2,l]
	   For six levels: [M_0,lRe  M_0,lIm  M_1,lRe  M_1,lIm  M_2,lRe  ...  ] */
	int	iM[MC_MAX_NUM_LEVELS][2]{}; //init DM
	int iN[2]{}; //init DM
	int iL[3]{}; //init DM
	int iN_mux{}; //init DM
	int iCodeRate[MC_MAX_NUM_LEVELS][2]{}; //init DM

	const int* piInterlSequ{}; //init DM

	int	iNumEncBits{}; //init DM

	CParameter::EChanType	eChannelType{}; //init DM
	CParameter::ECodScheme	eCodingScheme{}; //init DM
};

class CMLCEncoder : public CTransmitterModul<_BINARY, _COMPLEX>, 
					public CMLC
{
public:
	CMLCEncoder() {}
	virtual ~CMLCEncoder() {}

protected:
	CConvEncoder		ConvEncoder[MC_MAX_NUM_LEVELS];
	/* Two different types of interleaver table */
	CBitInterleaver		BitInterleaver[2];
	CQAMMapping			QAMMapping;
	CEngergyDispersal	EnergyDisp;

	/* Internal buffers */
	CVector<_BINARY>	vecbiEncInBuffer[MC_MAX_NUM_LEVELS];
	CVector<_BINARY>	vecbiEncOutBuffer[MC_MAX_NUM_LEVELS];

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& Parameter);
};

class CMLCDecoder : public CReceiverModul<CEquSig, _BINARY>, 
					public CMLC
{
public:
	CMLCDecoder() : iInitNumIterations(MC_NUM_ITERATIONS) {}
	virtual ~CMLCDecoder() {}

	_REAL GetAccMetric() const {return 10 * log10(rAccMetric);}
	void GetVectorSpace(CVector<_COMPLEX>& veccData);
	void SetNumIterations(int iNewNumIterations)
		{iInitNumIterations = iNewNumIterations; SetInitFlag();}
	int GetInitNumIterations() const {return iInitNumIterations;}

protected:
	CViterbiDecoder		ViterbiDecoder[MC_MAX_NUM_LEVELS]{}; //init DM
	CMLCMetric			MLCMetric{}; //init DM
	/* Two different types of deinterleaver table */
	CBitDeinterleaver	BitDeinterleaver[2]{}; //init DM
	CBitInterleaver		BitInterleaver[2]{}; //init DM
	CConvEncoder		ConvEncoder[MC_MAX_NUM_LEVELS]{}; //init DM
	CEngergyDispersal	EnergyDisp{}; //init DM

	/* Internal buffers */
	CVector<CDistance>	vecMetric{}; //init DM

	CVector<_BINARY>	vecbiDecOutBits[MC_MAX_NUM_LEVELS]{}; //init DM
	CVector<_BINARY>	vecbiSubsetDef[MC_MAX_NUM_LEVELS]{}; //init DM
	int					iNumOutBits{}; //init DM

	/* Accumulated metric */
	_REAL				rAccMetric{}; //init DM

	/* Internal buffer for GetVectorSpace function */
	CVector<_COMPLEX>	vecSigSpacBuf{}; //init DM

	int					iNumIterations{}; //init DM
	int					iInitNumIterations{}; //init DM
	int					iIndexLastBranch{}; //init DM

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


/******************************************************************************\
* Customized channel (de-)coders											   *
\******************************************************************************/
class CMSCMLCEncoder : public CMLCEncoder
{
protected:
	virtual void InitInternal(CParameter& TransmParam)
	{
		/* Set corresponding type */
		eChannelType = CParameter::CT_MSC;

		/* Call init in encoder */
		CMLCEncoder::InitInternal(TransmParam);
	};
};

class CFACMLCEncoder : public CMLCEncoder
{
protected:
	virtual void InitInternal(CParameter& TransmParam)
	{
		/* Set corresponding type */
		eChannelType = CParameter::CT_FAC;

		/* Call init in encoder */
		CMLCEncoder::InitInternal(TransmParam);
	};
};

class CMSCMLCDecoder : public CMLCDecoder
{
protected:
	virtual void InitInternal(CParameter& ReceiverParam)
	{
		/* Set corresponding type */
		eChannelType = CParameter::CT_MSC;

		/* Call init in encoder */
		CMLCDecoder::InitInternal(ReceiverParam);
	};
};

class CFACMLCDecoder : public CMLCDecoder
{
protected:
	virtual void InitInternal(CParameter& ReceiverParam)
	{
		/* Set corresponding type */
		eChannelType = CParameter::CT_FAC;

		/* Call init in encoder */
		CMLCDecoder::InitInternal(ReceiverParam);
	};
};


#endif // !defined(MLC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
