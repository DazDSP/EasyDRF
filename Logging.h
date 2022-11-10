#pragma once
void LogData(char*,bool);
extern char DMfilename[260];
extern float DMSNRaverage;
extern float DMSNRmax;
extern int DMobjectnum;
extern int DMrxokarray[50];
extern float DMSNRavarray[50];
extern float DMSNRmaxarray[50];
extern unsigned int DMgoodsegsarray[50];
extern unsigned int DMtotalsegsarray[50];
extern unsigned int DMpossegssarray[50];
extern unsigned int actsize; //Decoder active segment count global
extern unsigned int totsize; //Decoder total segment count global
extern unsigned int actpos; //Decoder actual position global
