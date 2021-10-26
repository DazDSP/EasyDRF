/******************************************************************************\
 * Copyright (c) 2020
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
//after each file ends, save the array data to a JS file
//the JS file can be loaded into a web page for display

//void LogData(unsigned int SNRaverage, unsigned int SNRmaximum, unsigned int goodsegments, unsigned int totalsegments) {
void LogData(char* fn) {
	//filenumber is derived from the filename - what if it doesn't arrive until the end? only execute this on save
	//save the data as each file saves

	//all the stats need to be saved in an array first, then dump the array out to the file
	//use an object to hold the data
	//add an object for each file number - can a JS number be an object? might need an alpha prefix!
	//compute stats in browser - just save the raw data here

	//save avSNR once, smoothed over time by the decoder
	//SNRav = SNRaverage
	//SNRmax = SNRmaximum
	//gs = goodsegments
	//ts = totalsegments
	//ps = active position

	//format is SWRG-nnn-nn.ext or SWRGtest-nnn-nn.ext
	int n = 0;
	int i = 0;
	int j = 0;

	//check if it's SWRG
	char filenumber[260];
	strcpy(filenumber, fn);
	if (strlen(filenumber) >= 4) {
		filenumber[4] = 0; //cut short
		n = strcmp(filenumber, "SWRG"); //Match first four letters only
		//n should = 0
		if (n == 0) {
			//compute object number from filename - use consistent naming to make this easy
			strcpy(filenumber, fn);
			//This also needs to work on the longer SWRGtest name, and any other name starting with SWRG...
			i = strlen(filenumber); //point to last char
			//find the first "-", backwards
			while ((n != '-') && (i > 0)) {
				n = filenumber[i]; //read a char
				i--;
			}
			i++;
			n = 0;
			//now i points to the "-"
			if ((filenumber[i] == '-') && (i < strlen(filenumber))) {
				//save filenumber
				//SWRG-nnn-nn.ext
				i++; //next char
				n = min(max(filenumber[i] - 48, 0), 9) * 10; //compute tens
				i++; //next char
				n = n + min(max(filenumber[i] - 48, 0), 9); //compute units and add
				DMobjectnum = n; //save

				if (DMobjectnum > 29) { DMobjectnum = 29; } //bounds limit array index
				DMSNRavarray[DMobjectnum] = DMSNRaverage;
				DMSNRmaxarray[DMobjectnum] = DMSNRmax;
				
				DMrxokarray[DMobjectnum] = 1; //record that this image DID save OK

				DMgoodsegsarray[DMobjectnum] = actsize;
				DMtotalsegsarray[DMobjectnum] = totsize;
				DMpossegssarray[DMobjectnum] = actpos;

				i -= 2; //point to the '-'
				//update log file now
				//write all objects to file and update each time
				//save the data to a matching filename, but with a .js extension
				if (strlen(filenumber) >= 11) {
					filenumber[i] = '.'; //overwrite - with .
					i++; //next char
					filenumber[i] = 'j'; //add js extension
					i++; //next char
					filenumber[i] = 's';
					i++; //next char
					filenumber[i] = 0; //terminate filename
				}
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
					if (DMobjectnum == 1) {
						DMobjectnum = DMobjectnum; //debug breakpoint
					}
					/*
					//add a var to show the number of vars
					char vv[20]{ 0 };
					err = sprintf_s(vv, "datann=%d;\r\n", DMobjectnum);
					for (j = 0; j < strlen(vv); j++) {
						putc(vv[j], set);
					}
					*/

					//do this DMobjectnum times
					for (i = 0; i < DMobjectnum +1; i++) {
						//reuse the filenumber array to hold data
						err = sprintf_s(filenumber, "data%02d={ok:%d,SNRav:%2.1f,SNRmax:%2.1f,gs:%d,ts:%d,ps:%d};\r\n", i, DMrxokarray[i], DMSNRavarray[i], DMSNRmaxarray[i], DMgoodsegsarray[i], DMtotalsegsarray[i], DMpossegssarray[i]);
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
};
	