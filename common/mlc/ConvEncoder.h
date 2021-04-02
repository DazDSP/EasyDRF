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

#if !defined(CONNVOL_ENC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define CONNVOL_ENC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../tables/TableMLC.h"
#include "../Vector.h"
#include "../Parameter.h"
#include "ChannelCode.h"


/* Classes ********************************************************************/
class CConvEncoder : public CChannelCode
{
public:
	CConvEncoder() {}
	virtual ~CConvEncoder() {}

	int		Encode(CVector<_BINARY>& vecInputData, 
				   CVector<_BINARY>& vecOutputData);
	void	Init(CParameter::ECodScheme eNewCodingScheme,
				 CParameter::EChanType eNewChannelType,
				 int iN1, int iN2, int iNewNumInBitsPartA,
				 int iNewNumInBitsPartB, int iPunctPatPartA, int iPunctPatPartB,
				 int iLevel);

protected:
	int						iNumInBits{}; //init DM
	int						iNumInBitsWithMemory{}; //init DM

	CVector<int>			veciTablePuncPat{}; //init DM

	CParameter::EChanType	eChannelType{}; //init DM
};


#endif // !defined(CONNVOL_ENC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
