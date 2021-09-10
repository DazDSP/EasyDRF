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
#include "../AudioFir.h"
#include "../libs/lpc10.h"
#include "../speex/speex.h"
//#include "../resample/Resample.cpp" //DM

/* Implementation *************************************************************/
/******************************************************************************\
*/

//#define DEFiInputBlockSize 38400 //define buffer size of transmit audio input buffer for 400mS @ 48000kHz stereo <-- Moved to Parameter.h
#define DEC_RATIO 6 //don't change this or the decimation code needs to be changed DM
#define DEFlpcResamplingArraySize DEFiInputBlockSize/DEC_RATIO/2 //this is the resampler to 8kHz
//#define DEFlpcResamplingArraySize DEFiInputBlockSize/DEC_RATIO //this is the resampler to 8kHz
#define DEFlpcPCM_BUFFSIZE 6400 //this is the buffer that feeds the LPC codec audio data in 180 byte chunks

//LPC10 audio buffers - Added by DM
//Make these global, so they persist (they don't get erased when the routine ends)
//Encoder Input
short LpcPcmInputArray[DEFlpcPCM_BUFFSIZE * 2]{ 0 }; //make a 4k buffer - this appears to require a BYTE count (*2) or else it overruns the buffer and corrupts the index pointers
//set the read index to zero
unsigned short int LpcPcmInputReadIndex = 0;
unsigned short int LpcPcmInputWriteIndex = 100;
//Decoder Output
short LpcPcmOutputArray[DEFlpcPCM_BUFFSIZE * 2]{ 0 }; //make a 4k buffer - this appears to require a BYTE count (*2) or else it overruns the buffer and corrupts the index pointers
//set the read index to zero
unsigned long int LpcPcmOutputReadIndex = 0;
unsigned long int LpcPcmOutputWriteIndex = 100;

//This is a bit reservior to make better use of the data bandwidth DM (this will require LPC bitrate adjustments for best results) TODO
char LpcBitsEncoderArray[4096]{ 0 }; //make a 4k buffer - this appears to require a BYTE count (*2) or else it overruns the buffer and corrupts the index pointers
unsigned short int LpcBitsEncoderReadIndex = 0;
unsigned short int LpcBitsEncoderWriteIndex = 0;

float audioAGC = 0.1; //AGC TODO
float audioGain = 1; //AGC TODO

//LPC_10
struct lpc10_e_state *es;
struct lpc10_d_state *ds;

//short wavdata[LPC10_SAMPLES_PER_FRAME]; //Array for Codec input
short wavdata[LPC10_SAMPLES_PER_FRAME*2]; //Array for Codec input - does this need to be x2 as well?
unsigned char encbytes[LPC10_BITS_IN_COMPRESSED_FRAME + 2]; //Array for Codec output
unsigned char chksum[130];

//Added DM
#define lpcblocks 17
#define lpcsum 20 //this is the new CRC/checksum size! (reduced to allow more audio bits)
#define lpcusedbits lpcblocks * LPC10_BITS_IN_COMPRESSED_FRAME

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

/*
* Encoder                                                                      *
\******************************************************************************/
void CAudioSourceEncoder::ProcessDataInternal(CParameter& TransmParam)
{
	signed int i,j,k;
	//short val;
	//signed int ival; //needs to not overflow on 16 bit audio summing DM
	float ival; //needs to not overflow on 16 bit audio summing DM
	short lpcResamplingArray[DEFlpcResamplingArraySize*2]; //

	/* The speech CODEC goes here --------------------------------- */
	/* Here is the recorded data. Now, the DRM encoder must be put in to encode this material and generate the _BINARY output data */

	if (bIsDataService == FALSE)
	{
		// avail space:
		// 2.25khz, msc=0 1920 bits PER SEC
		// 2.25khz, msc=1 2400 bits PER SEC
		// 2.50khz, msc=0 2208 bits PER SEC
		// 2.50khz, msc=1 2760 bits PER SEC

		//Needs at least 2500bps data rate to work (~1000 bits per 400mS DRM frame)
		//Also, the codec always uses the SAME data rate regardless of the DRM data rate used!
		//So higher DRM rates will NOT sound any better! Use Mode B, 2.5k QAM16, normal protection

		if (TransmParam.Service[0].AudioParam.eAudioCoding == CParameter::AC_LPC)
		{
			//LPC10 CODEC
			//DM
			//Rebuffering to manage the chunk size variations
			//Using a (global variable) circular buffer that is bigger than the maximum size needed
			//Resample the input buffer and read it all into the circular buffer
			//Then read it out again to the encoder, as many 180 sample groups as possible
			//Keep the unused samples in the buffer until the next frame, then read them out when possible
			//Also, add a bit reservior to the output to pack as many bits into the transmitted data as possible TODO
			//
			//Do resampling to 8kHz here - take 6 input samples and reduce them to 1
			//Process all 19200 samples into 3200 samples
			//Remove the original sample summing and use a proper IIR filter TODO
			//
			//The input buffer was 38400 bytes? Yeah, it was too big!
			//The soundcard buffer is stereo, but it's copied into this input buffer in MONO, so it needs to be HALF the size of the other buffers!
			//Data is mono 16 bit (ReadData converts it to mono)
			// 
			//How about a proper resampler? //TODO
			//Resample::Resample(pvecInputData[i], lpcResamplingArray[i]); //Not working DM
			//iOutputBlockSize = Resample(pvecInputData, pvecOutputData, (_REAL)SOUNDCRD_SAMPLE_RATE / (SOUNDCRD_SAMPLE_RATE - rSamRateOffset)); //Not working DM

			//Decimation by 6 - A proper IIR lowpass and/or a resampler would work better - For antialiasing it needs to be sharp DM
			i = 0;
			for (k = 0; k < iInputBlockSize;) //read 19200 words (16 bits) ===== CHANGED ==========================================
				{
					ival = (*pvecInputData)[k++];
					ival += (*pvecInputData)[k++];
					ival += (*pvecInputData)[k++];
					ival += (*pvecInputData)[k++];
					ival += (*pvecInputData)[k++];
					ival += (*pvecInputData)[k++];

					//Add some audio compression:  not working - this all needs to be signed arithmetic
//					if (audioAGC < (float)0.1) audioAGC = (float)0.1; //Add mike AGC Threshold
//					if (abs(ival) > audioAGC * 0.9) audioAGC = abs(ival); //Time constant
//					audioGain = (float)3000.0/audioAGC; //Compute gain
//					ival = ival * audioGain; //Apply gain correction

					lpcResamplingArray[i] = ival/6.0; //The downsampled PCM data DM
					i++;
				}

			//Now the entire 400mS input frame has been resampled to 8kHz DM
			//Next, buffer the audio so the audio input is fully read each pass
			//This means we need to read an average of 3200 times
			//But 180 won't divide evenly into 3200 so it will need to vary each time, depending on the relative pointer positions

			//First, write the 3200 new samples of data into the buffer
			for (i = 0; i < DEFlpcResamplingArraySize; i++) {
				LpcPcmInputArray[LpcPcmInputWriteIndex] = lpcResamplingArray[i]; //read in a sample
				LpcPcmInputWriteIndex++;
				if (LpcPcmInputWriteIndex > DEFlpcPCM_BUFFSIZE-1) {
					LpcPcmInputWriteIndex = 0; //wrap pointer at buffer end
				}
			}
			// source:
			// 19200 at 48000 sample/sec = 3200 at 8000samples/sec
			// 3200 / 180 = 17 or 18 audio blocks
			// LPC10 output: 54 bits * 17 = 918 bits audio.
			// LPC10 output: 54 bits * 18 = 972 bits audio.

			int lpccurrent = lpcblocks; // Number of blocks to process per 400mS DRM frame
			// Read 18 blocks if the unwrapped read/write pointer difference is at least 3240
			// If the write pointer is less than the read pointer, add the buffersize to it before finding the difference
			// otherwise, just find the difference
			// if the difference is more than 3240, use 18 reads, else use 17
			// This fixes most of the dropouts from the samplerate mismatch of the LPC10 codec
			// But ideally, the resampler code should be used for this
			// Also, sharper lowpass filters should be used for the decimation and interpolation to reduce aliasing
			i = LpcPcmInputWriteIndex;
			i = i-LpcPcmInputReadIndex;

			if (i < 0) { i = i + DEFlpcPCM_BUFFSIZE; }
			if (i >= 3240) {
				lpccurrent = 18; // Read 18 blocks instead
			}

			for (i = 0; i < lpccurrent; i++) { //Number of blocks to read
//				for (j = 0; j < 180; j++) //Read 180 sample groups
				for (j = 0; j < 160; j++) //Read 160 sample groups
				{
					wavdata[j] = LpcPcmInputArray[LpcPcmInputReadIndex]; //wavdata is a zero start index, but LpcPcmInputArray needs a persistant index pointer
					LpcPcmInputReadIndex++;
					if (LpcPcmInputReadIndex > DEFlpcPCM_BUFFSIZE-1) {
						LpcPcmInputReadIndex = 0; }
				}
				lpc10_bit_encode(wavdata, encbytes, es);
				for (k = 0; k < LPC10_BITS_IN_COMPRESSED_FRAME; k++) {
					(*pvecOutputData)[i * 54 + k] = encbytes[k]; //copy the encoded data DM
				}
			}

			//Add a bit reservior buffer here to maximize the coding efficiency.... TODO
			//read the bits into the buffer

			//LpcBitsEncoderArray
			//LpcBitsEncoderReadIndex;
			//LpcBitsEncoderWriteIndex;

			//but for now, just zero pad...
			//972 = 18 * 54 bits
			for (k = lpccurrent* LPC10_BITS_IN_COMPRESSED_FRAME; k < 972; k++)
				(*pvecOutputData)[k] = 0; //zero pad the extra buffer space DM

			//Make N bit checksum DM new (20 bits now)
			for (i = 0; i < lpcsum; i++)
				chksum[i] = 0;
			for (k = 0; k < lpcsum +1; k++)
			{
				for (i = 0; i < lpcsum; i++)
					chksum[i] ^= (*pvecOutputData)[k* lpcsum +i];
			}
			for (i = 0; i < lpcsum; i++)
				(*pvecOutputData)[18 * LPC10_BITS_IN_COMPRESSED_FRAME +i] = chksum[i];

			for (i = 18*54+lpcsum; i < iOutputBlockSize; i++) //zero pad output DM
				(*pvecOutputData)[i] = 0;
		}
		if (TransmParam.Service[0].AudioParam.eAudioCoding == CParameter::AC_SPEEX)
		{

			//SPEEX CODEC
			// source:
			// 19200 at 48000 sample/sec = 3200 at 8000samples/sec
			// 3200 / 160 = 20 audio blocks, 0 byte spare
			// destination
			// 20 audio blocks * 6 bytes = 120 bytes = 960 bits

			for (i = 0; i < 20; i++) //20 blocks 
			{
				speex_bits_reset(&encbits);
				int sct = i*960;
				for (k = 0; k < 160; k++)
				{
					ival  = (*pvecInputData)[sct++];
					ival += (*pvecInputData)[sct++];
					ival += (*pvecInputData)[sct++];
					ival += (*pvecInputData)[sct++];
					ival += (*pvecInputData)[sct++];
					ival += (*pvecInputData)[sct++];
					spinp[k]  = (float)ival / 6.0;
					//val  = (*pvecInputData)[i*960+k*6];
					//spinp[k] = (float)val;
				}
				speex_encode(enc_state, spinp, &encbits);
				nbBytes = speex_bits_write(&encbits, spoutp, 20);

				// 20 audio blocks, each 48 bits long = 6 bytes
				for (k = 0; k < 8; k++) 
				{
					char ak = (1 << k);
					spoutpb[k   ] = ((spoutp[0] & ak) != 0);
					spoutpb[k+ 8] = ((spoutp[1] & ak) != 0);
					spoutpb[k+16] = ((spoutp[2] & ak) != 0);
					spoutpb[k+24] = ((spoutp[3] & ak) != 0);
					spoutpb[k+32] = ((spoutp[4] & ak) != 0);
					spoutpb[k+40] = ((spoutp[5] & ak) != 0);
				}
				for (k = 0; k < 48; k++)  
					(*pvecOutputData)[i*48+k] = spoutpb[k];
			}

			//Make 15 bit Checksum, 64 iterations
			for (i = 0; i < 15; i++)
				chksum[i] = 0;
			for (k = 0; k < 64; k++)
			{
				for (i = 0; i < 15; i++)
					chksum[i] ^= (*pvecOutputData)[k* 15 +i];
			}
			for (i = 0; i < 15; i++)
				(*pvecOutputData)[960 +i] = chksum[i];
					
			for (i = 975; i < iOutputBlockSize; i++) //pad 975 to full
				(*pvecOutputData)[i] = 0;
			
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
					for (i = 0; i < len - k; i++) chksum[i] ^= (*pvecOutputData)[k+i]; 
				else
					for (i = 0; i < 24; i++) chksum[i] ^= (*pvecOutputData)[k+i]; 
				k += 24;
			}

			//Write checksum
			for (i = 0; i < 24; i++)
				(*pvecOutputData)[len+i] = chksum[i];
		}

		/* Text message application. Last four bytes in stream are written */
		if ((bUsingTextMessage == TRUE) && (iOutputBlockSize > 975 + 32))
		{
			/* Always four bytes for text message "piece" */
			CVector<_BINARY> vecbiTextMessBuf(
			SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR);
			
			/* Get a "piece" */
			TextMessage.Encode(vecbiTextMessBuf);

			/* Total number of bytes which are actually used. The number is
			   specified by iLenPartA + iLenPartB which is set in
			   "SDCTransmit.cpp". There is currently no "nice" solution for
			   setting these values. TODO: better solution */
			/* Padding to byte as done in SDCTransmit.cpp line 138ff */
			const int iTotByt =	(iOutputBlockSize / SIZEOF__BYTE) * SIZEOF__BYTE;
	
			for (i = iTotByt - SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR;i < iTotByt; i++)
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
	if (TransmParam.iNumDataService == 1)
	{
		bIsDataService = TRUE;
		iTotPacketSize = DataEncoder.Init(TransmParam);
	}
	else
	{
		bIsDataService = FALSE;
		init_lpc10_encoder_state(es);

		//NEW - Setup a global buffer for LPC10 encoder audio input (DM)
		//This buffer converts the 3200 word downsampled (8kHz) audio to 180 word chunks for the LPC10 codec
		//(because 180 doesn't divide into 3200 evenly)

		//We are starting a new transmission, so clear the buffer
		for (int i = 0; i < DEFlpcPCM_BUFFSIZE; i++)
			LpcPcmInputArray[i] = 0;
		
		//Decoder:
		//We are starting a new decode, so clear the buffer
		for (int i = 0; i < DEFlpcPCM_BUFFSIZE; i++)
			LpcPcmOutputArray[i] = 0;
		
		//set the input read index to zero
		LpcPcmInputReadIndex = 0; //init at 0
		LpcPcmInputWriteIndex = 256; //

		//set the output read pointer to zero TODO
		LpcPcmOutputReadIndex = 0; //init at 0
		LpcPcmOutputWriteIndex = 256; //

		//also, add a bit reservior buffer for the data out to reduce wastage...

	}
	/* Define input and output block size */
	iOutputBlockSize = TransmParam.iNumDecodedBitsMSC;
	iInputBlockSize = DEFiInputBlockSize/2; //This now works correctly! DM
	//	iInputBlockSize = 38400; //37800; //wrong - too big DM 
	//	iInputBlockSize = 37800; //OLD
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
	int tmp;
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
	double tot;
	double fact;
	tot = (double)succdecod + (double)errdecod; //edited DM added (double)
	if (tot == 0.0) return 0;
	fact = 100.0/tot;
	percentage = (int)(succdecod*fact);
	return percentage;
}

void CAudioSourceDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i,k;

	/* Check if something went wrong in the initialization routine */
	if (DoNotProcessData == TRUE)
	{
		errdecod++;
		return;
	}

	/* Text Message ***********************************************************/
	/* Total frame size depends on whether text message is used or not */
	if ((bTextMessageUsed == TRUE) && (iTotalFrameSize > 975+32))
	{
		/* Decode last for bytes of input block for text message */
		for (i = 0; i < SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
			vecbiTextMessBuf[i] = (*pvecInputData)[iTotalFrameSize + i]; 

		TextMessage.Decode(vecbiTextMessBuf);
	}

	/* Check if LPC should not be decoded */
	if (IsLPCAudio == TRUE)
	{
		bool lpccrc = TRUE;
		iOutputBlockSize = 0;

		//Make N bit checksum DM (20 bits at last count)
		for (i = 0; i < lpcsum; i++)
			chksum[i] = 0;
		for (k = 0; k < lpcsum+1; k++)
		{
			for (i = 0; i < lpcsum; i++)
				chksum[i] ^= (*pvecInputData)[k* lpcsum +i];
		}
		for (i = 0; i < lpcsum; i++)
			lpccrc &= ((*pvecInputData)[18 * LPC10_BITS_IN_COMPRESSED_FRAME +i] == chksum[i]); //Make sure the checksum matches DM

//		lpccrc = TRUE; //TEST DM This permanently unmutes the CRC for testing

		if (lpccrc)
		{  // 17 blocks at 54 bytes encbytes OR..
			//18 blocks at 54 bytes encbytes 
			int j;
			float tmp;
			for (i = 0; i < lpcblocks+1; i++)
			{
				for (j = 0; j < 54; j++)
					encbytes[j] = (*pvecInputData)[i*54+j];
				lpc10_bit_decode(encbytes,wavdata,ds);

				//Add a buffer here to restore the samplerate to 3200Hz average? DM
				
				
				//for (j = 0; j < 180; j++)
				for (j = 0; j < LPC10_SAMPLES_PER_FRAME; j++) //edited DM
				

					//Change this to a proper IIR lowpass and/or resampler DM TODO
					// 
					//6x upsampling
				{
					short interp;
					tmp = AudFirRX(wavdata[j]);
					if (tmp >=  32700) tmp =  32700;
					if (tmp <= -32700) tmp = -32700;

					interp = (short)(0.1666*tmp+0.8334*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)(0.3333*tmp+0.6667*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)(0.5000*tmp+0.5000*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)(0.6667*tmp+0.3333*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)(0.8334*tmp+0.1666*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)tmp;
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 

					oldwavsample = tmp;
				}
				/* Add new block to output block size ("* 2" for stereo output block) */
				//iOutputBlockSize += 180 * 6;
			}

			//DM Not needed anymore:
//			tmp = 0;
//			for (j = 0; j < 140; j++)
//			{
//				int j2;
//				for (j2 = 0;j2 < 6; j2++)
//				{
//					(*pvecOutputData)[iOutputBlockSize++] = tmp; 
//					(*pvecOutputData)[iOutputBlockSize++] = tmp; 
//				}
//			}
			
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
	else if (IsSPEEXAudio == TRUE)
	{
		bool spxcrc = TRUE;
		int j;
		short tmp;
		int ind;
		iOutputBlockSize = 0;

		if (iTotalFrameSize >= 990)
		{
			//Make 15 bit Checksum, 64 iterations
			for (i = 0; i < 15; i++)
				chksum[i] = 0;
			for (k = 0; k < 64; k++)
			{
				for (i = 0; i < 15; i++)
					chksum[i] ^= (*pvecInputData)[k*15+i]; 
			}
			for (i = 0; i < 15; i++)
				spxcrc &= ((*pvecInputData)[960+i] == chksum[i]);
		}

		/* Reset bit extraction access */
		//(*pvecInputData).ResetBitAccess();

		// 20 audio blocks, each 48 bits long = 6 bytes
		if (spxcrc)
		{

			for (i = 0; i < 20; i++)
			{
				
				ind = i*48;
				for (j = 0; j < 48; j++)
					spoutpb[j] = (*pvecInputData)[ind+j];
				for (j = 0; j < 6; j++)
				{
					ind = j*8;
					spoutp[j]  = spoutpb[ind++];
					spoutp[j] += spoutpb[ind++] << 1;
					spoutp[j] += spoutpb[ind++] << 2;
					spoutp[j] += spoutpb[ind++] << 3;
					spoutp[j] += spoutpb[ind++] << 4;
					spoutp[j] += spoutpb[ind++] << 5;
					spoutp[j] += spoutpb[ind++] << 6;
					spoutp[j] += spoutpb[ind++] << 7;
				}
				
				//for (j = 0; j < 6; j++)
				//	spoutp[j] = (*pvecInputData).Separate(8); 
				
				try
				{
					speex_bits_read_from(&decbits, spoutp, 6);
					speex_decode(dec_state, &decbits, spinp);
				}
				catch(...) //modified DM
				{
					bAudioIsOK = FALSE;
					iOutputBlockSize = 0;
					return;
				}
				for (j = 0; j < 160; j++)
				{
					short interp;
					tmp = AudFirRX(spinp[j]);
					if (tmp >=  32700) tmp =  32700;
					if (tmp <= -32700) tmp = -32700;

					interp = (short)(0.1666*tmp+0.8334*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)(0.3333*tmp+0.6667*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)(0.5000*tmp+0.5000*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)(0.6667*tmp+0.3333*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)(0.8334*tmp+0.1666*oldwavsample);
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					interp = (short)tmp;
					(*pvecOutputData)[iOutputBlockSize++] = interp; 
					(*pvecOutputData)[iOutputBlockSize++] = interp; 

					oldwavsample = tmp;
				}
				/* Add new block to output block size ("* 2" for stereo output block) */
				//iOutputBlockSize += 160 * 12;
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
				for (i = 0; i < len - k; i++) chksum[i] ^= (*pvecOutputData)[k+i]; 
			else
				for (i = 0; i < 24; i++) chksum[i] ^= (*pvecOutputData)[k+i]; 
			k += 24;
		}
		for (i = 0; i < 24; i++)
			sstvcrc &= ((*pvecInputData)[len+i] == chksum[i]);

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
	int iCurAudioStreamID;
	int iCurSelServ;

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

//		iOutputBlockSize = (int)((_REAL)SOUNDCRD_SAMPLE_RATE * (_REAL)0.4); //Try this DM
//		iOutputBlockSize = (int)((_REAL)SOUNDCRD_SAMPLE_RATE * (_REAL)0.4 * 2); //Try this DM ==========================================
		iOutputBlockSize = 2 * DEFiInputBlockSize; // 2 * 38400; //<-- Original WORKS
//		iOutputBlockSize = 4 * 38400; //2*37800; 4* also seems to work well....
//		iOutputBlockSize = 8 * 38400; //2*37800; Try 8? DM no better....

		/* Get number of total input bits for this module */
		iInputBlockSize = ReceiverParam.iNumAudioDecoderBits;

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
