/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	High level class for all modules. The common functionality for reading
 *	and writing the transfer-buffers are implemented here.
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

#if !defined(AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_)
#define AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_

#include "Buffer.h"
#include "Vector.h"
#include "Parameter.h"


/* Classes ********************************************************************/
/* CModul ------------------------------------------------------------------- */
template<class TInput, class TOutput> 
class CModul  
{
public:
	CModul();
	virtual ~CModul() {}

	virtual void Init(CParameter& Parameter);
	virtual void Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);

protected:
	CVectorEx<TInput>*	pvecInputData;
	CVectorEx<TOutput>*	pvecOutputData;

	/* Max block-size are used to determine the size of the requiered buffer */
	int					iMaxOutputBlockSize;
	/* Actual read (or written) size of the data */
	int					iInputBlockSize;
	int					iOutputBlockSize;

	void				Lock() {Mutex.Lock();}
	void				Unlock() {Mutex.Unlock();}

	void				InitThreadSave(CParameter& Parameter);
	virtual void		InitInternal(CParameter& Parameter) = 0;
	void				ProcessDataThreadSave(CParameter& Parameter);
	virtual void		ProcessDataInternal(CParameter& Parameter) = 0;

private:
	CMutex				Mutex;
};


/* CTransmitterModul -------------------------------------------------------- */
template<class TInput, class TOutput> 
class CTransmitterModul : public CModul<TInput, TOutput>
{
public:
	CTransmitterModul();
	virtual ~CTransmitterModul() {}

	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);
	virtual void		ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);
	virtual void		ProcessData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TOutput>& OutputBuffer);
	virtual void		ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TInput>& InputBuffer2, CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer);

protected:
	/* Additional buffers if the derived class has multiple input streams */
	CVectorEx<TInput>*	pvecInputData2;

	/* Actual read (or written) size of the data */
	int					iInputBlockSize2;
};


/* CReceiverModul ----------------------------------------------------------- */
template<class TInput, class TOutput> 
class CReceiverModul : public CModul<TInput, TOutput>
{
public:
	CReceiverModul();
	virtual ~CReceiverModul() {}

	void				SetInitFlag() {bDoInit = TRUE;}
	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);
	virtual void		Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer, CBuffer<TOutput>& OutputBuffer2);
	virtual void		Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer, CBuffer<TOutput>& OutputBuffer2, CBuffer<TOutput>& OutputBuffer3);
	virtual void		ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TOutput>& OutputBuffer, CBuffer<TOutput>& OutputBuffer2);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TOutput>& OutputBuffer, CBuffer<TOutput>& OutputBuffer2, CBuffer<TOutput>& OutputBuffer3);
	virtual _BOOLEAN	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer);

protected:
	void SetBufReset1() {bResetBuf = TRUE;}
	void SetBufReset2() {bResetBuf2 = TRUE;}
	void SetBufReset3() {bResetBuf3 = TRUE;}

	/* Additional buffers if the derived class has multiple output streams */
	CVectorEx<TOutput>*	pvecOutputData2;
	CVectorEx<TOutput>*	pvecOutputData3;

	/* Max block-size are used to determine the size of the requiered buffer */
	int					iMaxOutputBlockSize2;
	int					iMaxOutputBlockSize3;
	/* Actual read (or written) size of the data */
	int					iOutputBlockSize2;
	int					iOutputBlockSize3;

private:
	/* Init flag */
	_BOOLEAN			bDoInit;

	/* Reset flags for output cyclic-buffers */
	_BOOLEAN			bResetBuf;
	_BOOLEAN			bResetBuf2;
	_BOOLEAN			bResetBuf3;
};

/* Implementation *************************************************************/
/******************************************************************************\
* CModul																	   *
\******************************************************************************/
template<class TInput, class TOutput> 
CModul<TInput, TOutput>::CModul()
{
	/* Initialize everything with zeros */
	iMaxOutputBlockSize = 0;
	iInputBlockSize = 0;
	iOutputBlockSize = 0;
	pvecInputData = NULL;
	pvecOutputData = NULL;
}

template<class TInput, class TOutput> 
void CModul<TInput, TOutput>::ProcessDataThreadSave(CParameter& Parameter)
{
	/* Get a lock for the resources */
	Lock();

	/* Call processing routine of derived modul */
	ProcessDataInternal(Parameter);

	/* Unlock resources */
	Unlock();
}

template<class TInput, class TOutput> 
void CModul<TInput, TOutput>::InitThreadSave(CParameter& Parameter)
{
	/* Get a lock for the resources */
	Lock();

	try
	{
		/* Call init of derived modul */
		InitInternal(Parameter);

		/* Unlock resources */
		Unlock();
	}

	catch (CGenErr)
	{
		/* Unlock resources */
		Unlock();

		/* Throws the same error again which was send by the function */
		throw;
	}
}

template<class TInput, class TOutput> 
void CModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init some internal variables */
	iInputBlockSize = 0;

	/* Call init of derived modul */
	InitThreadSave(Parameter);
}

template<class TInput, class TOutput> 
void CModul<TInput, TOutput>::Init(CParameter& Parameter, 
								   CBuffer<TOutput>& OutputBuffer)
{
	/* Init some internal variables */
	iMaxOutputBlockSize = 0;
	iInputBlockSize = 0;
	iOutputBlockSize = 0;

	/* Call init of derived modul */
	try { InitThreadSave(Parameter); }
	catch (CGenErr) { throw; }


	/* Init output transfer buffer */
	if (iMaxOutputBlockSize != 0)
		OutputBuffer.Init(iMaxOutputBlockSize);
	else
		if (iOutputBlockSize != 0)
			OutputBuffer.Init(iOutputBlockSize);
}


/******************************************************************************\
* Transmitter modul (CTransmitterModul)										   *
\******************************************************************************/
template<class TInput, class TOutput> 
CTransmitterModul<TInput, TOutput>::CTransmitterModul()
{
	/* Initialize all member variables with zeros */
	iInputBlockSize2 = 0;
	pvecInputData2 = NULL;
}

template<class TInput, class TOutput> 
void CTransmitterModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init some internal variables */
	iInputBlockSize2 = 0;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput> 
void CTransmitterModul<TInput, TOutput>::Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* Init some internal variables */
	iInputBlockSize2 = 0;

	/* Init base-class */
	try { CModul<TInput, TOutput>::Init(Parameter, OutputBuffer); }
	catch (CGenErr) { throw; }
}

template<class TInput, class TOutput> 
void CTransmitterModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Query vector from output transfer-buffer for writing */
		pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		ProcessDataInternal(Parameter);
	
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}
}

template<class TInput, class TOutput> 
_BOOLEAN CTransmitterModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Check, if enough input data is available */
		if (InputBuffer.GetFillLevel() < iInputBlockSize)
		{
			/* Set request flag */
			InputBuffer.SetRequestFlag(TRUE);

			return FALSE;
		}

		/* Get vector from transfer-buffer */
		pvecInputData = InputBuffer.Get(iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from vectors */
		(*pvecOutputData).SetExData((*pvecInputData).GetExData());

		/* Call the underlying processing-routine */
		ProcessDataInternal(Parameter);
	
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}

	return TRUE;
}

template<class TInput, class TOutput> 
void CTransmitterModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TInput>& InputBuffer2, CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Check, if enough input data is available from all sources */
		if (InputBuffer.GetFillLevel() < iInputBlockSize)
		{
			/* Set request flag */
			InputBuffer.SetRequestFlag(TRUE);

			return;
		}
		if (InputBuffer2.GetFillLevel() < iInputBlockSize2)
		{
			/* Set request flag */
			InputBuffer2.SetRequestFlag(TRUE);

			return;
		}
	
		/* Get vectors from transfer-buffers */
		pvecInputData = InputBuffer.Get(iInputBlockSize);
		pvecInputData2 = InputBuffer2.Get(iInputBlockSize2);

		/* Query vector from output transfer-buffer for writing */
		pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		ProcessDataInternal(Parameter);
	
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::
	ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Read data and write it in the transfer-buffer.
		   Query vector from output transfer-buffer for writing */
		pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		ProcessDataInternal(Parameter);
		
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}
}

template<class TInput, class TOutput> 
_BOOLEAN CTransmitterModul<TInput, TOutput>::
	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter */
	/* Check, if enough input data is available */
	if (InputBuffer.GetFillLevel() < iInputBlockSize)
	{
		/* Set request flag */
		InputBuffer.SetRequestFlag(TRUE);

		return FALSE;
	}

	/* Get vector from transfer-buffer */
	pvecInputData = InputBuffer.Get(iInputBlockSize);

	/* Call the underlying processing-routine */
	ProcessDataInternal(Parameter);

	return TRUE;
}

/******************************************************************************\
* Receiver modul (CReceiverModul)											   *
\******************************************************************************/
template<class TInput, class TOutput> 
CReceiverModul<TInput, TOutput>::CReceiverModul()
{
	/* Initialize all member variables with zeros */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	pvecOutputData2 = NULL;
	pvecOutputData3 = NULL;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;
	bResetBuf3 = FALSE;
	bDoInit = FALSE;
}

template<class TInput, class TOutput> void 
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput> void 
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* Init flag */
	bResetBuf = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
}

template<class TInput, class TOutput> void 
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer, CBuffer<TOutput>& OutputBuffer2)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
}

template<class TInput, class TOutput> void 
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer, CBuffer<TOutput>& OutputBuffer2, CBuffer<TOutput>& OutputBuffer3)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;
	bResetBuf3 = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);

	if (iMaxOutputBlockSize3 != 0)
		OutputBuffer3.Init(iMaxOutputBlockSize3);
	else
		if (iOutputBlockSize3 != 0)
			OutputBuffer3.Init(iOutputBlockSize3);
}

template<class TInput, class TOutput> 
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TOutput>& OutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE 
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we 
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;

		return TRUE;
	}

	/* Special case if input block size is zero */
	if (iInputBlockSize == 0)
	{
		InputBuffer.Clear();

		return FALSE;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		pvecInputData = InputBuffer.Get(iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from vectors */
		(*pvecOutputData).SetExData((*pvecInputData).GetExData());

		/* Call the underlying processing-routine */
		ProcessDataThreadSave(Parameter);
	
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(iOutputBlockSize);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput> 
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TOutput>& OutputBuffer, CBuffer<TOutput>& OutputBuffer2)
{
	/* Check initialization flag. The initialization must be done OUTSIDE 
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we 
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2);

		/* Reset init flag */
		bDoInit = FALSE;

		return TRUE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		pvecInputData = InputBuffer.Get(iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		
		/* Call the underlying processing-routine */
		ProcessDataThreadSave(Parameter);
	
		/* Write processed data from internal memory in transfer-buffers */
		OutputBuffer.Put(iOutputBlockSize);
		OutputBuffer2.Put(iOutputBlockSize2);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		if (bResetBuf2 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = FALSE;
			OutputBuffer2.Clear();
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput> 
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer, CBuffer<TOutput>& OutputBuffer, CBuffer<TOutput>& OutputBuffer2, CBuffer<TOutput>& OutputBuffer3)
{
	/* Check initialization flag. The initialization must be done OUTSIDE 
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we 
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2, OutputBuffer3);

		/* Reset init flag */
		bDoInit = FALSE;

		return TRUE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		pvecInputData = InputBuffer.Get(iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		pvecOutputData3 = OutputBuffer3.QueryWriteBuffer();
		
		/* Call the underlying processing-routine */
		ProcessDataThreadSave(Parameter);
	
		/* Write processed data from internal memory in transfer-buffers */
		OutputBuffer.Put(iOutputBlockSize);
		OutputBuffer2.Put(iOutputBlockSize2);
		OutputBuffer3.Put(iOutputBlockSize3);

		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		if (bResetBuf2 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = FALSE;
			OutputBuffer2.Clear();
		}
		if (bResetBuf3 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf3 = FALSE;
			OutputBuffer3.Clear();
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput> 
void CReceiverModul<TInput, TOutput>::
	ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE 
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we 
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;

		return;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Query vector from output transfer-buffer for writing */
	pvecOutputData = OutputBuffer.QueryWriteBuffer();

	/* Call the underlying processing-routine */
	ProcessDataThreadSave(Parameter);

	/* Write processed data from internal memory in transfer-buffer */
	OutputBuffer.Put(iOutputBlockSize);

	/* Reset output-buffers if flag was set by processing routine */
	if (bResetBuf == TRUE)
	{
		/* Reset flag and clear buffer */
		bResetBuf = FALSE;
		OutputBuffer.Clear();
	}
}

template<class TInput, class TOutput> 
_BOOLEAN CReceiverModul<TInput, TOutput>::
	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE 
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we 
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter);

		/* Reset init flag */
		bDoInit = FALSE;

		return TRUE;
	}

	/* Special case if input block size is zero and buffer, too */
	if ((InputBuffer.GetFillLevel() == 0) && (iInputBlockSize == 0))
	{
		InputBuffer.Clear();
		return FALSE;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		pvecInputData = InputBuffer.Get(iInputBlockSize);
	
		/* Call the underlying processing-routine */
		ProcessDataThreadSave(Parameter);
	}

	return bEnoughData;
}

#endif // !defined(AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_)
