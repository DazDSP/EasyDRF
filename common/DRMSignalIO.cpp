/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Transmit and receive data
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

#include "DRMSignalIO.h"
#include "settings.h" //added DM

#include "../Dialog.h"
#include "../resource.h"

//PAPR processing DM
float oscpo = 0.0; //sinewave phase increment
int PAPRt = 0; //clipping threshold
int p1 = 0;
int p2 = 0;
int p3 = 0;
int p4 = 0;
int Id1 = 0;
int Id2 = 0;
int Qd1 = 0;
int Qd2 = 0;
int op1 = 0;
int op2 = 0;
int op3 = 0;
int op4 = 0;
int oId1 = 0;
int oId2 = 0;
int oQd1 = 0;
int oQd2 = 0;


/* Implementation *************************************************************/
/******************************************************************************\
* Transmitter                                                                  *
\******************************************************************************/
void CTransmitData::ProcessDataInternal(CParameter& Parameter)
{
	int i = 0; //init DM

	if ((paintmode == 0) || (paintmode == 2)) {
		//if (1 == 1) {

			//48kHz samplerate

			/* Filtering of output signal (FIR filter) ------------------------------ */
			/* Transfer input data in Matlib library vector */
		for (i = 0; i < iInputBlockSize; i++)
		{
			rvecDataI[i] = (*pvecInputData)[i].real() * rNormFactor * 0.5; //compensate amplitude here * 0.5 because now it's IQ DM
			rvecDataQ[i] = (*pvecInputData)[i].imag() * rNormFactor * 0.5; //compensate amplitude here * 0.5 because now it's IQ DM
		}

		//Filter 1 - antialias filter the entire buffer at 48kHz samplerate (6kHz stopband lowpass)
		//This new filter only computes every 4th sample, so it needs the smaller output buffer specified as the destination DM
		rvecDataDecI = FIRFiltDec(rvec1, rvecDataI, rvecDataDecI, rvecZI1); //filter I
		rvecDataDecQ = FIRFiltDec(rvec1, rvecDataQ, rvecDataDecQ, rvecZQ1); //filter Q

		//PAPR feature - Improve average power of the data by Hilbert clipping DM 2022
		//Required:
		//IQ mixing carriers (cos and sin at the Fc = 1.575kHz) - This changes with the DC freq setting
		//Clipping thresholds for QAM4, QAM16 and QAM64 are -18,-15,-12 dB (almost ideal, could take a little more clipping)
		//1. Mix the IQ signal down to centre on 0Hz in OFDM.cpp
		//2. Lowpass filter the IQ at 1.25kHz for a 2.5kHz bandpass to reduce OFDM sidelobes
		//3. Hilbert clip the IQ for PAPR gain
		//4. Lowpass filter the IQ at 1.25kHz for a 2.5kHz bandpass again
		//5. Hilbert clip the IQ again with overshoot compensation
		//6. Lowpass filter the IQ at 1.25kHz for a 2.5kHz bandpass again (ideally, a slightly wider filter for less overshoot)
		//7. Mix the IQ back to the original audio frequency again
		//All of this is now performed at a 12kHz samplerate for lower CPU use
	} //paintmode if

//Add WFText audio here ================================================================================================================
	if (paintmode == 1) {
		//Add WFText here so PAPR clips it - may need a custom clip level
		//paintmode == 1 which is for WFText
		//encode bitmap buffer and write to sound buffer at 12kHz
		//the block size here might not be very big...
		//it was easier to write all the sound data to one big buffer, then read it out as needed...

#define WFSCALE 31000 //scale this for the correct clipping level
		//do we have audio to play?
		if (audio.GetSize() > 0) {
			//copy as many samples as needed
			for (i = 0; i < iInputBlockSize / 4; i++) {
				rvecDataDecI[i] = audio[readout].real() * WFSCALE;
				rvecDataDecQ[i] = audio[readout].imag() * WFSCALE;
				readout++;
				if (readout >= audio.GetSize()) {
					readout = 0;
					paintmode = -1; //keep OFDM audio muted until TX turns off
					audio.Init(0); //clear the buffer and release the memory
					OnCommand(mainwindow, IDB_START, 0); //switch to receive
					break; //exit the for loop
				}
			}
		}
	}
//======================================================================================================================================
#if USEPAPR == 1

	//Filter 2 - Prefilter to remove OFDM sidelobes - use same coeffs as the post clipper filter (NUM3)
	//lowpass filter the entire buffer at 12kHz samplerate
	rvecDataDecI = Filter(rvec3, rvecA, rvecDataDecI, rvecZDecI2); //filter I
	rvecDataDecQ = Filter(rvec3, rvecA, rvecDataDecQ, rvecZDecQ2); //filter Q

	PAPRt = 6553; //try -13dB //8192; //default PAPR clipping threshold is -12dB for QAM64
	//if (getqam() == 2) PAPRt = 8192; //these need to be ints
	if (getqam() == 1) PAPRt = 5792; //QAM16 -15dB
	if (getqam() == 0) PAPRt = 4096; //QAM4  -18dB
	if (moderestore != -1) PAPRt = 4096; //Tuning tone and waterfall text mode

	//Hilbert clip the IQ signal
	for (i = 0; i < iInputBlockSize / 4; i++) {
		int const I = rvecDataDecI[i];
		int const Q = rvecDataDecQ[i];
		int const amp = max(sqrt(I * I + Q * Q), PAPRt); //vectorsum and threshold
		int const peak = max(amp, max(max(p3, p4), max(p1, p2))); //peak stretcher

		//update peak history
		p4 = p3;
		p3 = p2;
		p2 = p1;
		p1 = amp;

		float gain = (float)23166 / peak; //limit at -3dBFS (levels are int!) 0dBFS = 32767
		rvecDataDecI[i] = Id2 * gain;
		rvecDataDecQ[i] = Qd2 * gain;

		//update IQ history for delay compensation
		Id2 = Id1;
		Id1 = I;
		Qd2 = Qd1;
		Qd1 = Q;
	}
	//Filter 3 - Post clipper filter
	//lowpass filter the entire buffer at 12kHz samplerate
	rvecDataDecI = Filter(rvec3, rvecA, rvecDataDecI, rvecZDecI3); //filter I
	rvecDataDecQ = Filter(rvec3, rvecA, rvecDataDecQ, rvecZDecQ3); //filter Q

	//Apply overshoot compensation
	//Hilbert clip the IQ signal
	for (i = 0; i < iInputBlockSize / 4; i++) {
		int const I = rvecDataDecI[i];
		int const Q = rvecDataDecQ[i];
		int constexpr oc = 23166; //Overshoot clip level is -3dBFS
		int const amp = (max(sqrt(I * I + Q * Q) - oc, 0) * 2) + oc;
		int  const peak = max(amp, max(max(op3, op4), max(op1, op2))); //peak stretcher

		//update peak history
		op4 = op3;
		op3 = op2;
		op2 = op1;
		op1 = amp;

		float gain = (float)23166 / peak; //limit at -3dBFS (levels are int!)
		rvecDataDecI[i] = oId2 * gain;
		rvecDataDecQ[i] = oQd2 * gain;

		//update IQ history for delay compensation
		oId2 = oId1;
		oId1 = I;
		oQd2 = oQd1;
		oQd1 = Q;
	}

	//Filter 4 - Overshoot compensation filter (same coeffs as OFDM sidelobes filter)
	//lowpass filter the entire buffer at 12kHz
	rvecDataDecI = Filter(rvec4, rvecA, rvecDataDecI, rvecZDecI4); //filter I
	rvecDataDecQ = Filter(rvec4, rvecA, rvecDataDecQ, rvecZDecQ4); //filter Q

	//convert 0Hz IF back to normal baseband output frequency
	for (i = 0; i < iInputBlockSize / 4; i++)
	{
		//Generate carriers
		//float Icar = cos(oscpo);
		//float Qcar = sin(oscpo);
		//oscpo -= Fcp; //add phase increment for next pass
		//if (oscpo < -crPi) oscpo += 2 * crPi; //wrap phase at 2Pi
		float Icar = WURotate.real();
		float Qcar = WURotate.imag();


		//Convert to audio
		int I = rvecDataDecI[i];
		int Q = rvecDataDecQ[i];
		rvecDataDecI[i] = (I * Qcar) - (Q * Icar); //positive frequency shift
		//rvecDataDecQ[i] = (I * Icar) + (Q * Qcar); //positive frequency shift

		WURotate *= WUSinStep; //Rotate phase
		CReal gain = 1 / Max(0.5, sqrt(SqMag(WURotate)));
		WURotate *= gain; //Hilbert clip to limit level
	}

//TEST WFtext IQ quality here ==================================================================================================
//==============================================================================================================================

	//Interpolate by 2 (zero-stuff)
	int w = 0;
	for (i = 0; i < iInputBlockSize / 4; i++) {
		rvecData2I[w] = rvecDataDecI[i];
		//rvecData2Q[w] = rvecDataDecQ[i];
		w += 1;
		rvecData2I[w] = 0;
		//rvecData2Q[w] = 0;
		w += 1;
	}
	//Filter 5 - Filter at 24kHz samplerate
	rvecData2I = Filter(rvec5, rvecA, rvecData2I, rvecZDecI5); //interpolation filter DM
	//rvecData2Q = Filter(rvec5, rvecA, rvecData2Q, rvecZDecQ5); //interpolation filter DM

	//Interpolate by 2 (zero-stuff)
	w = 0;
	for (i = 0; i < iInputBlockSize / 2; i++) {
		rvecDataI[w] = rvecData2I[i];
		//rvecDataQ[w] = rvecData2Q[i];
		w += 1;
		rvecDataI[w] = 0;
		//rvecDataQ[w] = 0;
		w += 1;
	}
	//Filter 6 - Filter at 48kHz samplerate
	rvecDataI = Filter(rvec6, rvecA, rvecDataI, rvecZI6); //interpolation filter DM
	//rvecDataQ = Filter(rvec6, rvecA, rvecDataQ, rvecZQ6); //interpolation filter DM

#endif //USEPAPR

//Read out data to sound buffer:
/* Convert vector type. Fill vector with symbols (collect them) */
	const int iNs2 = iInputBlockSize * 2;  //added DM -- sizes adjusted to fix soundcard samplerate problems
	const float OutLevel = (float)max(0.3535, (float)TxLevel); //allow output level to be set to -12dB
	for (i = 0; i < iNs2; i += 2) //added DM
	{
		const int iCurIndex = iBlockCnt * iNs2 + i; //added DM

		//Normal audio output uses real I channel only - Q is also available
		const short sCurOutReal = (short)(rvecDataI[i / 2]) * OutLevel; // *rNormFactor); //added DM
		//const short sCurOutImag = (short)(rvecDataQ[i / 2]); // * rNormFactor); //added DM

		/* Use real valued signal as output for both sound card channels */
		vecsDataOut[iCurIndex] = vecsDataOut[iCurIndex + 1] = sCurOutReal; //@ //added DM

		//Or... use IQ output for testing																   
		//vecsDataOut[iCurIndex] = sCurOutReal; //left output channel I
		//vecsDataOut[iCurIndex + 1] = sCurOutImag; //right output channel Q
	}

	iBlockCnt++;
	if (iBlockCnt == iNumBlocks)
	{
		iBlockCnt = 0;

		/* Write data to sound card. Must be a blocking function */
		pSound->Write(vecsDataOut);
	}

}

void CTransmitData::InitInternal(CParameter& TransmParam)
{
	float const Fc = OFFSET + rDefCarOffset; //Hz Weaver mixing frequency
	float const Fcp = (-Fc / (SOUNDCRD_SAMPLE_RATE / 4)) * crPi * 2;
	WUSinStep = CComplex(cos(Fcp), sin(Fcp));
	WURotate = (_REAL)1.0;

	/* Init vector for storing a complete DRM frame number of OFDM symbols */
	iBlockCnt = 0;
	iNumBlocks = TransmParam.iNumSymPerFrame;

	const int iTotalSize = TransmParam.iSymbolBlockSize * 2 /* Stereo */ * iNumBlocks; //@ //Added DM
	//const int iTotalSize = TransmParam.iSymbolBlockSize * iNumBlocks; //@ //edited DM

	vecsDataOut.Init(iTotalSize);

	/* Init sound interface */
	pSound->InitPlayback(iTotalSize, TRUE);

	/* Init filter taps */
	rvec1.Init(FILTER_TAP_NUM1); //48k rate decimation
//	rvec2.Init(FILTER_TAP_NUM3); //OFDM sidelobes filter USE NUM3 INSTEAD
	rvec3.Init(FILTER_TAP_NUM3); //Post clipper filter (sharper)
	rvec4.Init(FILTER_TAP_NUM4); //OFDM and Overshoot Compensation filter (wider)
	rvec5.Init(FILTER_TAP_NUM5); //12k to 24k interpolation filter
	rvec6.Init(FILTER_TAP_NUM6); //24k to 48k interpolation filter

	/* Choose correct filter for chosen DRM bandwidth. Also, adjust offset
	   frequency for different modes. E.g., 5 kHz mode is on the right side
	   of the DC frequency */
	   //Ham-DRM is always on the right side of the DC frequency (positive) DM 

	   //PAPR code is added here
	   //The QAM mode is needed to compute the clipping level
	   //getqam() results:
	   // 0 = QAM4
	   // 1 = QAM16
	   // 2 = QAM64

	   //PAPR clipping calcs (Normalized to FS)
	   // 0 = 6dB = 0.5
	   // 1 = 3dB = 0.707
	   // 2 = 0dB = 1

	   //Full scale (0dBFS) = 1 = 32767
	   //Normalized clipper input level = -12dB = 0.25 = 8192

#if selectBW //not used now DM
//	float* pCurFilt = fTransmFilt2_5c; //added init DM
	float* pCurFilt = filter_taps2; //decimated version DM

	CReal rNormCurFreqOffset{}; //added init DM

	switch (TransmParam.GetSpectrumOccup())
	{
	case SO_0:
		//		pCurFilt = fTransmFilt4_5;
		//		pCurFilt = fTransmFilt2_5c; //A correct bandwidth filter works better! DM
		pCurFilt = filter_taps2; //decimated version - same for both bandwidths DM

		/* Completely on the right side of DC */
		//rNormCurFreqOffset = (rDefCarOffset + (CReal) 2250.0) / SOUNDCRD_SAMPLE_RATE; //edited DM
		rNormCurFreqOffset = (rDefCarOffset + (CReal)OFFSET) / (SOUNDCRD_SAMPLE_RATE / 4); //now at 12kHz rate DM
		break;

	case SO_1:
		//		pCurFilt = fTransmFilt5;
				//pCurFilt = fTransmFilt2_5c; //A correct bandwidth filter works better! DM
		pCurFilt = filter_taps2; //decimated version - same for both bandwidths DM

		/* Completely on the right side of DC */
		//rNormCurFreqOffset = (rDefCarOffset + (CReal) 2500.0) / SOUNDCRD_SAMPLE_RATE; //edited DM
		rNormCurFreqOffset = (rDefCarOffset + (CReal)OFFSET) / (SOUNDCRD_SAMPLE_RATE / 4); //now at 12kHz rate DM
		break;
	}

	//Modulate the lowpass filter coeffs to make it a bandpass filter - No longer needed - OFDM input is now centred on 0Hz
//	for (int i = 0; i < NUM_TAPS_TRANSMFILTER; i++) rvecB[i] = pCurFilt[i] * Cos((CReal) 2.0 * crPi * rNormCurFreqOffset * i);
//	for (int i = 0; i < FILTER_TAP_NUM2; i++) rvecB[i] = pCurFilt[i] * Cos((CReal)2.0 * crPi * rNormCurFreqOffset * i);
#endif //selectBW

	//Copy all filter coefficients into arrays
	//Use a plain lowpass filter for PAPR filters (not bandpass)
	for (int i = 0; i < FILTER_TAP_NUM1; i++) rvec1[i] = filter_taps1[i]; //48kHz decimation antialias filter
	//for (int i = 0; i < FILTER_TAP_NUM2; i++) rvec2[i] = filter_taps2[i]; //12kHz - use NUM3 coeffs for BOTH filter 2 and 3
	for (int i = 0; i < FILTER_TAP_NUM3; i++) rvec3[i] = filter_taps3[i]; //12kHz OFDM and post clipper filter coeffs
	for (int i = 0; i < FILTER_TAP_NUM4; i++) rvec4[i] = filter_taps4[i]; //12kHz Overshoot Compensation filter coeffs
	for (int i = 0; i < FILTER_TAP_NUM5; i++) rvec5[i] = filter_taps5[i] * 2; //24kHz scale coeffs amplitude by 2 to compensate for zero-stuffing
	for (int i = 0; i < FILTER_TAP_NUM6; i++) rvec6[i] = filter_taps6[i] * 2; //48kHz scale coeffs amplitude by 2 to compensate for zero-stuffing

	/* Only FIR filter */
	rvecA.Init(1);
	rvecA[0] = (CReal)1.0;

	/* Filter state memory (init with zeros) and data vector */
	rvecZI1.Init(FILTER_TAP_NUM1 - 1, (CReal)0.0);
	rvecZQ1.Init(FILTER_TAP_NUM1 - 1, (CReal)0.0);
	rvecZDecI2.Init(FILTER_TAP_NUM3 - 1, (CReal)0.0); //12kHz DM OFDM filter
	rvecZDecQ2.Init(FILTER_TAP_NUM3 - 1, (CReal)0.0); //12kHz DM OFDM filter
	rvecZDecI3.Init(FILTER_TAP_NUM3 - 1, (CReal)0.0); //12kHz DM post clipper
	rvecZDecQ3.Init(FILTER_TAP_NUM3 - 1, (CReal)0.0); //12kHz DM post clipper
	rvecZDecI4.Init(FILTER_TAP_NUM4 - 1, (CReal)0.0); //12kHz DM overshoot clipper filter
	rvecZDecQ4.Init(FILTER_TAP_NUM4 - 1, (CReal)0.0); //12kHz DM overshoot clipper filter
	rvecZDecI5.Init(FILTER_TAP_NUM5 - 1, (CReal)0.0); //24kHz DM interpolation filter
	rvecZDecQ5.Init(FILTER_TAP_NUM5 - 1, (CReal)0.0); //24kHz DM interpolation filter
	rvecZI6.Init(FILTER_TAP_NUM6 - 1, (CReal)0.0); //48kHz DM interpolation filter
	rvecZQ6.Init(FILTER_TAP_NUM6 - 1, (CReal)0.0); //48kHz DM interpolation filter

	//Data buffer sizes match the input and output data length at the scaled samplerate used DM
	rvecDataI.Init(TransmParam.iSymbolBlockSize); //48kHz DM
	rvecDataQ.Init(TransmParam.iSymbolBlockSize); //48kHz DM
	rvecData2I.Init(TransmParam.iSymbolBlockSize / 2); //24kHz DM
	rvecData2Q.Init(TransmParam.iSymbolBlockSize / 2); //24kHz DM
	rvecDataDecI.Init(TransmParam.iSymbolBlockSize / 4); //12kHz DM
	rvecDataDecQ.Init(TransmParam.iSymbolBlockSize / 4); //12kHz DM

	/* All robustness modes and spectrum occupancies should have the same output
	   power. Calculate the normaization factor based on the average power of
	   symbol (the number 3000 was obtained through output tests) */
	   // bei 3000 -> +- 6000
	   // bei 9000 -> +- 20000 achtung, 12000 ist zuviel -> foldback
	   // moeglich +-32000
	rNormFactor = (CReal)6000.0 / Sqrt(TransmParam.rAvPowPerSymbol);

	/* Define block-size for input */
	iInputBlockSize = TransmParam.iSymbolBlockSize;
}

CTransmitData::~CTransmitData()
{
}

#define no_dc_tap 17*2

double the_dcsum = 0.0;
double dcsumbuf[no_dc_tap] = {0.0};
int dcsumbufpt = 0;
 
_REAL averdc = 0.0;

//improved DC blocker DM Feb 2022
int In = 0; //new input
int Inp = 0; //previous input
int Out = 0; //Output
int Outp = 0; //previous output

/******************************************************************************\
* Receive data from the sound card                                             *
\******************************************************************************/
void CReceiveData::ProcessDataInternal(CParameter& Parameter)
{
	int i = 0; //init DM
	double dcsum = 0.0;

	if (bUseSoundcard == TRUE) //added DM
	{
		/* Using sound card ------------------------------------------------- */
		/* Get data from sound interface. The read function must be a
		   blocking function! */
		if (pSound->Read(vecsSoundBuffer) == FALSE)
			PostWinMessage(MS_IOINTERFACE, 0); /* green light */
		else
			PostWinMessage(MS_IOINTERFACE, 2); /* red light */

		/* Write data to output buffer */
		for (i = 0; i < iOutputBlockSize; i++)
		{
#ifdef MIX_INPUT_CHANNELS //added DM ---------------------------------------------
			/* Mix left and right channel together. Prevent overflow! First,
			   copy recorded data from "short" in "int" type variables */
			const int iLeftChan = vecsSoundBuffer[2 * i];
			const int iRightChan = vecsSoundBuffer[2 * i + 1];

			int temp = (_REAL)((iLeftChan + iRightChan) / 2); //@
			dcsum += temp;
			(*pvecOutputData)[i] = temp - averdc;
#else
			/* Use only desired channel, chosen by "RECORDING_CHANNEL" */
			//(*pvecOutputData)[i] = (_REAL)vecsSoundBuffer[2 * i + RECORDING_CHANNEL]; //added DM
			//(*pvecOutputData)[i] = (_REAL) vecsSoundBuffer[i]; //edited DM

			//Add highpass filter to avoid DC causing data errors DM 2022
			//find the DC offset by integrating the sample values
			//Version 1 of DC blocker
			//averdc = (_REAL)(averdc*0.98)+(vecsSoundBuffer[2 * i + RECORDING_CHANNEL])*0.02; //add sample value to DC computation DM
			//(*pvecOutputData)[i] = (_REAL)vecsSoundBuffer[2 * i + RECORDING_CHANNEL]-averdc; //subtract average DC value DM

			//Version 2 of DC blocker
			In = (_REAL)vecsSoundBuffer[2 * i + RECORDING_CHANNEL];
			Out = In - Inp + (0.97 * Outp); //compute 1st order highpass DM
			(*pvecOutputData)[i] = Out;
			Outp = Out;
			Inp = In;

#endif
		}

		/* This old DC removal code is NOT being used (and it can't handle varying DC offset either...) DM
		the_dcsum -= dcsumbuf[dcsumbufpt];
		dcsumbufpt++;
		if (dcsumbufpt >= no_dc_tap) dcsumbufpt = 0;
		dcsumbuf[dcsumbufpt] = dcsum / (double)iOutputBlockSize;  
		the_dcsum += dcsumbuf[dcsumbufpt];

		averdc = the_dcsum;
		*/
	}
	else
	{
	/* Read data from file ---------------------------------------------- */
	for (i = 0; i < iOutputBlockSize; i++)
	{
		/* If end-of-file is reached, stop simulation */
		float tIn;

		if (fscanf_s(pFileReceiver, "%e\n", &tIn) == EOF)
		{
			Parameter.bRunThread = FALSE;

			/* Set output block size to zero to avoid writing invalid
			   data */
			iOutputBlockSize = 0;

			return;
		}
		else
		{
			/* Write internal output buffer */
			(*pvecOutputData)[i] = (_REAL)tIn;
		}
	}
	}


	/* Flip spectrum if necessary ------------------------------------------- */
	if (bFippedSpectrum == TRUE)
	{
		static _BOOLEAN bFlagInv = FALSE;

		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* We flip the spectrum by using the mirror spectrum at the negative
			   frequencys. If we shift by half of the sample frequency, we can
			   do the shift without the need of a Hilbert transformation */
			if (bFlagInv == FALSE)
			{
				(*pvecOutputData)[i] = -(*pvecOutputData)[i];
				bFlagInv = TRUE;
			}
			else
				bFlagInv = FALSE;
		}
	}


	/* Copy data in buffer for spectrum calculation ------------------------- */
	vecrInpData.AddEnd((*pvecOutputData), iOutputBlockSize);


	/* Update level meter */
	SignalLevelMeter.Update((*pvecOutputData));
}

CRealVector		vecrHammingWindow; //added DM

void CReceiveData::InitInternal(CParameter& Parameter)
{
	/* Init sound interface. Set it to one symbol. The sound card interface
	   has to taken care about the buffering data of a whole MSC block.
	   Use stereo input (* 2) */
	pSound->InitRecording(Parameter.iSymbolBlockSize * 2); //added DM
//	pSound->InitRecording(Parameter.iSymbolBlockSize ); //@  

	/* Init buffer size for taking stereo input */
	vecsSoundBuffer.Init(Parameter.iSymbolBlockSize * 2); //added DM
//	vecsSoundBuffer.Init(Parameter.iSymbolBlockSize); //@ 

	/* Init signal meter */
	SignalLevelMeter.Init(0);

	/* Init Hanning Window */
	HanningWindow.Init();

	/* Define output block-size */
	iOutputBlockSize = Parameter.iSymbolBlockSize;
}

CReceiveData::~CReceiveData()
{
	/* Close file (if opened) */
	if (pFileReceiver != NULL)
		fclose(pFileReceiver);
}

void CHanningWindow::Init(void)
{

	const int iLenDSInputVector = 2048;
	vecrHannWind.Init(iLenDSInputVector);
	vecrHannWind = Hann(iLenDSInputVector);
	for (int i = 0; i < iLenDSInputVector; i++)
		vecrHannWind[i] *= 2.0;
	bIsInit = TRUE;
}

void CReceiveData::GetInputSpec(CVector<_REAL>& vecrData)
{
	int				i = 0; //init DM
	CComplexVector	veccSpectrum;
	CRealVector		vecrFFTInput;
	_REAL			rNormSqMag{}; //init DM
	_REAL			rNormData{}; //init DM

	const int iLenInputVector = 2*2048;
	const int iLenDSInputVector = 2048;
	const int iLenSpec = 512;

	if (!HanningWindow.bIsInit) return;

	/* Init input and output vectors */
	vecrData.Init(iLenSpec, (_REAL) 0.0);

	/* Lock resources */
	Lock();

	rNormData = 0.2 * (_REAL)iLenInputVector * iLenInputVector * _MAXSHORT;

	veccSpectrum.Init(iLenSpec);

	/* Copy data from shift register in Matlib vector */
	vecrFFTInput.Init(iLenDSInputVector);
	for (i = 0; i < iLenDSInputVector; i++)
		vecrFFTInput[i] = vecrInpData[i*2] * HanningWindow.vecrHannWind[i];

	/* Get spectrum */
	veccSpectrum = rfft(vecrFFTInput);

	/* Log power spectrum data */
	for (i = 0; i < iLenSpec; i++)
	{
		rNormSqMag = SqMag(veccSpectrum[i]) / rNormData;
		vecrData[i] = sqrt(rNormSqMag);
	}

	/* Release resources */
	Unlock();
}

/* Level meter -------------------------------------------------------------- */
void CSignalLevelMeter::Update(_REAL rVal)
{
	/* Search for max */
	const _REAL rCurAbsVal = Abs(rVal);
	if (rCurAbsVal > rCurLevel)
		rCurLevel = rCurAbsVal;
}


void CSignalLevelMeter::Update(CVector<_REAL> vecrVal)
{
	/* Do the update for entire vector */
	const int iVecSize = vecrVal.Size();
	rCurLevel = 0;
	for (int i = 0; i < iVecSize; i+=2)
		Update(vecrVal[i]);
}

void CSignalLevelMeter::Update(CVector<_SAMPLE> vecsVal)
{
	/* Do the update for entire vector, convert to real */
	const int iVecSize = vecsVal.Size();
	rCurLevel = 0;
	for (int i = 0; i < iVecSize; i+=2)
		Update((_REAL) vecsVal[i]);
}

_REAL CSignalLevelMeter::Level()
{
	return rCurLevel / _MAXSHORT;
}

