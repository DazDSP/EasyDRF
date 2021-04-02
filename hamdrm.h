/******************************************************************************\
 * Author(s):
 *  Francesco Lanza
 * Description:
 *  DLL spec HAM-DRM for Windows operating system
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

#include <windows.h>


#define mode_A 0
#define mode_B 1
#define mode_E 2

#define specocc_23 0
#define specocc_25 1

#define mscprot_norm 0
#define mscprot_low 1

#define qam_4  0
#define qam_16 1
#define qam_64 2

#define interleave_short 0
#define interleave_long 1


//********************************************************************


	// If you get something different than 0, something went the wrong way
	// 1 -> Receiver thread dead on Fatal
	// 2 -> Transmitter thread dead on Fatal
	// 3 -> Receiver thread startup failed
	// 4 -> Transmitter thread startup failed
	__declspec(dllexport) int __cdecl getFatalErr(void);

	// Initialize File Path (max 200 char)
	__declspec(dllexport) void __cdecl SetRXFileSavePath(char * PathToSaveRXFile);
	__declspec(dllexport) void __cdecl SetRXCorruptSavePath(char * PathToCorruptRXFile);
	__declspec(dllexport) void __cdecl SetBSRPath(char * PathToBSR);
  
	// Parameters for TX
	__declspec(dllexport) void __cdecl SetParams(int mode, int specocc, int mscprot, int qam, int interleave);
	__declspec(dllexport) void __cdecl SetCall(char * CallSign);  // max length 9 char !
	__declspec(dllexport) void __cdecl SetDCFreq(int DcFreq);	  // 350 recommended
	__declspec(dllexport) void __cdecl SetStartDelay(int delay);  // def value 9, long-lead-in 16

	// Parameters for RX (call at 400ms intervals max)
	__declspec(dllexport) boolean __cdecl GetParams(char * mode, char * specocc, char * mscprot, 
													char * qam, char * interleave);
	__declspec(dllexport) boolean __cdecl GetCall(char * CallSign);  // max length 9 char !


	// Get Soundcard Configuration
	// Separate In/Out enumeration (NEW)
	__declspec(dllexport) int	 __cdecl GetAudNumDevIn();
	__declspec(dllexport) char * __cdecl GetAudDeviceNameIn(int ID);
	__declspec(dllexport) void	 __cdecl SetAudDeviceIn(int ID);
	__declspec(dllexport) int	 __cdecl GetAudNumDevOut();
	__declspec(dllexport) char * __cdecl GetAudDeviceNameOut(int ID);
	__declspec(dllexport) void	 __cdecl SetAudDeviceOut(int ID);

	// Set Serial Device number for PTT 
	__declspec(dllexport) void	 __cdecl SetCommDevice(int dev);
	__declspec(dllexport) void	 __cdecl SetPTT(int on);

	// File transfer 
	__declspec(dllexport) boolean __cdecl SetFileTX(char * FileName, char * Dir_and_FileName, int inst = 1);  
		// 1 to 4 allowed for instance parameter
	__declspec(dllexport) boolean __cdecl GetFileRX(char * FileName); 
		// bsr requests go to root directory
	__declspec(dllexport) boolean __cdecl GetCorruptFileRX(char * FileName);  
	__declspec(dllexport) boolean __cdecl GetPercentTX(int * piccnt,int * percent); 
	__declspec(dllexport) void	  __cdecl GetSegPosTX(int * tot, int * act);
	__declspec(dllexport) boolean __cdecl GetActSegm(char * data);
	__declspec(dllexport) int	  __cdecl GetLastTID();
	
	// BSR Routines
	// Calculate BSR Request
	__declspec(dllexport) boolean __cdecl GetBSR(int * numbsrsegments, char * namebsrfile); 
		// use SendBSR to send request
		// numbsrsegments -> reports number of segments missing (this is an output!)
		// namebsrfile -> reports name of file (if known, this is an output!) 
	// Send BSR (NEW)
	__declspec(dllexport) boolean __cdecl SendBSR(int inst = 1, int fast = 0);
		// inst -> number of instances, 0 disables multiple inst on few seg
		// fast -> fast = 1 is not compatible to older versions
	// Answer BSR Request
	__declspec(dllexport) boolean __cdecl readthebsrfile(char* fnam,int * segno);					
		// reads file bsrreq.bin, returns filename. 
		// returns number of missing segments in bsr request
		// if false then the bsr is for another station
	__declspec(dllexport) void   __cdecl writebsrselsegments(int inst = 1);	
		// writes selected segments to tx buffer, start tx after this.
		// 1 to 4 allowed for instance parameter
		// auto sends 2 instance if (noseg < 50), 3 if (noseg < 10), 4 if (noseg < 3)

	// Threads. only start once.
	__declspec(dllexport) void __cdecl StartThreadRX(int AudDev);
	__declspec(dllexport) void __cdecl StartThreadTX(int AudDev);
	__declspec(dllexport) void __cdecl StopThreads();  

	// Start/Stop DRM routines
	__declspec(dllexport) void	  __cdecl ControlTX(boolean SetON);
	__declspec(dllexport) void	  __cdecl ControlRX(boolean SetON);
	__declspec(dllexport) void    __cdecl ResetRX(void);

	// Get data for display (array length 250 elements, call at 400ms intervals max except Spectrum 100ms)
	__declspec(dllexport) int  __cdecl GetSpectrum(float * data);	// 500 bins
	__declspec(dllexport) int  __cdecl GetSPSD(float * data);
	__declspec(dllexport) int  __cdecl GetTF(float * data, float * gddata);
	__declspec(dllexport) int  __cdecl GetIR(float*lb,float*hb,float*sg,float*eg,float*pb,float*pe,float*data);
	__declspec(dllexport) int  __cdecl GetFAC(float * datax,float * datay);
	__declspec(dllexport) int  __cdecl GetMSC(float * datax,float * datay);

	__declspec(dllexport) int  __cdecl GetSNR();
	__declspec(dllexport) int  __cdecl GetLevel();
	__declspec(dllexport) int  __cdecl GetDCFreq();
	__declspec(dllexport) int  __cdecl GetState(int * states);  
	__declspec(dllexport) void __cdecl GetData(int * totsize,int * actsize,int * actpos);  


//********************************************************************




