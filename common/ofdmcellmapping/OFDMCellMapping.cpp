/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Mapping of the symbols on the carriers
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

#include "OFDMCellMapping.h"
#include "../Tables/TableCarrier.h"

/* Implementation *************************************************************/
/******************************************************************************\
* OFDM cells mapping														   *
\******************************************************************************/
void COFDMCellMapping::ProcessDataInternal(CParameter& TransmParam)
{
	int iCar = 0; //inits DM
	int iMSCCounter = 0; //inits DM
	int iFACCounter = 0; //inits DM
	int iDummyCellCounter = 0; //inits DM
	int iSymbolCounterAbs;

	/* Mapping of the data and pilot cells on the OFDM symbol --------------- */
	/* Set absolute symbol position */
	iSymbolCounterAbs =
		TransmParam.iFrameIDTransm * iNumSymPerFrame + iSymbolCounter;

	/* Init temporary counter */
	iDummyCellCounter = 0;
	iMSCCounter = 0;
	iFACCounter = 0;

	for (iCar = 0; iCar < iNumCarrier; iCar++)
	{

		/* MSC */
		if (_IsMSC(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			if (iMSCCounter >= TransmParam.veciNumMSCSym[iSymbolCounterAbs])
			{
				/* Insert dummy cells */
				(*pvecOutputData)[iCar] = pcDummyCells[iDummyCellCounter];

				iDummyCellCounter++;
			}
			else
				(*pvecOutputData)[iCar] = (*pvecInputData)[iMSCCounter];
				
			iMSCCounter++;
		}

		/* FAC */
		if (_IsFAC(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			(*pvecOutputData)[iCar] = (*pvecInputData2)[iFACCounter];
				
			iFACCounter++;
		}

		/* DC carrier */
		if (_IsDC(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
			(*pvecOutputData)[iCar] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);

		/* Pilots */
		if (_IsPilot(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
			(*pvecOutputData)[iCar] = 
				TransmParam.matcPilotCells[iSymbolCounterAbs][iCar];
	}

	/* Increase symbol-counter and wrap if needed */
	iSymbolCounter++;
	if (iSymbolCounter == iNumSymPerFrame)
	{
		iSymbolCounter = 0;

		/* Increase frame-counter (ID) (Used also in FAC.cpp) */
		TransmParam.iFrameIDTransm++;
		if (TransmParam.iFrameIDTransm == NUM_FRAMES_IN_SUPERFRAME)
			TransmParam.iFrameIDTransm = 0;
	}

	/* Set absolute symbol position (for updated relative positions) */
	iSymbolCounterAbs =
		TransmParam.iFrameIDTransm * iNumSymPerFrame + iSymbolCounter;

	/* Set input block-sizes for next symbol */
	iInputBlockSize = TransmParam.veciNumMSCSym[iSymbolCounterAbs];
	iInputBlockSize2 = TransmParam.veciNumFACSym[iSymbolCounterAbs];
}

void COFDMCellMapping::InitInternal(CParameter& TransmParam)
{
	iNumSymPerFrame = TransmParam.iNumSymPerFrame;
	iNumCarrier = TransmParam.iNumCarrier;

	/* Init symbol-counter */
	iSymbolCounter = 0;

	/* Init frame ID */
	TransmParam.iFrameIDTransm = 0;

	/* Choose right dummy cells for MSC QAM scheme */
	switch (TransmParam.eMSCCodingScheme)
	{
	case CParameter::CS_2_SM:
		pcDummyCells = (_COMPLEX*) &cDummyCells16QAM[0];
		break;

	case CParameter::CS_3_SM:
		pcDummyCells = (_COMPLEX*) &cDummyCells64QAM[0];
		break;
	}

	/* Define block-sizes for input and output of the module ---------------- */
	iInputBlockSize = TransmParam.veciNumMSCSym[0]; /* MSC */
	iInputBlockSize2 = TransmParam.veciNumFACSym[0]; /* FAC */
	iOutputBlockSize = TransmParam.iNumCarrier; /* Output */
}


/******************************************************************************\
* OFDM cells demapping														   *
\******************************************************************************/
void COFDMCellDemapping::ProcessDataInternal(CParameter& ReceiverParam)
{
	int iCar = 0; //inits DM
	int iMSCCounter = 0; //inits DM
	int iFACCounter = 0; //inits DM
	int iSymbolCounterAbs = 0; //inits DM
	int iNewSymbolCounter = 0; //inits DM
	int iNewFrameID = 0; //inits DM

	/* Set absolute symbol position */
	iSymbolCounterAbs = iCurrentFrameID * iNumSymPerFrame + iSymbolCounter;

	/* Set output block-sizes for this symbol */
	iOutputBlockSize = ReceiverParam.veciNumMSCSym[iSymbolCounterAbs];
	iOutputBlockSize2 = ReceiverParam.veciNumFACSym[iSymbolCounterAbs];

	/* Demap data from the cells */
	iMSCCounter = 0;
	iFACCounter = 0;
	for (iCar = 0; iCar < iNumCarrier; iCar++)
	{
		/* MSC */
		if (_IsMSC(ReceiverParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			/* Ignore dummy cells */
			if (iMSCCounter < ReceiverParam.veciNumMSCSym[iSymbolCounterAbs])
			{
				(*pvecOutputData)[iMSCCounter] = (*pvecInputData)[iCar];

				iMSCCounter++; /* Local counter */
				iMSCCellCounter++; /* Super-frame counter */
			}
		}

		/* FAC */
		if (_IsFAC(ReceiverParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			(*pvecOutputData2)[iFACCounter] = (*pvecInputData)[iCar];

			iFACCounter++; /* Local counter */
			iFACCellCounter++; /* Super-frame counter */
		}

	}

	/* Get symbol-counter for next symbol and adjust frame-ID. Use the extended
	   data, shipped with the input vector */
	iNewSymbolCounter = (*pvecInputData).GetExData().iSymbolID + 1;

	/* Check range (iSymbolCounter must be in {0, ... , iNumSymPerFrame - 1} */
	while (iNewSymbolCounter >= iNumSymPerFrame)
		iNewSymbolCounter -= iNumSymPerFrame;
	while (iNewSymbolCounter < 0)
		iNewSymbolCounter += iNumSymPerFrame;

	/* Increment internal symbol counter and take care of wrap around */
	iSymbolCounter++;
	if (iSymbolCounter == iNumSymPerFrame)
		iSymbolCounter = 0;

	/* Check if symbol counter has changed (e.g. due to frame synchronization
	   unit). Reset all buffers in that case to avoid buffer overflow */
	if (iSymbolCounter != iNewSymbolCounter)
	{
		/* Init symbol counter with new value and reset all output buffers */
		iSymbolCounter = iNewSymbolCounter;

		SetBufReset1();
		SetBufReset2();
		SetBufReset3();
		iMSCCellCounter = 0;
		iFACCellCounter = 0;
	}

	/* If frame bound is reached, update frame ID from FAC stream */
	if (iSymbolCounter == 0)
	{
		/* Check, if number of FAC cells is correct. If not, reset
		   output cyclic-buffer. An incorrect number of FAC cells can be if
		   the "iSymbolCounterAbs" was changed, e.g. by the synchronization
		   units */
		if (iFACCellCounter != NUM_FAC_CELLS)
			SetBufReset2(); /* FAC: buffer number 2 */

		/* Reset FAC cell counter */
		iFACCellCounter = 0;

		/* Frame ID of this FAC block stands for the "current" block. We need
		   the ID of the next block, therefore we have to add "1" */
		iNewFrameID = ReceiverParam.iFrameIDReceiv + 1;
		if (iNewFrameID == NUM_FRAMES_IN_SUPERFRAME)
			iNewFrameID = 0;

		/* Increment internal frame ID and take care of wrap around */
		iCurrentFrameID++;
		if (iCurrentFrameID == NUM_FRAMES_IN_SUPERFRAME)
			iCurrentFrameID = 0;

		/* Check if frame ID has changed, if yes, reset output buffers to avoid
		   buffer overflows */
		if (iCurrentFrameID != iNewFrameID)
		{
			iCurrentFrameID = iNewFrameID;

			/* Only MSC depend on frame ID */
			SetBufReset1(); /* MSC: buffer number 1 */
			iMSCCellCounter = 0;
		}

		if (iCurrentFrameID == 0)
		{
			/* Super-frame bound reached, test cell-counters (same as with the
			   FAC cells, see above) */
			if (iMSCCellCounter != iNumUsefMSCCellsPerFrame *
				NUM_FRAMES_IN_SUPERFRAME)
			{
				SetBufReset1(); /* MSC: buffer number 1 */
			}

			/* Reset counters */
			iMSCCellCounter = 0;
		}
	}
}

void COFDMCellDemapping::InitInternal(CParameter& ReceiverParam)
{
	iNumSymPerFrame = ReceiverParam.iNumSymPerFrame;
	iNumCarrier = ReceiverParam.iNumCarrier;
	iNumUsefMSCCellsPerFrame = ReceiverParam.iNumUsefMSCCellsPerFrame;

	/* Init symbol-counter and internal frame counter */
	iSymbolCounter = 0;
	iCurrentFrameID = 0;

	/* Init cell-counter */
	iMSCCellCounter = 0;
	iFACCellCounter = 0;

	/* Define block-sizes for input and output of the module ---------------- */
	/* Input */
	iInputBlockSize = iNumCarrier;

	/* Define space for output cyclic buffers. We must consider enough headroom
	   otherwise the buffers could overrun */
	/* FAC, one block is exactly finished when last symbol with FAC cells is
	   processed */
	iMaxOutputBlockSize2 = NUM_FAC_BITS_PER_BLOCK;
	/* MSC, since the MSC logical frames must not end at the end of one symbol
	   (could be somewhere in the middle of the symbol), the output buffer must
	   accept more cells than one logical MSC frame is long. The worst case is
	   that the block ends right at the beginning of one symbol; in this case we
	   have an overhang of approximately one symbol of MSC cells */
	iMaxOutputBlockSize = ReceiverParam.iNumUsefMSCCellsPerFrame +
		ReceiverParam.iMaxNumMSCSym;
}
