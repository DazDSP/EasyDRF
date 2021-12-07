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

#if !defined(QAM_MAPPING_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define QAM_MAPPING_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../tables/TableQAMMapping.h"
#include "../Vector.h"
#include "../Parameter.h"


/* Classes ********************************************************************/
class CQAMMapping
{
public:
	CQAMMapping() {}
	virtual ~CQAMMapping() {}

	void Map(CVector<_BINARY>& vecbiInputData1, 
			 CVector<_BINARY>& vecbiInputData2, 
			 CVector<_BINARY>& vecbiInputData3, 
			 CVector<_BINARY>& vecbiInputData4, 
			 CVector<_BINARY>& vecbiInputData5, 
			 CVector<_BINARY>& vecbiInputData6,
			 CVector<_COMPLEX>* pcOutputData);
	void Init(int iNewOutputBlockSize, CParameter::ECodScheme eNewCodingScheme);

protected:
	int						iOutputBlockSize;
	CParameter::ECodScheme	eMapType;
};


#endif // !defined(QAM_MAPPING_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
