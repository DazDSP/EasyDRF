/******************************************************************************\
 * Copyright (c) 2002-2005
 *
 * Author(s):
 *  Francesco Lanza
 *
 * Description:
 *	mempool interface implementation
 *
 ******************************************************************************/

#include "poolid.h"

int iIDpool[numpoolele];
int age[numpoolele];
int actage;


BOOL CPoolID::ispoolid(int iID)
{
	// check for bsr file
	if (iID == 0) return FALSE;	//bsr.bin
	if (iID == 1) return FALSE;	//bsr.bin
	if (iID == 2) return FALSE;	//bsr.bin
	return TRUE;
}

BOOL CPoolID::storeinpool(int iID, int * position)
{
	int locposition = -1;
	int minage = actage + 1;

	for (int i=0;i<numpoolele;i++)
	{
		if (iID == iIDpool[i])
		{
			locposition = i;
			i = numpoolele;
		}
	}
	if (locposition >= 0)	// found in pool -> keep old segments !
	{		
		age[locposition] = actage;
		actage++;
		*position = locposition;
		return TRUE;
	}
	else				// not found in pool
	{
		for (int i=0;i<numpoolele;i++)
		{
			if (age[i] <= minage)
			{
				minage = age[i];
				locposition = i;
			}	
		}
		iIDpool[locposition] = iID;
		age[locposition] = actage;
		actage++;
		*position = locposition;
		return FALSE;
	}
}

BOOL CPoolID::getfrompool(int iID, int * position)
{
	int locposition = -1;
	// search for old pool entry
	for (int i=0;i<numpoolele;i++)
	{
		if (iID == iIDpool[i])
		{
			locposition = i;
			i = numpoolele;
		}
	}
	*position = locposition;
	if (locposition >= 0)	//found in pool
		return TRUE;
	else					//not found in pool
		return FALSE;
}

BOOL CPoolID::poolremove(int iID, int * position)
{
	int locposition = -1;
	// search for old pool entry
	for (int i=0;i<numpoolele;i++)
	{
		if (iID == iIDpool[i])
		{
			locposition = i;
			i = numpoolele;
		}
	}
	*position = locposition;
	if (locposition >= 0)	// found in pool
	{
		iIDpool[locposition] = -1;
		return TRUE;
	}
	else
		return FALSE;
}

void CPoolID::Reset()
{
	actage = 1;
	for (int i=0;i<numpoolele;i++)
	{
		age[i] = 0;
		iIDpool[i] = -1;
	}
}