/******************************************************************************\
 * Copyright (c) 2021
 *
 * Author(s):
 *	Daz Man
 *
 * Description:
 *	Logging.cpp - Logging of stats to a Javascript file
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

#include "logging.h"
//#include <string.h>
#include <Windows.h>
#include <xutility>
#include <cstdio>

//make a function that builds an array of data for the incoming files
//save the array data to a JS file
//the JS file can be loaded into a web page for display

char prevfilename[260] = "";

void LogData(char* fn, bool saved) {
	//fn: filename
	//saved: TRUE = record that file was saved, FALSE = do nothing
	//filenumber is derived from the filename
	//update the stats as the data comes in
	//save the data as each file saves
	// 
	//save the stats in an array first, then dump the array out to a file
	//compute stats in browser - just save the raw data here
	//use a Javascript object to hold the data
	//add an object for each file number

	//save avSNR once, smoothed over time by the decoder
	//SNRav = SNRaverage
	//SNRmax = SNRmaximum
	//ts = totalsegments
	//gs = goodsegments
	//ps = active position

	//format is SWRG-nnn-nn.ext or SWRGtest-nnn-nn.ext or RNEInn-sssss.ext or RNEInn.ext
	int mode = 0; //mode 0 = none, 1 = SWRG, 2 = RNEI or RCAR
#define SWRG 1
#define RNEI 2
	int n = 0;
	int i = 0;
	int j = 0;
	//check if it's SWRG
	char filenumber[260]; //array for both filename and filenumber computations
	strcpy(filenumber, fn);
	if (strlen(filenumber) >= 4) {
		filenumber[4] = 0; //cut short
		//n = strcmp(filenumber, "SWRG"); //Match first four letters only
		//n should = 0
		//if (n == 0) {
		//SWRG or RNEI detection
		if (strcmp(filenumber, "SWRG") == 0) { mode = SWRG; } //SWRG mode
		else if (strcmp(filenumber, "RNEI") == 0) { mode = RNEI; } //RNEI mode
		else if (strcmp(filenumber, "RCAR") == 0) { mode = RNEI; } //RNEI mode

		if (mode > 0) {
			//filename must be SWRG*.* or RNEI*.*
			//compute object number from filename - use consistent naming to make this easy
			strcpy(filenumber, fn); //get entire filename again
			//This also needs to work on the longer SWRGtest name, and any other name starting with SWRG or RNEI...

			//in case the .lz is on the filename...
			if (stricmp(&filenumber[strlen(filenumber) - 3], ".lz") == 0) {
				filenumber[strlen(filenumber) - 3] = 0; //terminate the string early to cut off the extra .lz extension
			}

			i = strlen(filenumber); //point to last char

			if (mode == SWRG) {
				//find the first "-", backwards
				while ((n != '-') && (i > 0)) {
					n = filenumber[i]; //read a char
					i--;
				}
				i++;
			}

			else if (mode == RNEI) {
				//find the first ".", backwards
				while ((n != '.') && (i > 0)) {
					n = filenumber[i]; //read a char
					i--;
				}
				i++;
			}
			else n = 0; //failed to find the character

			//now i points to the "-" (SWRG) or the "." (RNEI)
			if (n > 0) {
				//save filenumber
				//SWRG-nnn-nn.ext
				if (mode == SWRG) {
					i++; //next char
					n = min(max(filenumber[i] - 48, 0), 9) * 10; //compute tens
					i++; //next char
					n = n + min(max(filenumber[i] - 48, 0), 9); //compute units and add
					DMobjectnum = n; //save
				}
				if (mode == RNEI) { DMobjectnum = 0; } //just one file for RNEI

				//Range check - DMobjectnum must be between 0 and 29
				if ((DMobjectnum >= 0) && (DMobjectnum < 30)) {

					DMSNRavarray[DMobjectnum] = DMSNRaverage;
					DMSNRmaxarray[DMobjectnum] = DMSNRmax;

					if (saved == TRUE) {
						DMrxokarray[DMobjectnum] = 1; //record that this file DID save OK
					}

					DMtotalsegsarray[DMobjectnum] = totsize;
					DMpossegssarray[DMobjectnum] = actpos;

					if (stricmp(filenumber, prevfilename) == 0){
						//allow actsize to increase, but not decrease - because at the end of file it resets
						DMgoodsegsarray[DMobjectnum] = max(actsize, DMgoodsegsarray[DMobjectnum]);
					}
					else {
						DMgoodsegsarray[DMobjectnum] = actsize;  //if the base filename changed, just set it directly
						strcpy(prevfilename, filenumber); //update
					}
					
					if (mode == SWRG) { i -= 2; } //point to the '-' for SWRG
					//RNEI points to the "." already

					//update log file now
					//write all objects to file and update each time
					//save the data to a matching filename, but with a .js extension
					filenumber[i] = '.'; //overwrite - with . 
					i++; //next char
					filenumber[i] = 'j'; //add js extension
					i++; //next char
					filenumber[i] = 's';
					i++; //next char
					filenumber[i] = 0; //terminate filename

					//log file is opened here for writing
					char logfile[260];
					//add path
					wsprintf(logfile, "Rx Files\\%s", filenumber);

					FILE* set = nullptr;
					if ((set = fopen(logfile, "wb")) == nullptr) {
					// handle error here DM
					}
					else {
						int err = 0;

						//do this a set number of times so no data is lost if something repeats
						int max = 0; //default to one file for RNEI
						if (mode == SWRG) { max = 20; } //for SWRG save 20 entries to allow for many image files

						for (i = 0; i <= max; i++) {
							//reuse the filenumber array to hold data
							err = sprintf_s(filenumber, "data%02d={ok:%d,SNRav:%2.1f,SNRmax:%2.1f,ts:%d,gs:%d,ps:%d};\r\n", i, DMrxokarray[i], DMSNRavarray[i], DMSNRmaxarray[i], DMtotalsegsarray[i], DMgoodsegsarray[i], DMpossegssarray[i]);
							//write each char of filenumber array temp to file
							for (j = 0; j < strlen(filenumber); j++) {
								putc(filenumber[j], set);
							}
						}
						fclose(set); //file is closed here - but only if it was opened
					}
				}
			}
		}
	}
};
	