/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Daz Man 2021
 * 
 * Description:
 *	Global definitions
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

#if !defined(DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include <complex>
using namespace std; /* Because of the library: "complex" */
#include <string>
#include <stdio.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "tables/TableDRMGlobal.h"


/* Definitions ****************************************************************/
/* When you define the following flag, a directory called
   "test" MUST EXIST in the windows directory (or linux directory if you use
   Linux)! */
#define _DEBUG_
#undef _DEBUG_

#ifndef VERSION
# define VERSION "25.1" //"25.0" //"24.1" //updated DM
# define BUILD __DATE__ " " __TIME__ //added DM
#endif


#ifdef _WIN32 /* For Windows set flags here, otherwise it is set by configure */
/* Define whether using GUI or non-GUI receiver */
//# define USE_QT_GUI
# undef USE_QT_GUI

/* Activate or disable faad2 library (AAC decoding) */
//# define USE_FAAD2_LIBRARY
//# undef USE_FAAD2_LIBRARY

//# define HAVE_JOURNALINE
# undef HAVE_JOURNALINE

//# define HAVE_LIBFREEIMAGE
# undef HAVE_LIBFREEIMAGE
#endif


/* Choose algorithms -------------------------------------------------------- */
/* There are two algorithms available for frequency offset estimation for
   tracking mode: Using frequency pilots or the guard-interval correlation. In
   case of guard-interval correlation (which will be chosen if this macro is
   defined), the Hilbert filter in TimeSync must be used all the time -> more
   CPU usage. Also, the frequency tracking range is smaller */
#undef USE_FRQOFFS_TRACK_GUARDCORR

/* The frequency offset estimation can be done using the frequency pilots or the
   movement of the estimated impulse response. Defining this macro will enable
   the frequency pilot based estimation. Simulations showed that this method is
   more vulnerable to bad channel situations */
#undef USE_SAMOFFS_TRACK_FRE_PIL



/* Define the application specific data-types ------------------------------- */
typedef	double							_REAL;
typedef	complex<_REAL>					_COMPLEX;
typedef short							_SAMPLE;
typedef unsigned char					_BYTE;
typedef bool							_BOOLEAN;

// bool seems not to work with linux TODO: Fix Me!
typedef unsigned char/*bool*/			_BINARY;

#if defined(_WIN32)
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
#else
#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# else
typedef unsigned long long uint64_t;
typedef unsigned long uint32_t;
typedef unsigned int uint16_t;
# endif
#endif
#endif
typedef uint64_t						_UINT64BIT;
typedef uint32_t						_UINT32BIT;
typedef uint16_t						_UINT16BIT;

/* Define type-specific information */
#define SIZEOF__BYTE					8
#define _MAXSHORT						32767
#define _MAXREAL						((_REAL) 3.4e38) /* Max for float */


/* Definitions for window message system ------------------------------------ */
typedef unsigned int					_MESSAGE_IDENT;
#define MS_FAC_CRC						1	/* MS: Message */
#define MS_MSC_CRC						2
#define MS_FRAME_SYNC					3
#define MS_TIME_SYNC					4
#define MS_IOINTERFACE					5
#define MS_RESET_ALL					6
#define MS_MOT_OBJ_STAT					7
#define MS_FREQ_FOUND					8 //added DM 

#define GUI_CONTROL_UPDATE_TIME			500	/* Milliseconds */

/* Definitions for LED/status indicators DM ----------------------------------*/
#define RNEIPINK						0xBD80FF
#define DARKRED							0x000070
#define RED								0x0000FF
#define ORANGE							0x0090FF
#define YELLOW							0x00DEFF
#define GREEN							0x30FF00
#define BLUE							0xFF7000
#define	GREY							0x707070
#define DARKGREEN						0x207010
#define MAGENTA							0xFF00FF
#define BLACK							0x000000

/* Definitions for filestat DM ----------------------------------*/
#define FS_BLANK						0
#define FS_WAIT							1
#define FS_TRY							2
#define FS_SAVED						3
#define FS_FAILED						4

/* Global enumerations ------------------------------------------------------ */
enum ESpecOcc {SO_0, SO_1}; /* SO: Spectrum Occupancy */
enum ERobMode {RM_ROBUSTNESS_MODE_A, RM_ROBUSTNESS_MODE_B, RM_ROBUSTNESS_MODE_E}; /* RM: Robustness Mode */


/* Constants ---------------------------------------------------------------- */
const _REAL crPi = ((_REAL) 3.14159265358979323846);


/* Define a number for the case: log10(0), which would lead to #inf */
#define RET_VAL_LOG_0					((_REAL) -200.0)


/* Standard definitions */
#define	TRUE							1
#define FALSE							0


/* Classes ********************************************************************/
/* For metric */
class CDistance
{
public:
	/* Distance towards 0 or towards 1 */
	_REAL rTow0;
	_REAL rTow1;
};

/* Viterbi needs information of equalized received signal and channel */
class CEquSig
{
public:
	_COMPLEX	cSig; /* Actual signal */
	_REAL		rChan; /* Channel power at this cell */
};

/* Mutex object to access data safely from different threads */
/* QT mutex */
#ifdef USE_QT_GUI
#include <qthread.h>
class CMutex
{
public:
	void Lock() {Mutex.lock();}
	void Unlock() {Mutex.unlock();}

protected:
	QMutex Mutex;
};
#else
/* No GUI and no threads, we do not need mutex in this case */
class CMutex
{
public:
	void Lock() {}
	void Unlock() {}
};
#endif

class CGenErr
{
public:
	CGenErr(string strNE) : strError(strNE) {}
	string strError;
};

/* Prototypes for global functions ********************************************/
/* Posting a window message */
void PostWinMessage(const _MESSAGE_IDENT MessID, const int iMessageParam = 0);

/* Debug error handling */
void DebugError(const char* pchErDescr, const char* pchPar1Descr, const double dPar1, const char* pchPar2Descr,	const double dPar2);

void ErrorMessage(string strErrorString);


/* Global functions ***********************************************************/
/* Converting _REAL to _SAMPLE */
inline _SAMPLE Real2Sample(const _REAL rInput)
{
	/* Lower bound */
	if (rInput < -_MAXSHORT)
		return -_MAXSHORT;

	/* Upper bound */
	if (rInput > _MAXSHORT)
		return _MAXSHORT;

	return (_SAMPLE) rInput;
}


#endif // !defined(DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
