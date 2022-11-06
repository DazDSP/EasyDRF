 /******************************************************************************\
 * Copyright (c) 2004
 *
 * Author(s):
 *	Francesco Lanza
 *  Daz Man 2021
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
//#define WIN32_LEAN_AND_MEAN        //Added DM
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <conio.h>
#include "bsr.h"
#include "libs/callsign.h"

//extern CDRMTransmitter	DRMTransmitter; //edited DM


//Maybe these should be in Dialog.cpp or Dialog.h ??
//These appear to be for multi-window BSR requests
HWND bsrhwnd[NO_BSR_WIND]; //edited DM
BOOL bsr_onscreen_arr[NO_BSR_WIND]; //edited DM
int hasharr[NO_BSR_WIND]; //edited DM
string bsrcall[NO_BSR_WIND]; //edited DM

HWND txbsrhwnd[NO_BSR_WIND]; //edited DM
BOOL txbsr_onscreen_arr[NO_BSR_WIND]; //edited DM
int  txhasharr[NO_BSR_WIND]; //edited DM

string sentfilenamearr[numstredele] = { " " };
string sentfilenamenodirarr[numstredele] = { " " };
int sentfilehasharr[numstredele] = { 0 };
//CVector<short>  vecsToSendarr[numstredele];
int sentfilept = 0;

void storesentfilename(string the_filename, string the_filenamenodir, int hashval)
{
	if (the_filename.size() == 0) return;
	if (sentfilehasharr[sentfilept] != hashval)
	{
		sentfilept++;
		if (sentfilept >= numstredele) sentfilept = 0;
	}
	sentfilenamearr[sentfilept] = the_filename;
	sentfilenamenodirarr[sentfilept] = the_filenamenodir;
	sentfilehasharr[sentfilept] = hashval;
}

string idfilename;
string idfilenamenodir;

CVector<short>  vecsToSend;

void getsentfilename(int hashval)
{
	int i;
	for (i=0;i<numstredele;i++)
	{
		if (sentfilehasharr[i] == hashval)
		{
			idfilename = sentfilenamearr[i];
			idfilenamenodir = sentfilenamenodirarr[i];
			return;
		}
	}
	idfilename = ' ';
	idfilenamenodir = ' ';
}

int segsize;

string readbsrfile(char * path)
{
	FILE * bsr = nullptr; //inits DM
	int trid = 0;
	char filenam[300]{};
	char rubbish[100]{};
	int segno = 0;
	int vct = 0;

	wsprintf(filenam, "%s%s", path, "bsrreq.bin");
	bsr = fopen(filenam,"rt");

	if (bsr != NULL)
	{
		vecsToSend.Init(0);
		fscanf(bsr,"%d",&trid);
		fscanf(bsr,"%s",&rubbish);
		fscanf(bsr,"%d",&segsize);
		while (!feof(bsr))
		{
			fscanf(bsr,"%d",&segno);
			if (segno == -99) 
			{
				while (!feof(bsr)) fgetc(bsr);
			}
			else
			{
				vecsToSend.Enlarge(1);
				vecsToSend[vct] = segno;
				vct++;
			}
		}
		fclose(bsr);
		getsentfilename(trid);
		return idfilenamenodir;
	}
	return "";
}

int segnobsrfile()
{
	return vecsToSend.Size();
}

void compressBSR(char * filenamein, char * filenameout)
{
	FILE * inf = nullptr; //init DM
	FILE * outf = nullptr; //init DM
	char tmpstr[100]{}; //init DM
	int segno = 0;
	int lastseg = -9;
	int consct = 0;
	int data = 0;
	BOOL stopcompress = FALSE;

	inf = fopen(filenamein,"rt");
	outf = fopen(filenameout,"wt");

	fscanf(inf,"%s",&tmpstr); //all changed to fscanf_s DM
	fprintf(outf,"%s\n",tmpstr);
	fscanf(inf,"%s",&tmpstr);
	fprintf(outf,"%s\n",tmpstr);
	fscanf(inf,"%s",&tmpstr);
	fprintf(outf,"%s\n",tmpstr);


	while (!feof(inf))
	{
		fscanf_s(inf,"%d",&segno);
		if (segno == -99) 
		{
			fprintf(outf,"%d\n",lastseg);
			while (!feof(inf))
			{
				data = fgetc(inf);
				if (!feof(inf)) fputc(data,outf);
			}
		}
		else
		{
			if (segno == lastseg+1)	
				consct++;
			else			
			{
				if (consct == 1)
				{
					fprintf(outf,"%d\n",lastseg);	
					consct = 0;
				}
				else if (consct >= 2)
				{
					fprintf(outf,"%d\n",-1);	
					fprintf(outf,"%d\n",lastseg);	
					consct = 0;
				}
				if (lastseg != segno) fprintf(outf,"%d\n",segno);
			}
			lastseg = segno;
		}
	}

	fclose(inf);
	fclose(outf);
}

void decompressBSR(char * filenamein, char * filenameout)
{
	FILE * bsrin = nullptr;
	FILE * bsrout = nullptr;

	bsrin = fopen(filenamein,"rt");
	bsrout = fopen(filenameout,"wt");

	// decompress
	if (bsrin != nullptr) 
	{
		int segno = 0,oldsegno = 0;
		int par1 = 0;
		char par2[100];
		int data = 0;

		fscanf(bsrin,"%d",&par1);
		fprintf(bsrout,"%d\n",par1);
		fscanf(bsrin,"%s",&par2);
		fprintf(bsrout,"%s\n",par2);
		fscanf(bsrin,"%d",&par1);
		fprintf(bsrout,"%d\n",par1);

		while (fscanf(bsrin,"%d",&segno) != EOF)
		{
			if (segno == -99)
			{
				fprintf(bsrout,"%d\n",segno);
				while (!feof(bsrin))
				{
					data = fgetc(bsrin);
					if (!feof(bsrin)) fputc(data,bsrout);
				}
			}
			else
			{
				if (segno == -1)
				{
					fscanf_s(bsrin,"%d",&segno);
					if (segno > oldsegno)
					{
						int i;
						for (i=oldsegno+1;i<=segno;i++)
						{
							fprintf(bsrout,"%d\n",i);
						}
					}
				}
				else
				{
					fprintf(bsrout,"%d\n",segno);
				}
				oldsegno = segno;
			}
		}
		fclose(bsrin); //moved these inside the if, so they are only closed if they are opened DM
		fclose(bsrout);
	}
}

void writeselsegments(int num = 1)
{
		int noseg = vecsToSend.Size();
		DRMTransmitter.GetParameters()->iNumAudioService = 0;
		DRMTransmitter.GetParameters()->iNumDataService = 1;
		DRMTransmitter.GetParameters()->Service[0].eAudDataFlag = CParameter::SF_DATA;
		DRMTransmitter.GetParameters()->Service[0].DataParam.iStreamID = 0;
		DRMTransmitter.GetParameters()->Service[0].DataParam.eDataUnitInd = CParameter::DU_DATA_UNITS;
		DRMTransmitter.GetParameters()->Service[0].DataParam.eAppDomain = CParameter::AD_DAB_SPEC_APP;
		DRMTransmitter.GetParameters()->Service[0].iServiceDescr = 0;
		DRMTransmitter.Init();
		DRMTransmitter.GetParameters()->Service[0].DataParam.iPacketLen = calcpacklen(DRMTransmitter.GetParameters()->iNumDecodedBitsMSC); 
		DRMTransmitter.GetAudSrcEnc()->ClearPicFileNames();

		//Added DM
		//Trim the .lz from BSR request filenames for on-the-fly compressed files DM
		//This actually cuts the specified char array short by terminating with a zero byte (no copy is made)
		unsigned int tlen = idfilenamenodir.length() - 3;
		if (stricmp(&idfilenamenodir[tlen], ".lz") == 0) {
			idfilenamenodir[tlen] = 0; //terminate the string early to cut off the extra .lz extension DM
		}

		DRMTransmitter.GetAudSrcEnc()->SetPicFileName(idfilename, idfilenamenodir,vecsToSend);
		if (num >= 2)	// more instances
			DRMTransmitter.GetAudSrcEnc()->SetPicFileName(idfilename,idfilenamenodir,vecsToSend);
		if (num >= 3)	// more instances
			DRMTransmitter.GetAudSrcEnc()->SetPicFileName(idfilename,idfilenamenodir,vecsToSend);
		if (num >= 4)	// more instances
			DRMTransmitter.GetAudSrcEnc()->SetPicFileName(idfilename,idfilenamenodir,vecsToSend);
		if (num != 0)	// dont with 0 inst
		{
			if (noseg <= 50)	// two instances if few segments
				DRMTransmitter.GetAudSrcEnc()->SetPicFileName(idfilename,idfilenamenodir,vecsToSend);
			if (noseg <= 10)	// three instances if few segments
				DRMTransmitter.GetAudSrcEnc()->SetPicFileName(idfilename,idfilenamenodir,vecsToSend);
			if (noseg <=  3)	// five instances if few segments
				DRMTransmitter.GetAudSrcEnc()->SetPicFileName(idfilename,idfilenamenodir,vecsToSend);
		}
}

