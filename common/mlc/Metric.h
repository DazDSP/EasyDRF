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

#if !defined(MLC_METRIC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define MLC_METRIC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../tables/TableQAMMapping.h"
#include "../Vector.h"
#include "../Parameter.h"


/* Classes ********************************************************************/
inline _REAL Metric(const _REAL rDist, const _REAL rChan)
{
	return rDist * sqrt(rChan);
}

class CMLCMetric
{
public:
	CMLCMetric() {}
	virtual ~CMLCMetric() {}

	/* Return the number of used symbols for calculating one branch-metric */
	void	CalculateMetric(CVector<CEquSig>* pcInSymb, 
						    CVector<CDistance>& vecMetric, 
							CVector<_BINARY>& vecbiSubsetDef1, 
							CVector<_BINARY>& vecbiSubsetDef2,
							CVector<_BINARY>& vecbiSubsetDef3, 
							CVector<_BINARY>& vecbiSubsetDef4,
							CVector<_BINARY>& vecbiSubsetDef5,
							CVector<_BINARY>& vecbiSubsetDef6,
							int iLevel, _BOOLEAN bIteration);
	void	Init(int iNewInputBlockSize, CParameter::ECodScheme eNewCodingScheme);


protected:
	inline _REAL Minimum1(const _REAL rA, const _REAL rB,
						  const _REAL rChan) const
	{
		/* The minium in case of only one parameter is trivial */
		return Metric(fabs(rA - rB), rChan);
	}

	inline _REAL Minimum2(const _REAL rA, const _REAL rB1, const _REAL rB2,
						  const _REAL rChan) const
	{
		/* First, calculate all distances */
		const _REAL rResult1 = fabs(rA - rB1);
		const _REAL rResult2 = fabs(rA - rB2);

		/* Return smalles one */
		if (rResult1 < rResult2)
			return Metric(rResult1, rChan);
		else
			return Metric(rResult2, rChan);
	}

	inline _REAL Minimum4(const _REAL rA, const _REAL rB1, const _REAL rB2,
						  const _REAL rB3, _REAL rB4, const _REAL rChan) const
	{
		/* First, calculate all distances */
		const _REAL rResult1 = fabs(rA - rB1);
		const _REAL rResult2 = fabs(rA - rB2);
		const _REAL rResult3 = fabs(rA - rB3);
		const _REAL rResult4 = fabs(rA - rB4);

		/* Search for smallest one */
		_REAL rReturn = rResult1;
		if (rResult2 < rReturn)
			rReturn = rResult2;
		if (rResult3 < rReturn)
			rReturn = rResult3;
		if (rResult4 < rReturn)
			rReturn = rResult4;

		return Metric(rReturn, rChan);
	}


	int						iInputBlockSize{}; //init DM
	CParameter::ECodScheme	eMapType{}; //init DM
};


#endif // !defined(MLC_METRIC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
