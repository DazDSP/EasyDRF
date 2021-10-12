/******************************************************************************\
 * Copyright (c) 2020
 *
 * Author(s):
 *	Daz Man
 *
 * Description:
 *	RS-defs.h - Definitions for RS coding routines and associated code
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

#if !defined(RSDEF_)
#define RSDEF_ TRUE
#define WIN32_LEAN_AND_MEAN
extern int LeadIn; //Moved from ..? DM
extern string EZHeaderID; //header ID string
extern int EncFileSize; //Encoder current file size
extern int EncSegSize; //Encoder current segment size
extern int DecFileSize; //Decoder current file size
extern int totsize; //Decoder total segment count global
extern int actsize; //Decoder active segment count global
extern int actpos; //Decoder active position global
extern int DecSegSize; //Decoder current segment size
extern int DecTotalSegs;
extern int CompTotalSegs;
extern int HdrFileSize;
extern bool CRCOK;

extern int BarLastID;
extern int BarLastSeg;
extern int BarLastTot;

extern int DecCheckReg; //Serial decoder check register
extern int SerialFileSize; //Serial register for file size transmission
extern int BarTransportID; //Last bargraph transport ID
extern int DecTransportID; //Current decoder transport ID
extern int DecPacketID; //Current decoder Packet ID - TEST
extern int RSlastTransportID; //Last decoded Transport ID
extern int RxRSlevel;	//added by DM to detect RS encoding on incoming file segments, even if file header fails - Used in DABMOT.cpp
extern int RSError;
extern int RSfilesize;
extern int DecVecbSize;
extern int CompTotalSegs;

extern char erasures[3][8192 / 8];
extern int erasuressegsize[3];
extern int erasureswitch; //which array is being written to currently DM
extern int erasureindex;
extern int erasureflags;

extern int DecPrevSeg;
extern int DecHighSeg;

extern int DMRSindex;
extern int DMRSpsize;

extern char DMfilename[260];

extern char DMdecodestat[15];

extern int DMmodehash;

extern int DMspeechmodecount;

extern int dcomperr; //decompressor error
extern int RSbusy;
extern int lasterror; //save RS error count

#define ZLIB_WINAPI
#define ZLIB_DLL
#define ZLIB_INTERNAL
#include "zconf.h"
#include "zlib.h"
#include "Logging.h"
#include "LzmaLib.h"
#include "common/RS/RS-coder.h"
#include <thread>

#endif
