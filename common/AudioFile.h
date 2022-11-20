/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
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

#if !defined(AUDIOFILE_H__FD6B234594328533_80UWFB06C2AC__INCLUDED_)
#define AUDIOFILE_H__FD6B234594328533_80UWFB06C2AC__INCLUDED_

#include "GlobalDefinitions.h"


/* Classes ********************************************************************/
class CWaveFile
{
public:
	CWaveFile() : pFile(nullptr), iBytesWritten(0) {}
	virtual ~CWaveFile() {if (pFile != nullptr) Close();}

	void Open(const string strFileName)
	{
		if (pFile != nullptr)
			Close();

		const CWaveHdr WaveHeader =
		{
			/* Use always stereo and PCM */
			{'R', 'I', 'F', 'F'}, 0, {'W', 'A', 'V', 'E'},
			{'f', 'm', 't', ' '}, 16, 1, 2, REAL_SOUNDCRD_SAMPLE_RATE,
			SOUNDCRD_SAMPLE_RATE * 4 /* same as block align */,
			4 /* block align */, 16,
			{'d', 'a', 't', 'a'}, 0
		};

		pFile = fopen(strFileName.c_str(), "wb");
		if (pFile != nullptr)
		{
			iBytesWritten = sizeof(CWaveHdr);
			fwrite((const void*) &WaveHeader, size_t(sizeof(CWaveHdr)), size_t(1), pFile);
		}
	}

	void AddStereoSample(const _SAMPLE sLeft, const _SAMPLE sRight)
	{
		if (pFile != nullptr)
		{
			iBytesWritten += 2 * sizeof(_SAMPLE);
			fwrite((const void*) &sLeft, size_t(2), size_t(1), pFile);
			fwrite((const void*) &sRight, size_t(2), size_t(1), pFile);
		}
	}

	void Close()
	{
		if (pFile != nullptr)
		{
			const _UINT32BIT iFileLength = iBytesWritten - 8;
			fseek(pFile, 4 /* offset */, SEEK_SET /* origin */);
			fwrite((const void*) &iFileLength, size_t(4), size_t(1), pFile);

			const _UINT32BIT iDataLength = iBytesWritten - sizeof(CWaveHdr);
			fseek(pFile, 40 /* offset */, SEEK_SET /* origin */);
			fwrite((const void*) &iDataLength, size_t(4), size_t(1), pFile);

			fclose(pFile);
		}
	}

protected:
	struct CWaveHdr
	{
		/* Wave header struct */
		char cMainChunk[4]; /* "RIFF" */
		_UINT32BIT length; /* Length of file */
		char cChunkType[4]; /* "WAVE" */
		char cSubChunk[4]; /* "fmt " */
		_UINT32BIT cLength; /* Length of cSubChunk (always 16 bytes) */
		_UINT16BIT iFormatTag; /* waveform code: PCM */
		_UINT16BIT iChannels; /* Number of channels */
		_UINT32BIT iSamplesPerSec; /* Sample-rate */
		_UINT32BIT iAvgBytesPerSec;
		_UINT16BIT iBlockAlign; /* Bytes per sample */
		_UINT16BIT iBitsPerSample;
		char cDataChunk[4]; /* "data" */
		_UINT32BIT iDataLength; /* Length of data */
	};

	FILE*		pFile;
	_UINT32BIT	iBytesWritten;
};


#endif // AUDIOFILE_H__FD6B234594328533_80UWFB06C2AC__INCLUDED_
