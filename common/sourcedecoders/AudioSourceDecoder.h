/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *  Francesco Lanza
 *
 * Description:
 *	See AudioSourceDecoder.cpp
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

#if !defined(AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_)
#define AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../Modul.h"
#include "../CRC.h"
#include "../TextMessage.h"
#include "../datadecoding/DataDecoder.h"

/* Definitions ****************************************************************/
/* Forgetting factor for audio blocks in case CRC was wrong */
#define FORFACT_AUD_BL_BAD_CRC			((_REAL) 0.6)

extern int FrameSize;
extern int BlockSize;
extern int TextBytes;
extern int TextBytesi;

/* Classes ********************************************************************/
class CAudioSourceEncoder : public CTransmitterModul<_SAMPLE, _BINARY>
{
public:
	CAudioSourceEncoder();
	virtual ~CAudioSourceEncoder();

	void SetTextMessage(const string& strText);
	void ClearTextMessage();

	void SetPicFileName(const string& strFileName, const string& strFileNamenoDir, CVector<short>  vecsToSend)
		{DataEncoder.GetSliShowEnc()->AddFileName(strFileName, strFileNamenoDir, vecsToSend);}
	void ClearPicFileNames()
		{DataEncoder.GetSliShowEnc()->ClearAllFileNames();}

	void SetTheStartDelay(int thestartdelay)
		{ DataEncoder.GetSliShowEnc()->SetMyStartDelay(thestartdelay);} //edited DM
	int GetPicCnt()
		{return DataEncoder.GetSliShowEnc()->GetPicCnt(); }
	int GetPicPerc()
		{return DataEncoder.GetSliShowEnc()->GetPicPerc(); }
	int GetNoOfPic()
		{return DataEncoder.GetSliShowEnc()->GetNoOfPic(); }
	int GetPicSegmAct() 
		{return DataEncoder.GetSliShowEnc()->GetPicSegmAct(); }
	int GetPicSegmTot() 
		{return DataEncoder.GetSliShowEnc()->GetPicSegmTot(); }					 

	void GetTxSpeechBuffer(CVector<_REAL>& scopeData) {

		/* Init output vectors */
		//vecrData.Init(19200, (_REAL)0.0); //initialize this on program start instead
		for (int j = 0; j < 19200; j++) {
			scopeData[j] = speechIN[j]; //copy speech buffer for Oscilloscope display
		}
		return;
	};

protected:
	CTextMessageEncoder TextMessage{};
	_BOOLEAN			bUsingTextMessage{};
	CDataEncoder		DataEncoder{};
	int					iTotPacketSize{};
	_BOOLEAN			bIsDataService{};

	CRealVector			speechIN; //speech buffer DM
	CRealVector			speechLPFDec; //~8kHz LPC-10 decimation buffer DM
	CRealVector			speechLPFDecSpeex; //8kHz decimation buffer DM
	CRealVector			rvecS;		//4kHz LPF filter coeffs DM
	CRealVector			rvecZS;		//state memory

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class CAudioSourceDecoder : public CReceiverModul<_BINARY, _SAMPLE>
{
public:
	CAudioSourceDecoder();
	virtual ~CAudioSourceDecoder();

	void GetSpeechBuffer(CVector<_REAL>& scopeData) {
	
		/* Init output vectors */
		//vecrData.Init(19200, (_REAL)0.0); //initialize this on program start instead
		for (int j = 0; j < 19200; j++) {
			scopeData[j] = speechLPF[j]; //copy speech buffer for Oscilloscope display
		}
		return;
	};

	int getdecodperc();

protected:
	enum EInitErr {ET_ALL, ET_AAC}; /* ET: Error type */ //class added DM =====================
	class CInitErr 
	{
	public:
		CInitErr(EInitErr eNewErrType) : eErrType(eNewErrType) {}
		EInitErr eErrType{};
	};

	/* General */
	_BOOLEAN			DoNotProcessData{};
	_BOOLEAN			IsLPCAudio{};
	_BOOLEAN			IsSPEEXAudio{};
	_BOOLEAN			IsCELPAudio{};
	_BOOLEAN			IsSSTVData{};

	/* Text message */
	_BOOLEAN			bTextMessageUsed{};
	CTextMessageDecoder	TextMessage{};
	CVector<_BINARY>	vecbiTextMessBuf{};

	//FIR audio filter buffers for speech modes DM
	CRealVector			speechLPF; //Speech buffer DM
	CRealVector			speechLPFDec; //Speech buffer DM
	CRealVector			speechLPFDec2; //Speech buffer 2 DM
	CRealVector			rvecA;
	CRealVector			rvecS;		//4kHz LPF filter coeffs DM
	CRealVector			rvecZS;		//state memory
	CRealVector			rvecS2;		//4kHz LPF filter coeffs DM
	CRealVector			rvecZS2;	//state memory

	int					iTotalFrameSize{};

	_BOOLEAN			bAudioIsOK{};
	_BOOLEAN			bAudioWasOK{};


	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 89100 Hz

* 0 Hz - 4000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.6242239011973285 dB

* 7000 Hz - 44500 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -41.61622859504223 dB

*/

#define FILTER_TAP_NUMS2 49

static double filter_tapsS2[FILTER_TAP_NUMS2] = {
  0.006147073445362295,
  0.006169527241612282,
  0.006729684284137452,
  0.00866793069556383,
  0.007872677267484005,
  0.007567205846123031,
  0.0042863982074637695,
  0.001184068385578285,
  -0.0044103260981075785,
  -0.009179918425754356,
  -0.01502251220893395,
  -0.018531050994469712,
  -0.021141617612170753,
  -0.01969880839568269,
  -0.015720475281452265,
  -0.006752028702438089,
  0.005125286264390968,
  0.02135036191939927,
  0.039082907036674254,
  0.05885752389685666,
  0.07732207171984029,
  0.09461970094637498,
  0.10750087584462426,
  0.11645128978983152,
  0.11896206846984418,
  0.11645128978983152,
  0.10750087584462426,
  0.09461970094637498,
  0.07732207171984029,
  0.05885752389685666,
  0.039082907036674254,
  0.02135036191939927,
  0.005125286264390968,
  -0.006752028702438089,
  -0.015720475281452265,
  -0.01969880839568269,
  -0.021141617612170753,
  -0.018531050994469712,
  -0.01502251220893395,
  -0.009179918425754356,
  -0.0044103260981075785,
  0.001184068385578285,
  0.0042863982074637695,
  0.007567205846123031,
  0.007872677267484005,
  0.00866793069556383,
  0.006729684284137452,
  0.006169527241612282,
  0.006147073445362295
};


/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 48000 Hz

* 0 Hz - 3700 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.7383065475070077 dB

* 4400 Hz - 24000 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -40.15979230920542 dB

*/

#define FILTER_TAP_NUMS 107

static double filter_tapsS[FILTER_TAP_NUMS] = {
  0.006473720950574429,
  0.0032065389945106224,
  0.0030098704145874317,
  0.002042633344865547,
  0.0003814931807759399,
  -0.001703316272853219,
  -0.0037870614250812332,
  -0.0053862243637712,
  -0.006078921224537158,
  -0.005628686766148074,
  -0.0040675536875699045,
  -0.00171987669531221,
  0.0008538394070133059,
  0.0029847572000017693,
  0.004068211519981329,
  0.003734910832063731,
  0.001974946843196097,
  -0.0008236074669734899,
  -0.003944220395035163,
  -0.00651060634083058,
  -0.007718608669743186,
  -0.0070662311536504405,
  -0.00452649478690276,
  -0.0006022931426277302,
  0.003752752093548056,
  0.007374223989633839,
  0.009151504609146804,
  0.008409948649916253,
  0.005068303360119585,
  -0.0002100698897031663,
  -0.00614077965842723,
  -0.011116346076733154,
  -0.013606906762826528,
  -0.012582341593945242,
  -0.007871275196661962,
  -0.000314738668865231,
  0.00833854990606367,
  0.015796029916482386,
  0.01976583170875572,
  0.018570177925460618,
  0.01168213835676921,
  0.000042452609698138316,
  -0.01395220639480135,
  -0.02682032587320644,
  -0.03466632799235094,
  -0.03401960819392587,
  -0.022661956127323246,
  -0.0002580693559838257,
  0.03137556650735465,
  0.06844105781346048,
  0.10576328756903988,
  0.13769720890159795,
  0.15919803179582057,
  0.16677856097508018,
  0.15919803179582057,
  0.13769720890159795,
  0.10576328756903988,
  0.06844105781346048,
  0.03137556650735465,
  -0.0002580693559838257,
  -0.022661956127323246,
  -0.03401960819392587,
  -0.03466632799235094,
  -0.02682032587320644,
  -0.01395220639480135,
  0.000042452609698138316,
  0.01168213835676921,
  0.018570177925460618,
  0.01976583170875572,
  0.015796029916482386,
  0.00833854990606367,
  -0.000314738668865231,
  -0.007871275196661962,
  -0.012582341593945242,
  -0.013606906762826528,
  -0.011116346076733154,
  -0.00614077965842723,
  -0.0002100698897031663,
  0.005068303360119585,
  0.008409948649916253,
  0.009151504609146804,
  0.007374223989633839,
  0.003752752093548056,
  -0.0006022931426277302,
  -0.00452649478690276,
  -0.0070662311536504405,
  -0.007718608669743186,
  -0.00651060634083058,
  -0.003944220395035163,
  -0.0008236074669734899,
  0.001974946843196097,
  0.003734910832063731,
  0.004068211519981329,
  0.0029847572000017693,
  0.0008538394070133059,
  -0.00171987669531221,
  -0.0040675536875699045,
  -0.005628686766148074,
  -0.006078921224537158,
  -0.0053862243637712,
  -0.0037870614250812332,
  -0.001703316272853219,
  0.0003814931807759399,
  0.002042633344865547,
  0.0030098704145874317,
  0.0032065389945106224,
  0.006473720950574429
};

#endif // !defined(AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_)

