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
#define RS_SIZE_METHOD 1 //0 = old (7 segments RS datasize), 1 = new (4 segments RS datasize/255)

#define WIN32_LEAN_AND_MEAN

extern int ECCmode; //Used to be called LeadIn DM
extern string EZHeaderID; //header ID string
extern unsigned int EncFileSize; //Encoder current file size
extern unsigned int DecFileSize; //Decoder current file size
extern unsigned int HdrFileSize;
extern unsigned int SerialFileSize; //Serial register for file size transmission

extern int totsize; //Decoder total segment count global
extern int actsize; //Decoder active segment count global
extern int actpos; //Decoder active position global

extern unsigned int DecSegSize; //Decoder current segment size
extern unsigned int DecTotalSegs;
extern unsigned int CompTotalSegs;

extern bool CRCOK;

extern int BarLastID;
extern int BarLastSeg;
extern int BarLastTot;

extern unsigned int DecCheckReg; //Serial decoder check register
extern unsigned int BarTransportID; //Last bargraph transport ID
extern unsigned int DecTransportID; //Current decoder transport ID
extern unsigned int RSlastTransportID; //Last decoded Transport ID

extern unsigned int RxRSlevel; //added by DM to detect RS encoding on incoming file segments, even if file header fails - Used in DABMOT.cpp
extern unsigned int RxRSlevelold; //previous value of RxRSlevel
extern int RSError;
extern unsigned int RSfilesize;
extern unsigned int RSpercent;
extern bool RSsw;

extern char erasures[2][8192 / 8]; //was 3 x 1024, now 2 x 1024
extern int erasuressegsize[2]; //was 3, now 2

extern int DecPrevSeg;
extern int DecHighSeg;

extern int DMRSindex;
extern int DMRSpsize;

extern char DMfilename[260];
extern char DMdecodestat[15];

extern int DMmodehash;
extern bool DMnewfile;

//extern int DMspeechmodecount;

extern int dcomperr; //decompressor error
extern int lasterror; //save RS error count
extern int RSbusy;
extern unsigned int RScount; //save RS attempts count
extern unsigned int RSpsegs; //save RS segs on last attempt

extern unsigned char filestate; //file save status - 0=blank, 1=WAIT, 2=try..., 3=SAVED, 4=FAILED
extern unsigned char filestate2; //file save status - 0=blank, 1=WAIT, 2=try..., 3=SAVED, 4=FAILED - for cache clearing
extern char showgood; //stretch colour timing for SAVED (green) and FAILED (red)

extern char runmode;

#define SHOWCOL 10 //how long (in frames) to display green/red saved/failed colour for

#if USEGZIP
#define ZLIB_WINAPI
#define ZLIB_DLL
#define ZLIB_INTERNAL
#include "zlib.h"
#endif //USEGZIP

#include "zconf.h" //needed for some types...

#include "Logging.h"
#include "LzmaLib.h"
#include "common/RS/RS-coder.h"
#include <thread>

#endif
