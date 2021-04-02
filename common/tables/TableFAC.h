/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Tables for FAC
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

#if !defined(TABLE_FAC_H__3B0_CA63_4344_BGDEB2B_23E7912__INCLUDED_)
#define TABLE_FAC_H__3B0_CA63_4344_BGDEB2B_23E7912__INCLUDED_

#include <string>
#include "../GlobalDefinitions.h"


/* Definitions ****************************************************************/
#define NUM_FAC_BITS_PER_BLOCK		48 

// fac_cells * quam_bits * coderule - tailbits = fac_bits
// 65 * 2 * 0.6 - 6 = 72
// see iCodRateCombFDC4SM for coderule

/* iTableNoOfServices[a][b]
   a: Number of audio services
   b: Number of data services 
   (6.3.4) */
const int iTableNoOfServices[5][5] = {
	/* -> Data */
	{-1,  1,  2,  3, 15},
	{ 4,  5,  6,  7, -1},
	{ 8,  9, 10, -1, -1},
	{12, 13, -1, -1, -1},
	{ 0, -1, -1, -1, -1}
};


#endif // !defined(TABLE_FAC_H__3B0_CA63_4344_BGDEB2B_23E7912__INCLUDED_)
