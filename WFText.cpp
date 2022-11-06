/******************************************************************************\
 * Copyright (c) 2022
 *
 * Author(s):
 *	Daz Man
 *
 * Description:
 *	WFText.cpp - Text to Waterfall converter by Daz Man 2022
 *  ASCII conversion table originally from ChirpHel by DF6NM.
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

//Text to Waterfall converter by Daz Man 2022
//The chirp code was based on ChirpHel by DF6NM, but totally rewritten in C++
//
//Read the text to be sent from file, or from the program variables
//"G" Good Copy <-- from file (for multilanguage support)
//"B" Bad Copy <-- from file (for multilanguage support)
//ID <-- from callsign setting
//If files are not found, use the English defaults in the program
//Read all the text into a string buffer
//Convert the characters to pixels
//Convert from the bottom row to the top row, one row at a time, in a certain width
//Repeat each character pixel N times to make the characters wider and/or taller
//Fill the output buffer with all the converted text
//Read all the data until it has all been sent, then switch back to receive
//
//Ideally, the text input would happen in Dialog.cpp and the conversion to tones would happen in DRMsignalIO.cpp..?
//

#include "WFText.h"
#include "common/DrmTransmitter.h"
#include "common/libs/callsign.h"

#define SCALE 120
#define MAXSIZE 9; //Maximum text length per line - change scale for more characters, or add code for multiline text
CVector<LONG64> bitmap; //bitmap array is an 8-byte array (64 bits) that will store 1 bit packed pixel patterns
//CVector<short> audio; //Chirped audio samples
CComplexVector audio; //need a complex vector for easy IQ processing

#define TBsize 1200
#define TSsize (TBsize/80)*SCALE
CRealVector	textLPF(TBsize); //buffer to resample the text
CRealVector	textLPFs(TSsize); //buffer to resample the text

CRealVector rvecT(FILTER_TAP_NUMT); //text resampling filter
CRealVector	rvecZT(FILTER_TAP_NUMT - 1); //state memory

CRealVector paintbuffer;

CFftPlans				FftPlanT;
CComplexVector			veccFFTInputT;
CComplexVector			veccFFTOutputT;
int						TextDFTSize;

int readout = 0;

CComplex SinStep; //for frequency mixing IQ output to a 0Hz IF
CComplex Rotate; //for frequency mixing IQ output to a 0Hz IF

int WFtext(int select) {

    FILE* set = nullptr;
    int i = 0;
    int j = 0;
    int readsize = 0;
    const int maxsize = MAXSIZE;
    string textbuffer;
    int result = 0;

    if (readout == 0) {

        float mixfreq = (_REAL)2.0 * crPi * 1640 / 12000;

        //Rotation vector freq shift
        _COMPLEX SinStep = _COMPLEX(cos(mixfreq), sin(mixfreq));

        //Start with phase null (can be arbitrarily chosen)
        _COMPLEX Rotate = (_REAL)1.0;


        //select = which message to send
        if (select < 2) {        //Good from file
                //read the string to send into the string variable

            //open the file
            if (select == 0) {
                set = fopen("good.txt", "rb"); //read binary
            }
            else {
                //select must be 2
                set = fopen("bad.txt", "rb"); //read binary
            }
            if (set != nullptr)
            {
                //obtain file size
                fseek(set, 0, SEEK_END);
                readsize = ftell(set);
                if (readsize > maxsize) readsize = maxsize; //limit to MAXSIZE
                textbuffer.resize(readsize); //make buffer big enough
                rewind(set);

                for (i = 0; i < readsize; i++) {
                    //read entire file into buffer
                    result = fread(&textbuffer, 1, readsize, set);
                }
                fclose(set); //close file if it was open DM
            }
            else {
                //if files are not open, just use default text
                if (select == 0) textbuffer = "GOOD COPY";
                if (select == 1) textbuffer = "BAD COPY";
            }
        }
        else if (select == 2) {
            //Use callsign string
            //read the string to send into the string variable
            textbuffer = getcall(); //get callsign char array
        }
        //add code to skip spaces at word ends and centre text TODO

        //Format text here
        //for starters, it can be just one line
        //later, it could be made multi-line
        //So for now, limit it to 9 characters or so

        //Load the text here or put this in Dialog.cpp

        //build bitmap array one line of text at a time
        //break words at spaces or buffer end if needed TODO
        readsize = textbuffer.size();
        //bitmap array is a 1D buffer, accessed as a 2D array using offsets
        //each line of text is the same width, so pad the end spacings to centre the text TODO
        //each char in the bitmap needs to be 8 pixels wide
        //each char that isn't last needs 1 pixel space
        //last char needs no space TODO

        //readsize is how many characters
        bitmap.resize(readsize * 8); //total size computation ** round this up to a multiple of the max width in cols?

        //character mask is scanned from bottom to top, left to right
        //original was in ASCII 1 and 0 which is one char per pixel
        //converted array is in hexadecimal which is one bit per pixel

        //loop through all characters in the text buffer to convert them all into a bitmap
        for (i = 0; i < maxsize; i++) {
            if (i < readsize) {
                unsigned char character = textbuffer.at(i); //get an ASCII character to encode
                character = min(max(character - 32, 0), 95); //don't go negative, or above 95
                bitmap.at(i) = (LONG64)chrmask[character];
            }
            else bitmap.at(i) = 0;
            //look it up in the mask array by subtracting 32 from the value (64 bit 8x8 bitpattern)
            //leave it in the original 64 bits form for simplicity
        }

        float v = 0.0;
        LONG64 b = 0;
        unsigned int x = 0;
        unsigned int y = 0;
        unsigned int pw = 0;
        for (y = 0; y < 8; y++) { //8 Y bits for rows
            for (i = 0; i < maxsize; i++) { //9 chars unless scaled differently
                (LONG64)b = (LONG64)bitmap.at(i); //grab a 64 bit char - bitmap array holds each ASCII character converted to a 64 bit pattern
                for (x = 0; x < 8; x++) { //take 1 bit per 8
                    v = (b >> (((7 - x) * 8) + y)) & 1;
                    pw = (((7 - y) * 80) + (i * 8)) + x;
                    
                    //textLPF array is at least 8 * 10 * 8 = 640 long, and each row is 80 pixels long
                    textLPF[pw] = v * 8; //scale up the pixel amplitudes from 1
                }
            }
        }

        //try filtering and resampling the bitmap buffer for scaling
        for (int i = 0; i < FILTER_TAP_NUMT; i++) rvecT[i] = filter_tapsT[i]; //write filter coeffs
        FIRFracDec(rvecT, textLPF, textLPFs, rvecZT);

        paintbuffer.Init(TSsize); //needs to be big enough to hold entire send data

        for (i = 0; i < TSsize; i++) {
            paintbuffer[i] = max((textLPFs[i] * 1) - 0.0, 0);
        }

        //use IFFT code here and convert text bitmap to audio tones

        /* Init plans for FFT (faster processing of Fft and Ifft commands) */
        TextDFTSize = 512; //512 seems to work best
        FftPlanT.Init(TextDFTSize);

        veccFFTInputT.Init(TextDFTSize, (CReal)0.0); //clear unused FFT bins
        veccFFTOutputT.Init(TextDFTSize); //init output buffer

        //audio.resize(48000); //doesn't work with CComplexVector
        audio.Init(48000);
        int audiowrite = 0;

        //8 rows
        for (int r = 0; r < 8; r++) {

            //repeat each row for vertical stretching
#define ROWREPEAT 10
            for (int t = 0; t < ROWREPEAT; t++) {

#define START 14 //FFT bin to start from - aligns first character

                int ii = START;
                for (i = 0; i < SCALE; i++) //80 column pixels per row
                {
                    //read a pixel
                    float a = paintbuffer[(r * SCALE) + i];

                    //FFT only handles one expanded row at a time
#define WIDE 1
                    for (int w = 0; w < WIDE; w++) {
                        float ph = (crPi / TextDFTSize) * ((ii * ii) * 4); //Chirp the FFT input for best results
                        veccFFTInputT[ii] = CComplex(cos(ph) * a, sin(ph) * a);
                        ii++;
                    }
                }
                /* Calculate inverse fast Fourier transformation */
                veccFFTOutputT = Ifft(veccFFTInputT, FftPlanT);

                /* Copy complex FFT output in output buffer and scale */

                /*
                //ensure the buffer is large enough - doesn't work with CComplexVector.. add resize to class?
                if (audiowrite + TextDFTSize > audio.size()) {
                    audio.resize(audio.size() + TextDFTSize);
                }
                */

                for (i = 0; i < TextDFTSize; i++) {
                    audio[audiowrite] = veccFFTOutputT[i];
                    audiowrite++;
                }
            }
        }

        //Feed into PAPR clipper
        //This needs a 0Hz IF
        //so frequency mix the FFT IQ output down to 0Hz
        for (i = 0; i < audio.GetSize(); i++) {
            audio[i] = audio[i] * Conj(Rotate);
            Rotate *= SinStep;
        }
    }
    return 0;
}



