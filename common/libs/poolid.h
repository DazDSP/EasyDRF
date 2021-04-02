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

#if !defined(POOLID_H__3P0UBVE93452KJVEW363E7A0D31912__INCLUDED_)
#define POOLID_H__3P0UBVE93452KJVEW363E7A0D31912__INCLUDED_

#include <windows.h>

/* Classes ********************************************************************/

#define numpoolele 5

class CPoolID
{
public:
	CPoolID() { Reset();}

	void Reset();

	BOOL ispoolid(int iID);
	BOOL storeinpool(int iID, int * position);
	BOOL getfrompool(int iID, int * position);
	BOOL poolremove(int iID, int * position);
};

#endif // 
