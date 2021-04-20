#ifndef REEDSOLOMONCODER_H
#define REEDSOLOMONCODER_H
//#include "appglobal.h"
//#include "appdefs.h"
//#include <QFile>
//#include <QByteArray>

enum eRSType {RSTNONE,RST1,RST2,RST3,RST4};

extern char* rsTypeStr[RST4+1];

union long_byte_union
{
        int64_t i;
        unsigned char b[8];
};


class reedSolomonCoder
{
public:
  reedSolomonCoder();
  ~reedSolomonCoder();
  void init();
  bool decode(char* &ba, char* fn, char* &newFileName, char* &baFile, char* extension, int* &erasuresArray);
  bool encode(char*&ba, char* extension, eRSType rsType);
private:
  void distribute(char*src, char*dst, int rows, int cols, int reverse);
  bool decode_and_write();
  char* ec_buf;  /* pointer to encoding/decoding buffer */
  char* tr_buf;  /* pointer to transmit-buffer (fread/fwrite) */
  char* bk_buf; /* pointer to backup-buffer for resync */
  int rs_bsize;
  int rs_dsize;
  int bep_size;
  unsigned long sumOfFailures;
  unsigned long uncorrectableFailures;
//  int k;
  FILE fpin, fpout;
  long got,chunks;
  int coded_file_size ;
  char coded_file_ext[4] ;
  char* origFileName;
//  char *p ;
  eRSType fileType;
  int totalSegments;
  int segmentLength;
  int *zeroPositions;
  int *newZeroPositions;
  int numMissing;
};

#endif // REEDSOLOMONCODER_H

