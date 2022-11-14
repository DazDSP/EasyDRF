/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *  Daz Man 2021
 *
 * Description:
 *	Audio source encoder/decoder (Speech Codecs)
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
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 1111
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "AudioSourceDecoder.h"
#include "../speex/speex.h"

/* Implementation *************************************************************/
/******************************************************************************\
*/

//LPC_10
struct lpc10_e_state *es;
struct lpc10_d_state *ds;

//short wavdata[LPC10_SAMPLES_PER_FRAME]; //Array for Codec input
short wavdata[maxLPC10_SAMPLES_PER_FRAME*2]; //Array for Codec input - does this need to be x2 as well? (for bytes value)
unsigned char encbytes[LPC10_BITS_IN_COMPRESSED_FRAME + 2]; //Array for Codec output
unsigned char chksum[130];


int lpcblocksT = 0;
int lpcsumT = 27; //LPC-10 CRC is always 27 bits now - this always divides evenly into the 54 bit data size
int lpciterT = 0;
int upsampleT = 0;

int lpcblocksR = 0;
int lpcsumR = lpcsumT;
int lpciterR = 0;
int upsampleR = 0;
int size1 = 0;

int LPC10_SAMPLES_PER_FRAME = 180; //default on Mode B, QAM16, normal protection, 2.5kHz

//SPEEX
SpeexBits encbits;
SpeexBits decbits;
void *enc_state;
void *dec_state;
int speex_frame_size;
float spinp[160];
char spoutp[20];
char spoutpb[60];
int nbBytes;

float audioamp = 0; //for mike compressor
float noisegate = 0;

/*
* Encoder                                                                      *
\******************************************************************************/
void CAudioSourceEncoder::ProcessDataInternal(CParameter& TransmParam)
{
	signed int i = 0, j = 0, k = 0;

	/* The speech CODEC goes here --------------------------------- */
	/* Here is the recorded data. Now, the DRM encoder must be put in to encode this material and generate the _BINARY output data */

	if (bIsDataService == FALSE)
	{
		if (iInputBlockSize > 0) {
			//Audio Compressor and Noisegate DM 2022
			//Compress audio slightly for better volume
			//Add noisegating to reduce noise
			//Make the noisegating multiband TODO
			for (i = 0; i < iInputBlockSize; i++) {
				int samp = (*pvecInputData)[i]; //get a sample
				float amp = (float)abs(samp); //rectify audio
				audioamp = max(max((audioamp * 0.99998f), amp), 512.0f); //recovery Tc.. add some attack smoothing later?
				float ng = min((amp * 100.0f), 32767.0f); //add gain and limit for noisegate
				noisegate = max((float)(noisegate * 0.9999), ng); //smooth noisegate control level
				float gain = ((float)30000.0f / audioamp); //compute gain for some slight headroom
				(*pvecInputData)[i] = (samp * gain) * noisegate / 32767;  //compress and noisegate
			}
		}

		// avail space:
		// 2.25khz, msc=0 1920 bits PER SEC
		// 2.25khz, msc=1 2400 bits PER SEC
		// 2.50khz, msc=0 2208 bits PER SEC
		// 2.50khz, msc=1 2760 bits PER SEC

		//Needs at least 2500bps data rate to work (~1040 bits per 400mS DRM frame)
		//Also, the codec always uses the SAME data rate regardless of the DRM data rate used.
		//So higher DRM data rates will NOT sound any better!
		//Use Mode B, 2.5kHz QAM16, normal protection

		//NEW - Adaptive LPC blocks, to work in QAM4 modes... Expect poorer quality at lower AND higher rates

		if (TransmParam.Service[0].AudioParam.eAudioCoding == CParameter::AC_LPC)
		{
			//Make sure we have a destination to write to...
			if (iOutputBlockSize > 0) {

				//copy short data into vector
				//read 19200 words (16 bits)
				for (k = 0; k < iInputBlockSize; k++) {
					speechIN[k] = (*pvecInputData)[k]; //copy directly
				}

				//Make sure these variables are current
				//compute data sizes from iOutputBlockSize
				if (iOutputBlockSize > 0) {
					LPC10_SAMPLES_PER_FRAME = max(min((180 / max(lpcblocksT, 6)) * 18, maxLPC10_SAMPLES_PER_FRAME),180); //adapt to available bandwidth DM
					lpcblocksT = (((iOutputBlockSize / SIZEOF__BYTE) * SIZEOF__BYTE) - lpcsumT) / LPC10_BITS_IN_COMPRESSED_FRAME; //how many 54 bit blocks
					lpciterT = (lpcblocksT * LPC10_BITS_IN_COMPRESSED_FRAME) / lpcsumT;
					upsampleT = 35000 / (LPC10_SAMPLES_PER_FRAME * lpcblocksT);

					//if the sizes change, reinit the buffer
					int LPFDecSize = lpcblocksT * LPC10_SAMPLES_PER_FRAME;
					if (speechLPFDec.GetSize() != LPFDecSize) speechLPFDec.Init(LPFDecSize);
				}

				i = 0;
				speechLPFDec = FIRFracDec(rvecS, speechIN, speechLPFDec, rvecZS); //antialias filter and fractional decimation
			
				k = 0;
				int t = 0;
				for (i = 0; i < lpcblocksT; i++) { //blocks
					for (j = 0; j < LPC10_SAMPLES_PER_FRAME; j++) //Read LPC10_SAMPLES_PER_FRAME sample groups
					{
						t = speechLPFDec[i * LPC10_SAMPLES_PER_FRAME + j];
						if (t >= 32700) t = 32700; //clip to prevent overflow
						if (t <= -32700) t = -32700;
						wavdata[j] = (short)t;
					}
					//lpc10_bit_encode(wavdata, encbytes, es); //old lib name
					lpc10_encode(wavdata, encbytes, es);
					for (k = 0; k < LPC10_BITS_IN_COMPRESSED_FRAME; k++) {
						(*pvecOutputData)[i * LPC10_BITS_IN_COMPRESSED_FRAME + k] = encbytes[k]; //copy the encoded data DM
					}
				}

				//Make 27 bit checksum DM new
				for (i = 0; i < lpcsumT; i++)
					chksum[i] = 0;
				for (k = 0; k < lpciterT; k++)
				{
					for (i = 0; i < lpcsumT; i++)
						chksum[i] ^= (*pvecOutputData)[(k * lpcsumT) + i];
				}
				for (i = 0; i < lpcsumT; i++)
					(*pvecOutputData)[lpcblocksT * LPC10_BITS_IN_COMPRESSED_FRAME + i] = chksum[i];

			}
		}
#define WRITELPC 0
#if WRITELPC == 1
				//save LPC-10 data to a file for compression testing
				//open file (every 400mS)
				FILE* set = nullptr;
				if ((set = fopen("LPC-test.bin", "a+b")) == nullptr) {
				}
				else {
					//if it opened OK...
					//append to file
					unsigned int a = 0;
					unsigned int b = 0;
					for (k = 0; k < 122; k++) {
						a = 0; //clear byte
						//take 1 bit for each index and pack them into a byte
						for (i = 0; i < 8; i++) {
							a |= (*pvecOutputData)[(k * 8) + i] << i;
						}
						//write the byte
						putc(a, set);
					}
					//close file
					fclose(set); //file is closed here - but only if it was opened
				}
#endif //WRITELPC == 1

#define WRITELPCBYTES 0
#if WRITELPCBYTES == 1
				//save LPC-10 data to a file for compression testing
				//open file (every 400mS)
				FILE* set = nullptr;
				if ((set = fopen("LPC-test.bin", "a+b")) == nullptr) {
				}
				else {
					//if it opened OK...
					//append to file
					unsigned int a = 0;
					unsigned int b = 0;
					for (k = 0; k < (LPC10_BITS_IN_COMPRESSED_FRAME * lpcblocksT); k++) {
						a |= (*pvecOutputData)[k];

						//write the byte
						putc(a, set);
					}
					//close file
					fclose(set); //file is closed here - but only if it was opened
				}
#endif //WRITELPCBYTES == 1


		if (TransmParam.Service[0].AudioParam.eAudioCoding == CParameter::AC_SPEEX)
		{
			if (iOutputBlockSize > 1040) { //don't try to encode Speex in QAM4 because the buffers will overrun DM
				//SPEEX CODEC
				// source:
				// 19200 at 48000 sample/sec = 3200 at 8000samples/sec
				// 3200 / 160 = 20 audio blocks, 0 byte spare
				// destination
				// 20 audio blocks * 6 bytes = 120 bytes = 960 bits

				//copy the audio input
				//read 19200 words (16 bits)
				for (k = 0; k < iInputBlockSize; k++) {
					speechIN[k] = (*pvecInputData)[k]; //copy directly
				}

				//Lowpass filter and decimate the audio input to 4kHz DM
				speechLPFDecSpeex = FIRFiltDec(rvecS, speechIN, speechLPFDecSpeex, rvecZS); //4kHz antialias filter

				//speechLPFDecSpeex now contains the filtered and downsampled audio input

				for (i = 0; i < 20; i++) //20 blocks 
				{
					speex_bits_reset(&encbits);
					for (k = 0; k < 160; k++)
					{
						//read in 160 samples to the spinp buffer
						spinp[k] = (float)speechLPFDecSpeex[(i * 160) + k];

					}
					speex_encode(enc_state, spinp, &encbits);
					nbBytes = speex_bits_write(&encbits, spoutp, 20);

					// 20 audio blocks, each 48 bits long = 6 bytes
					for (k = 0; k < 8; k++)
					{
						char ak = (1 << k);
						spoutpb[k] = ((spoutp[0] & ak) != 0);
						spoutpb[k + 8] = ((spoutp[1] & ak) != 0);
						spoutpb[k + 16] = ((spoutp[2] & ak) != 0);
						spoutpb[k + 24] = ((spoutp[3] & ak) != 0);
						spoutpb[k + 32] = ((spoutp[4] & ak) != 0);
						spoutpb[k + 40] = ((spoutp[5] & ak) != 0);
					}
					for (k = 0; k < 48; k++)
						(*pvecOutputData)[i * 48 + k] = spoutpb[k];
				}

				//Make 15 bit Checksum, 64 iterations
				for (i = 0; i < 15; i++)
					chksum[i] = 0;
				for (k = 0; k < 64; k++)
				{
					for (i = 0; i < 15; i++)
						chksum[i] ^= (*pvecOutputData)[k * 15 + i];
				}
				for (i = 0; i < 15; i++)
					(*pvecOutputData)[960 + i] = chksum[i];

				for (i = 975; i < iOutputBlockSize; i++) //pad 975 to full
					(*pvecOutputData)[i] = 0;

			}
		}
		if (TransmParam.Service[0].AudioParam.eAudioCoding == CParameter::AC_CELP)
		{
			for (i = 0; i < iOutputBlockSize; i++) //pad 0 to full
			{
				(*pvecOutputData)[i] = 0;
			}
		}
		//AC_SSTV mode is not used DM
		if (TransmParam.Service[0].AudioParam.eAudioCoding == CParameter::AC_SSTV)
		{
			int len = iOutputBlockSize - 24;
			if (bUsingTextMessage) len -= 32;

			//getbuf(len,pvecOutputData);

			//Make 24 bit Checksum
			for (i = 0; i < 24; i++) chksum[i] = 0;
			k = 0;
			while (k < len)
			{
				if (len - k <= 24)
					for (i = 0; i < len - k; i++) chksum[i] ^= (*pvecOutputData)[k + i];
				else
					for (i = 0; i < 24; i++) chksum[i] ^= (*pvecOutputData)[k + i];
				k += 24;
			}

			//Write checksum
			for (i = 0; i < 24; i++)
				(*pvecOutputData)[len + i] = chksum[i];
		}

		BlockSize = iOutputBlockSize; //debug DM

		/* Text message application. Last four bytes in stream are written */
		//if ((bUsingTextMessage == TRUE) && (iOutputBlockSize > 975 + 32))
		if ((bUsingTextMessage == TRUE) && (iOutputBlockSize > 1030)) //This will not normally execute in modes below QAM16, 2.5kHz
		{
			/* Always four bytes for text message "piece" */
			CVector<_BINARY> vecbiTextMessBuf(SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR);

			/* Get a "piece" */
			TextMessage.Encode(vecbiTextMessBuf);

			/* Total number of bytes which are actually used. The number is
			   specified by iLenPartA + iLenPartB which is set in
			   "SDCTransmit.cpp". There is currently no "nice" solution for
			   setting these values. TODO: better solution */
			   /* Padding to byte as done in SDCTransmit.cpp line 138ff */
			const int iTotByt = (iOutputBlockSize / SIZEOF__BYTE) * SIZEOF__BYTE;
			TextBytes = iTotByt; //debug DM
			TextBytesi = iTotByt - SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; //debug DM
			for (i = iTotByt - SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i < iTotByt; i++)
			{
				(*pvecOutputData)[i] = vecbiTextMessBuf[i - (iTotByt - SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR)];
			}
		}

	}
	/* ---------------------------------------------------------------------- */

	if (bIsDataService == TRUE)
	{
		// TODO: make a separate modul for data encoding
				/* Write data packets in stream */
		CVector<_BINARY> vecbiData;
		const int iNumPack = iOutputBlockSize / iTotPacketSize;
		int iPos = 0;

		for (int j = 0; j < iNumPack; j++)
		{
			/* Get new packet */
			DataEncoder.GeneratePacket(vecbiData);

			/* Put it on stream */
			for (i = 0; i < iTotPacketSize; i++)
			{
				(*pvecOutputData)[iPos] = vecbiData[i];
				iPos++;
			}
		}
	}
}


void CAudioSourceEncoder::InitInternal(CParameter& TransmParam)
{
	/* Define input and output block size */
	iOutputBlockSize = TransmParam.iNumDecodedBitsMSC; //
	iInputBlockSize = DEFiInputBlockSize / 2; //

	if (TransmParam.iNumDataService == 1)
	{
		bIsDataService = TRUE;
		iTotPacketSize = DataEncoder.Init(TransmParam); //
	}
	else
	{
		bIsDataService = FALSE;
		init_lpc10_encoder_state(es);

		//Init new resampling filter and buffers DM
		speechIN.Init(DEFiInputBlockSize / 2); //48kHz DM

		int LPFDecSize = lpcblocksT * LPC10_SAMPLES_PER_FRAME;
		if (LPFDecSize == 0) { LPFDecSize = 4000; } //default just to stop buffer overruns
		speechLPFDec.Init(LPFDecSize); //downsampled buffer for LPC-10

		speechLPFDecSpeex.Init(3200); //downsampled buffer for Speex is a fixed size of 3200 samples
		rvecS.Init(FILTER_TAP_NUMS); //48k rate decimation filter coeffs
		rvecZS.Init(FILTER_TAP_NUMS - 1, (CReal)0.0);
		for (int i = 0; i < FILTER_TAP_NUMS; i++) rvecS[i] = filter_tapsS[i]; //write 48kHz decimation antialias filter coeffs

	}
}

void CAudioSourceEncoder::SetTextMessage(const string& strText)
{
	/* Set text message in text message object */
	TextMessage.SetMessage(strText);

	/* Set text message flag */
	bUsingTextMessage = TRUE;
}

void CAudioSourceEncoder::ClearTextMessage()
{
	/* Clear all text segments */
	TextMessage.ClearAllText();

	/* Clear text message flag */
	bUsingTextMessage = FALSE;
}

CAudioSourceEncoder::CAudioSourceEncoder()
{
	int tmp = 0;
	// LPC_10
	es = create_lpc10_encoder_state();
	if (es == NULL) { printf("Couldn't allocate  encoder state.\n"); }
	init_lpc10_encoder_state(es);
	// SPEEX
	speex_bits_init(&encbits);
	enc_state = speex_encoder_init(&speex_nb_mode);
	speex_encoder_ctl(enc_state,SPEEX_GET_FRAME_SIZE,&speex_frame_size);
	tmp = 10; speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &tmp);
	tmp = 2400; speex_encoder_ctl(enc_state, SPEEX_SET_BITRATE, &tmp);
}

CAudioSourceEncoder::~CAudioSourceEncoder()
{
	// LPC_10
	destroy_lpc10_encoder_state (es);
	// SPEEX
	speex_bits_destroy(&encbits);
	speex_encoder_destroy(enc_state);
}

/******************************************************************************\
* Decoder                                                                      *
\******************************************************************************/

float oldwavsample = 0.0;
int succdecod = 0;
int errdecod = 0;
int percentage = 0;

int CAudioSourceDecoder::getdecodperc(void)
{
	double tot = 0;
	double fact = 0;
	tot = (double)succdecod + (double)errdecod; //edited DM added (double)
	if (tot == 0.0) return 0;
	fact = 100.0/tot;
	percentage = (int)(succdecod*fact);
	return percentage;
}

void CAudioSourceDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i = 0, k = 0;

	/* Check if something went wrong in the initialization routine */
	if (DoNotProcessData == TRUE)
	{
		errdecod++;
		return;
	}

	/* Text Message ***********************************************************/
	/* Total frame size depends on whether text message is used or not */
	FrameSize = iTotalFrameSize; //for debugging DM
	if ((bTextMessageUsed == TRUE) && (iTotalFrameSize > 975 + 32))
	{
		/* Decode last four bytes of input block for text message */
		for (i = 0; i < SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
			vecbiTextMessBuf[i] = (*pvecInputData)[iTotalFrameSize + i];

		TextMessage.Decode(vecbiTextMessBuf);
	}

	/* Check if LPC should be decoded */
	if (IsLPCAudio == TRUE)
	{
		bool lpccrc = TRUE;
		iOutputBlockSize = 0;


		//LPC-10 in all modes ====================================================================================
		//First, make sure all the variables are current
		//compute LPC data sizes
		if (iInputBlockSize > 0) {
			LPC10_SAMPLES_PER_FRAME = max(min((180 / max(lpcblocksR, 6)) * 18, maxLPC10_SAMPLES_PER_FRAME), 180); //adapt to available bandwidth DM
			lpciterR = (lpcblocksR * LPC10_BITS_IN_COMPRESSED_FRAME) / lpcsumR;
			upsampleR = 35000 / (LPC10_SAMPLES_PER_FRAME * lpcblocksR);

			//if the sizes change, reinit the buffers
			int LPFDecSize = lpcblocksR * LPC10_SAMPLES_PER_FRAME;
			if (speechLPFDec.GetSize() != LPFDecSize) speechLPFDec.Init(LPFDecSize);
			size1 = upsampleR * LPC10_SAMPLES_PER_FRAME * lpcblocksR;
			if (speechLPFDec2.GetSize() != size1) speechLPFDec2.Init(size1);


			//Make N bit checksum DM
			for (i = 0; i < lpcsumR; i++)
				chksum[i] = 0; //clear checksum array
			for (k = 0; k < lpciterR; k++)
			{
				for (i = 0; i < lpcsumR; i++) {
					int addr = (k * lpcsumR) + i;
					chksum[i] ^= (*pvecInputData).at(addr);
				}
			}
			for (i = 0; i < lpcsumR; i++)
				lpccrc &= ((*pvecInputData).at(lpcblocksR * LPC10_BITS_IN_COMPRESSED_FRAME + i) == chksum[i]); //Make sure the checksum matches DM

				//lpccrc = TRUE; //Unmute for testing DM

			if (lpccrc)
			{  // lpcblocks at 54 bytes encbytes
				int j = 0;
				int w = 0;
				//float tmp = 0.0;
				for (i = 0; i < lpcblocksR; i++)
				{
					for (j = 0; j < LPC10_BITS_IN_COMPRESSED_FRAME; j++) encbytes[j] = (*pvecInputData)[i * LPC10_BITS_IN_COMPRESSED_FRAME + j]; //move bits into decoder input buffer DM
					//lpc10_bit_decode(encbytes, wavdata, ds);
					lpc10_decode(encbytes, wavdata, ds);

					//Gather all samples into the new buffer DM
					//for each pass, add another block of samples
					for (j = 0; j < LPC10_SAMPLES_PER_FRAME; j++) //edited DM
					{
						speechLPFDec[w++] = wavdata[j] * 0.7; //fill the buffer and scale level DM
					}

				}

				//an integer upsample to a large rate seems to reduce aliasing from resampler timing jitter
				for (j = 0; j < size1; j++) //
				{
					speechLPFDec2[j] = speechLPFDec[j / upsampleR]; //just copy samples
				}

				speechLPFDec2 = Filter(rvecS2, rvecA, speechLPFDec2, rvecZS2); //antialias filter 1

				for (j = 0; j < speechLPF.GetSize(); j++) //
				{
					speechLPF[j] = 0.0; //erase buffer for zero stuffing
				}

				const float stepsize = (float)(DEFiInputBlockSize / 2) / size1;
				for (j = 0; j < size1; j++) //
				{
					speechLPF[j * stepsize] = speechLPFDec2[j]; //fill the buffer DM
				}

				speechLPF = Filter(rvecS, rvecA, speechLPF, rvecZS); //antialias filter 2

				//copy data
				iOutputBlockSize = DEFiInputBlockSize; //buffer is full and in stereo
				for (j = 0; j < iOutputBlockSize / 2; j++)
				{
					(*pvecOutputData)[j * 2] = speechLPF[j]; //copy all data in stereo
					(*pvecOutputData)[j * 2 + 1] = speechLPF[j]; //copy all data in stereo
				}

				bAudioIsOK = TRUE;
				succdecod++;
			}
			else
			{
				bAudioIsOK = FALSE;
				iOutputBlockSize = 0;
				errdecod++;
			}
		}
	}
	else if ((IsSPEEXAudio == TRUE) && (iInputBlockSize > 1030)) //only try to decode if there are enough bits DM
	{
		bool spxcrc = TRUE;
		int j = 0;
		int ind = 0;
		iOutputBlockSize = 0;

		if (iTotalFrameSize >= 990)
		{
			//Make 15 bit Checksum, 64 iterations
			for (i = 0; i < 15; i++)
				chksum[i] = 0;
			for (k = 0; k < 64; k++)
			{
				for (i = 0; i < 15; i++)
					chksum[i] ^= (*pvecInputData)[k * 15 + i];
			}
			for (i = 0; i < 15; i++)
				spxcrc &= ((*pvecInputData)[960 + i] == chksum[i]);
		}

		/* Reset bit extraction access */
		//(*pvecInputData).ResetBitAccess();

		// 20 audio blocks, each 48 bits long = 6 bytes
		if (spxcrc)
		{
			for (i = 0; i < 20; i++)
			{

				ind = i * 48;
				for (j = 0; j < 48; j++)
					spoutpb[j] = (*pvecInputData)[ind + j];
				for (j = 0; j < 6; j++)
				{
					ind = j * 8;
					spoutp[j] = spoutpb[ind++];
					spoutp[j] += spoutpb[ind++] << 1;
					spoutp[j] += spoutpb[ind++] << 2;
					spoutp[j] += spoutpb[ind++] << 3;
					spoutp[j] += spoutpb[ind++] << 4;
					spoutp[j] += spoutpb[ind++] << 5;
					spoutp[j] += spoutpb[ind++] << 6;
					spoutp[j] += spoutpb[ind++] << 7;
				}

				try
				{
					speex_bits_read_from(&decbits, spoutp, 6);
					speex_decode(dec_state, &decbits, spinp);
				}
				catch (...) //modified DM
				{
					bAudioIsOK = FALSE;
					iOutputBlockSize = 0;
					return;
				}
				for (j = 0; j < 160; j++)
				{
					//gather all samples into the buffer first DM
					speechLPF[((i * 160) + j) * 6] = spinp[j] * 1.1; //fill the buffer and scale level DM
					speechLPF[((i * 160) + j) * 6 + 1] = 0; //zero stuff DM
					speechLPF[((i * 160) + j) * 6 + 2] = 0;
					speechLPF[((i * 160) + j) * 6 + 3] = 0;
					speechLPF[((i * 160) + j) * 6 + 4] = 0;
					speechLPF[((i * 160) + j) * 6 + 5] = 0;
				}
			}
			//Antialias filter the decoded audio at 4kHz DM
			speechLPF = Filter(rvecS, rvecA, speechLPF, rvecZS);

			//copy data
			iOutputBlockSize = DEFiInputBlockSize; //buffer is full and in stereo
			for (j = 0; j < iOutputBlockSize / 2; j++)
			{
				(*pvecOutputData)[j * 2] = speechLPF[j]; //copy all data in stereo
				(*pvecOutputData)[j * 2 + 1] = speechLPF[j]; //copy all data in stereo
			}

			bAudioIsOK = TRUE;
			succdecod++;
		}
		else
		{
			bAudioIsOK = FALSE;
			iOutputBlockSize = 0;
			errdecod++;
		}
	}
	else if (IsSSTVData == TRUE)
	{
		bool sstvcrc = TRUE;
		int len = iTotalFrameSize - 24;
		if (bTextMessageUsed) len -= 32;


		//Make 24 bit Checksum
		for (i = 0; i < 24; i++) chksum[i] = 0;
		k = 0;
		while (k < len)
		{
			if (len - k <= 24)
				for (i = 0; i < len - k; i++) chksum[i] ^= (*pvecOutputData)[k + i];
			else
				for (i = 0; i < 24; i++) chksum[i] ^= (*pvecOutputData)[k + i];
			k += 24;
		}
		for (i = 0; i < 24; i++)
			sstvcrc &= ((*pvecInputData)[len + i] == chksum[i]);

		if (sstvcrc)
		{
			//putbuf(len,pvecOutputData);
			bAudioIsOK = TRUE;
		}
		else
			bAudioIsOK = FALSE;
		iOutputBlockSize = 0;
	}
	else if (IsCELPAudio == TRUE)
	{
		bAudioIsOK = FALSE;
		iOutputBlockSize = 0;
	}
	else
	{
		bAudioIsOK = FALSE;
		iOutputBlockSize = 0;
	}

	if (bAudioIsOK) PostWinMessage(MS_MSC_CRC, 0);
	else if (bAudioWasOK) PostWinMessage(MS_MSC_CRC, 1);
	else PostWinMessage(MS_MSC_CRC, 2);
	bAudioWasOK = bAudioIsOK;

}

void CAudioSourceDecoder::InitInternal(CParameter& ReceiverParam)
{
/*
	Since we use the exception mechanism in this init routine, the sequence of
	the individual initializations is very important!
	Requirement for text message is "stream is used" and "audio service".
	Requirement for AAC decoding are the requirements above plus "audio coding
	is AAC"
	* NOTE: AAC is not used in WinDRM/EasyDRF - DM
*/
	int iCurAudioStreamID = 0;
	int iCurSelServ = 0;

	succdecod = 0;
	errdecod = 0;

	try
	{
		/* Init error flags and output block size parameter. The output block
		   size is set in the processing routine. We must set it here in case
		   of an error in the initialization, this part in the processing
		   routine is not being called */
		IsLPCAudio = FALSE;
		IsSPEEXAudio = FALSE;
		IsCELPAudio = FALSE;
		IsSSTVData = FALSE;
		DoNotProcessData = FALSE;

		iOutputBlockSize = DEFiInputBlockSize; //Surely the buffer only needs to be 38400 samples long per 400mS?? This is 96000 samples per second in stereo... so 48000 each channel at 48kHz DM

		/* Get number of total input bits for this module */
		iInputBlockSize = ReceiverParam.iNumAudioDecoderBits; //moved here

		lpcblocksR = (iInputBlockSize - lpcsumR) / LPC10_BITS_IN_COMPRESSED_FRAME; //how many 54 bit blocks

		//Decoder filters
		speechLPF.Init(DEFiInputBlockSize / 2); //speech buffer is 19200 samples

		int LPFDecSize = lpcblocksR * LPC10_SAMPLES_PER_FRAME;
		if (LPFDecSize == 0) { LPFDecSize = 4000; } //default just to stop buffer overruns
		speechLPFDec.Init(LPFDecSize); //this buffer is variable for LPC-10 in various modes

		size1 = upsampleR * LPC10_SAMPLES_PER_FRAME * lpcblocksR;
		if (size1 == 0) { size1 = 20000; } //default just to stop buffer overruns
		speechLPFDec2.Init(size1); //larger buffer for upsampling


		rvecS.Init(FILTER_TAP_NUMS); //
		rvecZS.Init(FILTER_TAP_NUMS - 1, (CReal)0.0);
		rvecS2.Init(FILTER_TAP_NUMS2); //
		rvecZS2.Init(FILTER_TAP_NUMS2 - 1, (CReal)0.0);
		for (int i = 0; i < FILTER_TAP_NUMS; i++) rvecS[i] = filter_tapsS[i]; //write 48kHz decimation antialias filter coeffs
		for (int i = 0; i < FILTER_TAP_NUMS2; i++) rvecS2[i] = filter_tapsS2[i]; //

		/* Only FIR filter */
		rvecA.Init(1);
		rvecA[0] = (CReal)1.0;

		/* Get number of total input bits for this module */
		//iInputBlockSize = ReceiverParam.iNumAudioDecoderBits; //moved higher up

		/* Get current selected audio service */
		iCurSelServ = ReceiverParam.GetCurSelAudioService();

		/* Current audio stream ID */
		iCurAudioStreamID = ReceiverParam.Service[iCurSelServ].AudioParam.iStreamID;

		/* The requirement for this module is that the stream is used and the
		   service is an audio service. Check it here */
		if ((ReceiverParam.Service[iCurSelServ].eAudDataFlag != CParameter::SF_AUDIO) || (iCurAudioStreamID == STREAM_ID_NOT_USED))
		{
			throw CInitErr(ET_ALL);
		}

		/* Init text message application ------------------------------------ */
		switch (ReceiverParam.Service[iCurSelServ].AudioParam.bTextflag)
		{
		case TRUE:
			bTextMessageUsed = TRUE;

			/* Get a pointer to the string */
			TextMessage.Init(&ReceiverParam.Service[iCurSelServ].AudioParam.strTextMessage);

			/* Total frame size is input block size minus the bytes for the text message */
			iTotalFrameSize = iInputBlockSize - SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR;

			/* Init vector for text message bytes */
			vecbiTextMessBuf.Init(SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR);
			break;

		case FALSE:
			bTextMessageUsed = FALSE;

			/* All bytes are used for AAC data, no text message present */
			iTotalFrameSize = iInputBlockSize;
			break;
		}

		//added to disable speech decoding in P mode (Picture mode) - This can stop exceptions happening in the speech decoder DM
		if (runmode != 'P') {
		/* Init for decoding -------------------------------------------- */
		if (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioCoding == CParameter::AC_LPC)
			IsLPCAudio = TRUE;
		if (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioCoding == CParameter::AC_SPEEX)
			IsSPEEXAudio = TRUE;
		if (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioCoding == CParameter::AC_CELP)
			IsCELPAudio = TRUE;
		if (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioCoding == CParameter::AC_SSTV)
			IsSSTVData = TRUE;
		}


	}

	catch (CInitErr CurErr)
	{
		switch (CurErr.eErrType)
		{
		case ET_ALL:
			/* An init error occurred, do not process data in this module */
			DoNotProcessData = TRUE;
			break;

		case ET_AAC:
			/* AAC part should not be decdoded, set flag */
			IsCELPAudio = FALSE;
			IsSPEEXAudio = FALSE;
			IsLPCAudio = FALSE;
			IsSSTVData = FALSE;
			break;

		default:
			DoNotProcessData = TRUE;
		}
	}
}

CAudioSourceDecoder::CAudioSourceDecoder()
{
	int enhon = 1;
	// LPC_10
	ds = create_lpc10_decoder_state();
	if (ds == NULL) { printf("Couldn't allocate  decoder state.\n"); }
	init_lpc10_decoder_state(ds);
	// SPEEX
	speex_bits_init(&decbits);
	dec_state = speex_decoder_init(&speex_nb_mode);
	speex_decoder_ctl(dec_state, SPEEX_SET_ENH, &enhon);
}

CAudioSourceDecoder::~CAudioSourceDecoder()
{
	// LPC_10
	destroy_lpc10_decoder_state (ds);
	// SPEEX
	speex_bits_destroy(&decbits);
	speex_decoder_destroy(dec_state);
}
