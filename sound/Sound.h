/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See Sound.cpp
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

#if !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
#define AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_
#define WIN32_LEAN_AND_MEAN        
#include <windows.h>
#include <mmsystem.h>

#include "../common/GlobalDefinitions.h"
#include "../common/Vector.h"


/* Definitions ****************************************************************/
#define	NUM_IN_OUT_CHANNELS		2	 //was 1 DM	/* Stereo recording (but we only use one channel for recording) */ //Edited DM was 1 ======================================================
#define	BITS_PER_SAMPLE			16		/* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE		2		/* Number of bytes per sample */

/* Set this number as high as we have to prebuffer symbols for one MSC block. In case of robustness mode D we have 24 symbols */
#define NUM_SOUND_BUFFERS_IN	20		/* Number of sound card buffers */ //Edited DM 

#define NUM_SOUND_BUFFERS_OUT	4 		/* Number of sound card buffers */

/* Maximum number of recognized sound cards installed in the system */
#define MAX_NUMBER_SOUND_CARDS	10


/* Classes ********************************************************************/
class CSound
{
public:
	CSound();
	virtual ~CSound();

	void		InitRecording(int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE);
	void		InitPlayback(int iNewBufferSize, _BOOLEAN bNewBlocking = FALSE);
	_BOOLEAN	Read(CVector<short>& psData);
	_BOOLEAN	Write(CVector<short>& psData);
	_BOOLEAN	IsEmpty(void);

	int			GetNumDevIn() {return iNumDevsIn;};
	string		GetDeviceNameIn(int iDiD) {return pstrDevicesIn[iDiD];};
	int			GetNumDevOut() {return iNumDevsOut;};
	string		GetDeviceNameOut(int iDiD) {return pstrDevicesOut[iDiD];};
	void		SetInDev(int iNewDev);
	void		SetOutDev(int iNewDev);

	void		Close();

protected:
	void		OpenInDevice();
	void		OpenOutDevice();
	void		PrepareInBuffer(int iBufNum);
	void		PrepareOutBuffer(int iBufNum);
	void		AddInBuffer();
	void		AddOutBuffer(int iBufNum);
	void		GetDoneBuffer(int& iCntPrepBuf, int& iIndexDoneBuf);

	WAVEFORMATEX	sWaveFormatEx;
	UINT			iNumDevsIn;
	UINT			iNumDevsOut;
	string			pstrDevicesIn[MAX_NUMBER_SOUND_CARDS];
	string			pstrDevicesOut[MAX_NUMBER_SOUND_CARDS];
	UINT			iCurInDev;
	UINT			iCurOutDev;
	BOOLEAN			bChangDevIn;
	BOOLEAN			bChangDevOut;

	/* Wave in */
	WAVEINCAPS		m_WaveInDevCaps;
	HWAVEIN			m_WaveIn;
	HANDLE			m_WaveInEvent;
	WAVEHDR			m_WaveInHeader[NUM_SOUND_BUFFERS_IN];
	int				iBufferSizeIn;
	int				iWhichBufferIn;
	short*			psSoundcardBuffer[NUM_SOUND_BUFFERS_IN];
	_BOOLEAN		bBlockingRec;

	/* Wave out */
	WAVEOUTCAPS		m_WaveOutDevCaps;
	int				iBufferSizeOut;
	HWAVEOUT		m_WaveOut;
	short*			psPlaybackBuffer[NUM_SOUND_BUFFERS_OUT];
	WAVEHDR			m_WaveOutHeader[NUM_SOUND_BUFFERS_OUT];
	HANDLE			m_WaveOutEvent;
	_BOOLEAN		bBlockingPlay;
};


#endif // !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
