/******************************************************************************\
 * Copyright (c) 2022
 *
 * Author(s):
 *	Daz Man
 *
 * Description:
 *  WFText.cpp - Text to Waterfall converter by Daz Man 2022
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

#include "WFText.h"
#include "common/DrmTransmitter.h"
#include "common/libs/callsign.h"

#define MAXSIZE 20; //Maximum text length per line
//CVector<short> audio; //Chirped audio samples
CComplexVector audio; //need a complex vector for easy IQ processing
int readout = 0;

void WFtext(int select) {

    if (select == 99) {
        readout = 0;
        paintmode = 0;
    }
    else {
        FILE* set = nullptr;
        int i = 0;
        int j = 0;
        int readsize = 0;
        const int maxsize = MAXSIZE;
        string textbuffer;
        int result = 0;

        if (readout == 0) {
            CComplex SinStep; //for frequency mixing IQ output to a 0Hz IF
            CComplex Rotate; //for frequency mixing IQ output to a 0Hz IF

            float mixfreq = (_REAL)2.0 * crPi * 1640 / 12000; //for IQ mixing output to a 0Hz IF

            //Rotation vector freq shift
            SinStep = _COMPLEX(cos(mixfreq), sin(mixfreq)); //compute rotation step

            //Start with phase null (can be arbitrarily chosen)
            Rotate = (_REAL)1.0;

            //select = which message to send
            if (select < 2) {
                //Good and Bad from text file or default text
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

                    result = fread(&textbuffer[0], 1, readsize, set);
                    fclose(set); //close file if it was open DM
                    readsize = result; //how many chars were actually read from the file
                }
                if ((set == nullptr) || (readsize == 0)) {
                    //if files are not found or the read size was zero, just use default English text
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
            //Width and height are now scaled depending on text length

            //build bitmap array one line of text at a time
            //break words at spaces or buffer end if needed TODO
            readsize = textbuffer.size();

            if (readsize > 0) {

                //bitmap array is a 1D buffer, accessed as a 2D array using offsets
                //each line of text is the same width, so pad the end spacings to centre the text TODO
                //Now text is scaled to fill the display width
                //each char in the bitmap needs to be 8 pixels wide
                //each char that isn't last needs 1 pixel space
                //last char needs no space TODO

                //readsize is how many characters
                CVector<LONG64> bitmap; //bitmap array is an 8-byte array (64 bits) that will store 1 bit packed pixel patterns
                bitmap.resize(30 * 8); //total size computation ** round this up to a multiple of the max width in cols?

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

                CRealVector paintbuffer;
                paintbuffer.Init(readsize * 8 * 8 + 1000); //needs to be big enough to hold entire send data

                //always wrap rows at a multiple of the readsize
                float v = 0.0;
                LONG64 b = 0;
                unsigned int x = 0;
                unsigned int y = 0;
                unsigned int pw = 0;
                for (y = 0; y < 8; y++) { //8 Y bits for rows
                    for (i = 0; i < readsize; i++) { //read each character
                        (LONG64)b = (LONG64)bitmap.at(i); //grab a 64 bit char - bitmap array holds each ASCII character converted to a 64 bit pattern
                        for (x = 0; x < 8; x++) { //take 1 bit per 8
                            v = (b >> (((7 - x) * 8) + y)) & 1;
                            pw = (((7 - y) * 8 * readsize) + (i * 8)) + x;

                            paintbuffer[pw] = v * 8; //scale up amplitude and write pixel into buffer
                        }
                    }
                }

#define CHIRPRATE 8 //speed of chirp modulation

                //try adapting the FFT size depending on the text width
                int TextDFTSize = 512 + max(readsize - 9, 0) * 8; //512 seems to work best on short text
                int OUTsize = TextDFTSize * 1.65; //output scale
                int INsize = (readsize * 8 * 8); //input scale
                int startpos = 3 * ((float)(INsize) / TextDFTSize) + 18; //FFT bin to start from - aligns first character
                int rowrepeat = max(min(100 / readsize, 50), 5) + 1; //compute vertical stretching

                CRealVector	textLPF(INsize); //buffer to resample the text
                CRealVector	textLPFs(OUTsize); //buffer to resample the text
                CRealVector rvecT(FILTER_TAP_NUMT); //text resampling filter
                CRealVector	rvecZT(FILTER_TAP_NUMT - 1); //state memory

                for (i = 0; i < textLPF.GetSize(); i++) {
                    textLPF[i] = 0; //clear buffer
                }

                //load the filter coeffs
                for (int i = 0; i < FILTER_TAP_NUMT; i++) rvecT[i] = filter_tapsT[i]; //write filter coeffs

                //audio.resize(48000); //doesn't work with CComplexVector
                audio.Init((TextDFTSize * 8 * rowrepeat) + 12000); //enough for one line of text only
                int audiowrite = 0; //reset audio write index

                CFftPlans				FftPlanT;
                CComplexVector			veccFFTInputT;
                CComplexVector			veccFFTOutputT;

                /* Init plans for FFT (faster processing of Fft and Ifft commands) */
                FftPlanT.Init(TextDFTSize);

                veccFFTInputT.Init(TextDFTSize, (CReal)0.0); //clear unused FFT bins
                veccFFTOutputT.Init(TextDFTSize); //init output buffer

                //there are as many rows as there are vertical pixels (8)
                for (int row = 0; row < 8; row++) {

                    //repeat each row for vertical stretching
                    for (int t = 0; t < rowrepeat; t++) {

                        //there are 8 pixels per character
                        //so read in a row of pixels, then resize it to the output size and output it to the IFFT
                        //for 9 chars, the row length is 72 pixels
                        for (int pix = 0; pix < (8 * readsize); pix++) {
                            //pixel start index is row * (8 * readsize)
                            //pixel index is (row * (8 * readsize)) + pix
                            //read a row of pixels from the paintbuffer and write it into textLPF
                            textLPF[pix] = paintbuffer[(row * 8 * readsize) + pix];
                        }
                        //scale the row
                        textLPFs = FIRFracDec(rvecT, textLPF, textLPFs, rvecZT);
                        //perform IFFT on the row
                        //phase chirp all the pixel data one pixel at a time
                        for (int ifft = startpos; ifft < TextDFTSize - startpos; ifft++) {
                            int iffts = ifft - startpos;
                            float a = textLPFs[iffts]; //read a pixel
                            //float a2 = a * 18 / sqrt(iffts + 256); //Does this help even the amplitudes?
                            float ph = (crPi / TextDFTSize) * ((ifft * ifft) * CHIRPRATE); //Chirp the FFT input for best results
                            veccFFTInputT[ifft] = CComplex(cos(ph) * a, sin(ph) * a);
                        }
                        veccFFTOutputT = Ifft(veccFFTInputT, FftPlanT); //perform IFFT

                        //add new audio to output buffer
                        for (i = 0; i < TextDFTSize; i++) {
                            audio[audiowrite] = veccFFTOutputT[i];
                            audiowrite++;
                        }
                    }
                }


                /*
                //ensure the buffer is large enough - doesn't work with CComplexVector.. add resize to class?
                if (audiowrite + TextDFTSize > audio.size()) {
                    audio.resize(audio.size() + TextDFTSize);
                }
                */

                //Feed into PAPR clipper
                //This needs a 0Hz IF
                //so frequency mix the FFT IQ output down to 0Hz

                for (i = 0; i < audio.GetSize(); i++) {
                    audio[i] = audio[i] * Conj(Rotate);
                    Rotate *= SinStep;
                }
            }
        }
    }
}
