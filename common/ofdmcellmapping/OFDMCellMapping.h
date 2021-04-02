/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	
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

#if !defined(OFDMCELLMAPPING_H__3B0BA660_CA63_4344_BB2BE7A0D31912__INCLUDED_)
#define OFDMCELLMAPPING_H__3B0BA660_CA63_4344_BB2BE7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../Modul.h"
#include "../tables/TableCarMap.h"
#include "../tables/TableFAC.h"


/* Classes ********************************************************************/
class COFDMCellMapping : public CTransmitterModul<_COMPLEX, _COMPLEX>
{
public:
	COFDMCellMapping() {}
	virtual ~COFDMCellMapping() {}

protected:
	int			iNumSymPerFrame{}; //init DM
	int			iNumCarrier{}; //init DM
	int			iSymbolCounter{}; //init DM
	_COMPLEX*	pcDummyCells{}; //init DM

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class COFDMCellDemapping : public CReceiverModul<CEquSig, CEquSig>
{
public:
	COFDMCellDemapping() {}
	virtual ~COFDMCellDemapping() {}

protected:
	int		iNumSymPerFrame{}; //init DM
	int		iNumCarrier{}; //init DM
	int		iNumUsefMSCCellsPerFrame{}; //init DM

	int		iSymbolCounter{}; //init DM
	int		iCurrentFrameID{}; //init DM

	int		iMSCCellCounter{}; //init DM
	int		iFACCellCounter{}; //init DM

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(OFDMCELLMAPPING_H__3B0BA660_CA63_4344_BB2BE7A0D31912__INCLUDED_)
