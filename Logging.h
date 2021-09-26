#pragma once
void LogData(char*);
extern char DMfilename[260];
extern float DMSNRaverage;
extern float DMSNRmax;
extern int DMobjectnum;
extern int DMrxokarray[50];
extern float DMSNRavarray[50];
extern float DMSNRmaxarray[50];
extern int DMgoodsegsarray[50];
extern int DMtotalsegsarray[50];
extern int DMpossegssarray[50];
extern int actsize; //Decoder active segment count global
extern int totsize; //Decoder total segment count global
extern int actpos; //Decoder actual position global
