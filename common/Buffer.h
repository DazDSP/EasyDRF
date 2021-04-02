/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * Transfer buffer between different modules
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

#if !defined(PUFFER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define PUFFER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "GlobalDefinitions.h"
#include "Vector.h"


/* Classes ********************************************************************/
/* Buffer base class */
template<class TData> class CBuffer
{
public:
	CBuffer() : iBufferSize(0), bRequestFlag(FALSE) {}
	virtual	~CBuffer() {}

	void			SetRequestFlag(const _BOOLEAN bNewRequestFlag)
						{bRequestFlag = bNewRequestFlag;}
	_BOOLEAN		GetRequestFlag() const {return bRequestFlag;}

	/* Virtual function to be declared by the derived object */
	virtual void				Init(const int iNewBufferSize);
	virtual CVectorEx<TData>*	Get(const int iRequestedSize) = 0;
	virtual CVectorEx<TData>*	QueryWriteBuffer() = 0;
	virtual void				Put(const int iOfferedSize) = 0;
	virtual void				Clear() = 0;
	virtual int					GetFillLevel() const = 0;

protected:
	CVectorEx<TData>	vecBuffer;
	int					iBufferSize;

	_BOOLEAN			bRequestFlag;
};

/* Single block buffer */
template<class TData> class CSingleBuffer : public CBuffer<TData>
{
public:
	CSingleBuffer() : iFillLevel(0) {}
	virtual	~CSingleBuffer() {}

	virtual void				Init(const int iNewBufferSize);
	virtual CVectorEx<TData>*	Get(const int iRequestedSize);
	virtual CVectorEx<TData>*	QueryWriteBuffer() {return &vecBuffer;}
	virtual void				Put(const int iOfferedSize);
	virtual void				Clear() {iFillLevel = 0;}
	virtual int					GetFillLevel() const {return iFillLevel;}

protected:
	int iFillLevel;
};

/* Cyclic buffer */
template<class TData> class CCyclicBuffer : public CBuffer<TData>
{
public:
	enum EBufferState {BS_FULL, BS_EMPTY}; // BS: Buffer Status

	CCyclicBuffer() {Clear();}
	virtual	~CCyclicBuffer() {}

	virtual void				Init(const int iNewBufferSize);
	virtual CVectorEx<TData>*	Get(const int iRequestedSize);
	virtual CVectorEx<TData>*	QueryWriteBuffer() {return &vecInOutBuffer;}
	virtual void				Put(const int iOfferedSize);
	virtual void				Clear();
	virtual int					GetFillLevel() const;

protected:
	CVectorEx<TData>	vecInOutBuffer;

	int					iPut;
	int					iGet;
	EBufferState		iBufferState;
};


/* Implementation *************************************************************/
template<class TData> void CBuffer<TData>::Init(const int iNewBufferSize)
{
	/* Assign buffer size */
	iBufferSize = iNewBufferSize;

	/* Allocate memory for data field */
	vecBuffer.Init(iBufferSize);
}


/******************************************************************************\
* Single block buffer														   *
\******************************************************************************/
template<class TData> void CSingleBuffer<TData>::Init(const int iNewBufferSize)
{
	CBuffer<TData>::Init(iNewBufferSize);

	/* Reset buffer parameters (empty buffer) */
	iFillLevel = 0;
}

template<class TData> CVectorEx<TData>* CSingleBuffer<TData>::Get(const int iRequestedSize)
{
	/* Get data ***************************************************************/
#ifdef _DEBUG_
	if (iRequestedSize > iFillLevel)
	{
		DebugError("SingleBuffer Get()", "FillLevel",
			iFillLevel, "Requested size", iRequestedSize);
	}
#endif

	/* Block is read, buffer is now empty again */
	iFillLevel -= iRequestedSize;

	return &vecBuffer;		
}

template<class TData> void CSingleBuffer<TData>::Put(const int iOfferedSize)
{
	/* New Block was added, set new fill level */
	iFillLevel += iOfferedSize;

#ifdef _DEBUG_
	if (iFillLevel > iBufferSize)
	{
		DebugError("SingleBuffer Put()", "FillLevel",
			iFillLevel, "Buffer size", iBufferSize);
	}
#endif
}


/******************************************************************************\
* Cyclic buffer																   *
\******************************************************************************/
template<class TData> void CCyclicBuffer<TData>::Init(const int iNewBufferSize)
{
	CBuffer<TData>::Init(iNewBufferSize);

	/* Make in- and output buffer the same size as the main buffer to
	   make sure that the worst-case is no problem */
	vecInOutBuffer.Init(iNewBufferSize);

	/* Reset buffer parameters (empty buffer) */
	iPut = 0;
	iGet = 0;
	iBufferState = BS_EMPTY;
}

template<class TData> void CCyclicBuffer<TData>::Clear()
{
	/* Clear buffer by resetting the pointer */
	iPut = 0;
	iGet = 0;
	iBufferState = BS_EMPTY;
	bRequestFlag = FALSE;
}

template<class TData> CVectorEx<TData>* CCyclicBuffer<TData>::Get(const int iRequestedSize)
{
	int	i;
	int	iAvailSpace;
	int iElementCount;

	/* Test if enough data is available for reading */
	iAvailSpace = iPut - iGet;
	/* Test if wrap is needed */
	if ((iAvailSpace < 0) || ((iAvailSpace == 0) && (iBufferState == BS_FULL)))
		iAvailSpace += iBufferSize;

#ifdef _DEBUG_
	if (iAvailSpace < iRequestedSize)
	{
		DebugError("CyclicBuffer Get()", "Availabe space",
			iAvailSpace, "Requested size", iAvailSpace);
	}
#endif

	/* Get data ***************************************************************/
	iElementCount = 0;

	/* Test if data can be read in one block */
	if (iBufferSize - iGet < iRequestedSize)
	{
		/* Data must be read in two portions */
		for (i = iGet; i < iBufferSize; i++)
		{
			vecInOutBuffer[iElementCount] = vecBuffer[i];
			iElementCount++;
		}
		for (i = 0; i < iRequestedSize - iBufferSize + iGet; i++)
		{
			vecInOutBuffer[iElementCount] = vecBuffer[i];
			iElementCount++;
		}
	}
	else
	{
		/* Data can be read in one block */
		for (i = iGet; i < iGet + iRequestedSize; i++)
		{
			vecInOutBuffer[iElementCount] = vecBuffer[i];
			iElementCount++;
		}
	}

	/* Adjust iGet pointer */
	iGet += iRequestedSize;
	if (iGet >= iBufferSize)
		iGet -= iBufferSize;

	/* Test if buffer is empty. If yes, set empty-flag */
	if ((iGet == iPut) && (iRequestedSize > 0))
		iBufferState = BS_EMPTY;

	return &vecInOutBuffer;		
}

template<class TData> void CCyclicBuffer<TData>::Put(const int iOfferedSize)
{
	int	iAvailSpace;
	int	i;
	int iElementCount;

	/* Test if enough data is available for writing */
	iAvailSpace = iGet - iPut;
	/* Test if wrap is needed */
	if ((iAvailSpace < 0) || ((iAvailSpace == 0) && (iBufferState == BS_EMPTY)))
		iAvailSpace += iBufferSize;

#ifdef _DEBUG_
	if (iAvailSpace < iOfferedSize)
	{
		DebugError("CyclicBuffer Put()", "Availabe space",
			iAvailSpace, "Offered size", iOfferedSize);
	}
#endif

	/* Put data ***************************************************************/
	iElementCount = 0;

	/* Test if data can be written in one block */
	if (iBufferSize - iPut < iOfferedSize)
	{
		/* Data must be written in two steps */
		for (i = iPut; i < iBufferSize; i++)
		{
			vecBuffer[i] = vecInOutBuffer[iElementCount];
			iElementCount++;
		}
		for (i = 0; i < iOfferedSize - iBufferSize + iPut; i++)
		{
			vecBuffer[i] = vecInOutBuffer[iElementCount];
			iElementCount++;
		}
	}
	else
	{
		/* Data can be written in one block */
		for (i = iPut; i < iPut + iOfferedSize; i++)
		{
			vecBuffer[i] = vecInOutBuffer[iElementCount];
			iElementCount++;
		}
	}

	/* Adjust iPut pointer */
	iPut += iOfferedSize;
	if (iPut >= iBufferSize)
		iPut -= iBufferSize;

	/* Test if buffer is full. If yes, set full-flag */
	if ((iGet == iPut) && (iOfferedSize > 0))
		iBufferState = BS_FULL;
}

template<class TData> int CCyclicBuffer<TData>::GetFillLevel() const
{
	int iFillLevel;

	/* Calculate the available data in the buffer. Test if wrap is needed!
	   Take into account the flag-information (full or empty buffer) */
	iFillLevel = iPut - iGet;
	if ((iFillLevel == 0) && (iBufferState == BS_FULL))
		iFillLevel = iBufferSize;
	if (iFillLevel < 0)
		iFillLevel += iBufferSize;	/* Wrap around */

	return iFillLevel;
}


#endif // !defined(PUFFER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
