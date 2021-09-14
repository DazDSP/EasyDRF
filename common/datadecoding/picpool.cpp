/******************************************************************************\
 * Copyright (c) 2005
 *
 * Author(s):
 *  Francesco Lanza
 *
 * Description:
 *	picpool interface implementation
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
#define WIN32_LEAN_AND_MEAN        
#include <windows.h>
#include "picpool.h"

void CopyNew(CMOTObjectRaw::CDataUnitRx& input, CMOTObjectRaw::CDataUnitRx& output)
{
	int segsize = 0; //init DM
	int segno = 0; //unused?? DM
	output.bOK = input.bOK;
	output.bReady = input.bReady;
	output.iDataSegNum = input.iDataSegNum;
	output.iTotSegments = input.iTotSegments;
	segsize = input.vvbiSegment.Size();
	output.vvbiSegment.Init(segsize);
	for (int i=0;i<segsize;i++)
	{
		output.vvbiSegment[i].Init(input.vvbiSegment[i].Size());
		output.vvbiSegment[i] = input.vvbiSegment[i];
	}
	output.iDataSegNum = input.iDataSegNum;
}
void CopyNewH(CMOTObjectRaw::CDataUnit& input, CMOTObjectRaw::CDataUnit& output)
{
	int segsize = 0; //init DM
	output.bOK = input.bOK;
	output.bReady = input.bReady;
	output.iDataSegNum = input.iDataSegNum;
	segsize = input.vecbiData.Size();
	output.vecbiData.Init(segsize);
	for (int i=0;i<segsize;i++)
	{
		output.vecbiData[i] = input.vecbiData[i];
	}
}

void CopyOld(CMOTObjectRaw::CDataUnitRx& input, CMOTObjectRaw::CDataUnitRx& output)
{
	int segsizein = 0,segsizeout = 0; //init DM
	int segno = 0;
	if (input.bOK) output.bOK = TRUE;
	if (input.bReady) output.bReady = TRUE;
	if (input.iTotSegments >= output.iTotSegments) output.iTotSegments = input.iTotSegments;
	segsizein = input.vvbiSegment.Size();
	segsizeout = output.vvbiSegment.Size();
	if (segsizein > segsizeout)	output.vvbiSegment.Enlarge(segsizein-segsizeout);
	for (int i=0;i<segsizein;i++)
	{
		if (input.vvbiSegment[i].Size() > 0)
		{
			output.vvbiSegment[i].Init(input.vvbiSegment[i].Size());
			output.vvbiSegment[i] = input.vvbiSegment[i];
		}
		if (output.vvbiSegment[i].Size() > 0) segno++;
	}
	output.iDataSegNum = segno;
}
void CopyOldH(CMOTObjectRaw::CDataUnit& input, CMOTObjectRaw::CDataUnit& output)
{
	int segsizein = 0,segsizeout = 0; //init DM
	if (input.bOK) output.bOK = TRUE;
	if (input.bReady) output.bReady = TRUE;
	if (input.iDataSegNum >= output.iDataSegNum) input.iDataSegNum = output.iDataSegNum;
	segsizein = input.vecbiData.Size();
	segsizeout = output.vecbiData.Size();
	if (segsizein > segsizeout)	output.vecbiData.Enlarge(segsizein-segsizeout);
	for (int i=0;i<segsizein;i++)
	{
		output.vecbiData[i] = input.vecbiData[i];
	}
}

void CPicPool::storeinpool(CMOTObjectRaw& input)
{
	int position = 0; //init DM

	if (!poolID.ispoolid(input.iTransportID)) return;

	if (poolID.storeinpool(input.iTransportID,&position))	// found in pool
	{		
		CopyOld(input.BodyRx,picpool[position].BodyRx);
		CopyOldH(input.Header,picpool[position].Header);
	}
	else
	{
		CopyNewH(input.Header,picpool[position].Header);
		CopyNew(input.BodyRx,picpool[position].BodyRx);
		picpool[position].iSegmentSize = input.iSegmentSize;
	}
}

void CPicPool::getfrompool(int transid, CMOTObjectRaw& output)
{
	int position = 0;
	if (poolID.getfrompool(transid,&position))	// found in pool
	{
		CopyNew(picpool[position].BodyRx,output.BodyRx);
		CopyNewH(picpool[position].Header,output.Header);
		output.iSegmentSize = picpool[position].iSegmentSize;
		output.iTransportID = transid;
	}
	else
	{
		output.BodyRx.Reset();
		output.Header.Reset();
		output.iSegmentSize = 0;
		output.iTransportID = transid;
	}
}

void CPicPool::poolremove(int transid)
{
	int position = 0;
	if (poolID.poolremove(transid,&position))	// found in pool
	{
		picpool[position].Body.Reset();
		picpool[position].BodyRx.Reset();
		picpool[position].Header.Reset();
		picpool[position].iTransportID = -1;
		picpool[position].iSegmentSize = -1;
	}
}

void CPicPool::Reset()
{
	poolID.Reset();
	for (int i=0;i<numpoolele;i++)
	{
		picpool[i].Body.Reset();
		picpool[i].BodyRx.Reset();
		picpool[i].Header.Reset();
		picpool[i].iTransportID = -1;
		picpool[i].iSegmentSize = -1;
	}
}