/***************************************************************************
    rs1encode.c  -  Reed-Solomon & Burst Error Protection
                    -------------------------------------
    begin                : Mon Jul  30 20:27:09 MEST 2001
    copyright            : (C) 2000 by Guido Fiala
    email                : gfiala@s.netic.de
   
 ***************************************************************************/

/***************************************************************************
    Slightly modified by ttsiodras at removethis gmail dot com

    It doesn't write the 3 parameters of rsbep as a single line before the 
    data, as this makes the stream fragile (if this information is lost, 
    decoding fails)

    It uses a default value of 16*255=4080 for parameter D, and it can thus 
    tolerate 4080*16=65280 consecutive bytes to be lost anywhere in the 
    stream, and still recover...
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
/******************************************************************
  CHANGED TO ENCODE 224,255 ONLY 	
  BY Ties bos - PA0MBO
  date March 28th 2012
******************************************************************/
/***********************************************************************
  Being modified by Daz Man 2021 to encode all RS modes used by EasyPal,
  By setting a memory variable for the RS mode to use.
***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> 
#include <string.h>
#include <assert.h>

#include "rs.h"       
#define rse32 encode_rs       
#define rsd32 eras_dec_rs

#define ERR_BURST_LEN (255*16)   //default, 4080
#define RS_BSIZE 255            //default
//#define RS_DSIZE 224            //default was 191
//RS1 = 224
//RS2 = 192
//RS3 = 160
//RS4 = 128
#define RSBEP_VERSION "0.0.3"
#define RSBEP_HDR "rsbep"
#define MAX_FILE_SIZE 20000000 

//just a "large" random string because it's so unlikely to be in the data by chance - better idea?
#define RSBEP_MAGIC_NUM "InÇÊ˜8˜¬R7ö¿ç”S¡"

#define PHDR_LEN  54    // (40+strlen(RSBEP_MAGIC_NUM)) //maximum expected len of header line...


union long_byte_union
{
        long long i;
        char b[8];
};

//this function is used to copy src to dst while separating consecutive bytes
//equally distributed over the entire array, it tries to utilize several
//cpu-pipes and as high as possible memory efficiency without using asm.
//(but not several CPU's...anyway -it's only 15% of CPU-time, most is RS)

void distribute(dtype *src, dtype *dst, int rows, int cols, int reverse)
{
        register long i,j,rc,ri,rl;
        union long_byte_union lb[8];
        rc=rows*cols;
        ri=0;
        rl = reverse ? cols : rows;
        for(i=0;i<rc;i+=64) //we read 64 byte at once
        {
                lb[0].i=*((long long*)(src+i));    
                lb[1].i=*((long long*)(src+i+8));  
                lb[2].i=*((long long*)(src+i+16)); 
                lb[3].i=*((long long*)(src+i+24)); 
                lb[4].i=*((long long*)(src+i+32)); 
                lb[5].i=*((long long*)(src+i+40)); 
                lb[6].i=*((long long*)(src+i+48)); 
                lb[7].i=*((long long*)(src+i+56)); 
                for(j=0;j<64 && i+j<rc;j++) 
                {       //we let the compiler optimize this loop...
                        //branch prediction should kill most the overhead...
                        *(dst+ri)=lb[0].b[j];//write byte in dst
                        ri+=rl;//next position in dst
                        if(ri>=rc)ri-=rc-1;//go around and add one.
                }
        }
}

//decode buffer and write to stdout
void decode_and_write(char* ec_buf,int rs_bsize,int bep_size,int rs_dsize,long* sum_of_failures,
                             long* uncorrectable_failures,char* argv,int quiet,int verbose, FILE *fpout)
{
        register int i;
        int eras_pos[32]; //may once hold positions of erasures...
        for(i=0;i<bep_size;i++)
        {
                int failure=rsd32((ec_buf+i*rs_bsize),eras_pos,0);
                if (failure>0)
                {
                        *sum_of_failures+=failure;
                        if(!quiet && verbose) fprintf(stderr,"%s: errors corrected: %ld uncorrectable blocks: %ld\r",
                                                      argv,*sum_of_failures,*uncorrectable_failures);
                }
                else if (failure==-1)
                {       
                        *uncorrectable_failures++;
                        if(!quiet && verbose) fprintf(stderr,"%s: errors corrected: %ld uncorrectable blocks: %ld\r",
                                                      argv,*sum_of_failures,*uncorrectable_failures);
                }
                if (rs_dsize!=fwrite((ec_buf+i*rs_bsize), sizeof(dtype),rs_dsize, fpout))
                {       //write decoded data
                        perror("serious problem: ");
                        exit(-1);
                }
        }
}                       

//resync-function
//is searches tr_buf for the header-string, copies the data after into bk_buf
//zeroes tr_buf from header string, decodes tr_buf, writes tr_buf to stdout
//copies bk_buf to tr_buf and fills it up, checks if we are in synch again
//if yes decode and exit else loop.
void resync(char* tr_buf,char* ec_buf, char* bk_buf,int len,char* phdr,int bep_size,int rs_bsize, int rs_dsize, 
                   int quiet, int verbose, int override, long* sum_of_failures, long* uncorrectable_failures, char* argv, FILE *fpin, FILE *fpout)
{
        int i,j, k;
        char hdr[PHDR_LEN];//just to hold the header line
        int got, aa, bb;
        int rsb,rsd,bep;
        char nam[PHDR_LEN];
	char mag[PHDR_LEN];
	fprintf(stderr,"entering resynch\n");fflush(stderr);//debug only
        start_over://label for the ugly goto below...
        if(verbose && !quiet) fprintf(stderr,"%s: trying to resync on data...\n", argv);
        //search for the magic num (can be anywhere!)
        for(i=0;i<len-15;i++) //if the magic is broken over the buffer-end, we loose another frame...
        {
                if(0==strncmp(tr_buf+i,RSBEP_MAGIC_NUM,strlen(RSBEP_MAGIC_NUM)))
                { //found magic, check for string "rsbep"
			if(verbose && !quiet) fprintf(stderr,"magic found. \n");
                        for(j=i;j>i-PHDR_LEN;j--)
                        {
                                if(0==strncmp(tr_buf+j,RSBEP_HDR,strlen(RSBEP_HDR)))
                                {
					if(verbose && !quiet) fprintf(stderr,"rspeb-header found. \n");
                                        aa = len-i-9;
                                        bb = len-j;
                                        //copy data into bk_uf
                                        memcpy(bk_buf,tr_buf+i+9,aa);
                                        //zero tr_buf
										for (k=j; k < j+ bb; k++ )
                                           tr_buf[k] =0;
                                       // bzero(tr_buf+j,bb);
                                        //decode tr_buf
                                        distribute(tr_buf,ec_buf,bep_size,rs_bsize,1);
                                        //write to stdout
                                        decode_and_write(ec_buf,rs_bsize,bep_size,rs_dsize,sum_of_failures,
                                                         uncorrectable_failures,argv, quiet,verbose, fpout);
                                        //copy bk_buf
                                        memcpy(tr_buf,bk_buf,aa);
                                        //fillup tr_buf (until EOF)
                                        got=fread(tr_buf+aa,1,bep_size*rs_bsize-aa,fpin);//read the encoded data
                                        if (got < (bep_size*rs_bsize-aa))
                                        {       //can only happen, when data at end of stream are lost, zero missing data
						if(verbose && !quiet) fprintf(stderr,"eof-data decoded. \n");
                                                // bzero((tr_buf+got),(bep_size*rs_bsize)-got);
						                       for (k=got; k < bep_size*rs_bsize; k++)
												        tr_buf[k] = 0;
                                                distribute(tr_buf,ec_buf,bep_size,rs_bsize,1);
                                                decode_and_write(ec_buf,rs_bsize,bep_size,rs_dsize,sum_of_failures,
                                                                 uncorrectable_failures,argv, quiet,verbose, fpout);
                                                return;//no more data
                                        }
                                        else
                                        {
                                                //Check if we now get an rsbep-buffer-stream at stdin with block-sync
                                                phdr=fgets(hdr,PHDR_LEN-1,stdin);
                                                if(phdr==NULL)
                                                {
							return;//no more data
                                                }
                                                else
                                                {       //on we go:
                                                        sscanf(phdr,"%9s %d %d %d %20s",&nam,&rsb,&rsd,&bep,&mag);
                                                        if( 0!=strncmp(RSBEP_HDR,nam,strlen(RSBEP_HDR)) ||
							    0!=strncmp(RSBEP_MAGIC_NUM,mag,strlen(RSBEP_MAGIC_NUM)) || 
                                                            (!override && (rs_bsize!=rsb || rs_dsize!=rsd || bep_size!=bep)) )
                                                        {       //again, the evil thing:
                                                                //would be recursive call of resync, we have to avoid it by a ugly:
                                                                //(the fgets above has probably read in the header into tr_buf)
								if(verbose && !quiet) fprintf(stderr,"\nstartover. ");
                                                                goto start_over;
                                                        }
                                                        else
                                                        {       //we are through the bad data for now:
                                                                //normal decode and write
								if(verbose && !quiet) fprintf(stderr,"successful resynched.\n");								
                                                                distribute(tr_buf,ec_buf,bep_size,rs_bsize,1);
                                                                decode_and_write(ec_buf,rs_bsize,bep_size,rs_dsize,sum_of_failures,
                                                                                 uncorrectable_failures,argv,quiet,verbose, fpout);
                                                        }
                                                }
                                        }
                                        continue;//for-j
                                }
                        }
                        continue;//for-i
                }               
        }
        //no magic found, just go on - what else can we do...
	fprintf(stderr,"leaving resynch.\n");
}


/* reversestring not avaliable in gcc */
char * revstring( char * inputstr)
{
   int i,j;
   char tempchar;
   i=0;
   j = strlen(inputstr) - 1;
   while ( i < j )
   {
      tempchar = inputstr[i];
      inputstr[i++] = inputstr[j];
      inputstr[j--] = tempchar;
   }
}

//main-program: reads commandline args and does anything else...

int main(int argc,char *argv[])
{
  dtype *ec_buf = NULL;//pointer to encoding/decoding buffer
  dtype *tr_buf = NULL;//pointer to transmit-buffer (fread/fwrite)
  dtype *bk_buf = NULL;//pointer to backup-buffer for resync
  char hdr[PHDR_LEN];//just to hold the header line
  char *phdr=NULL;//holds return value of fgets()
  char name[PHDR_LEN];//will hold the name in header
  char mag[PHDR_LEN];//for magic string
  long verbose=0,decode=0,quiet=0,temp=0,override=0;
  long rs_bsize=0,rs_dsize=0,bep_size=0;
  long i=0;
  long sum_of_failures=0;
  long uncorrectable_failures=0;
  char *optarg;
  int k;
  FILE *fpin, *fpout;
  int chunks, got ;
  char *p ;
  char orgfilename[50];
  char orgfile_extension [4];
  char revfilename[50];
  char outputfilename[50];
  int posdot;
  int j, extlength;
  char tempchar;
  unsigned char  databyte ;


  assert(sizeof(dtype)==1);//anything works only with bytes
  
  if ((fpin=fopen(argv[1], "rb"))==NULL)
  {
	  printf("Cannot open %s for input \n", argv[1]);
	  exit(-1);
  }

  tr_buf = malloc(MAX_FILE_SIZE*sizeof(dtype));
  if(tr_buf==NULL )
  {
        fprintf(stderr,"%s: out of memory, bailing.\n",argv[0]);
         exit(-1);
  }
   rs_bsize=RS_BSIZE;
   rs_dsize=RS_DSIZE;


  got = fread(tr_buf, 1, MAX_FILE_SIZE, fpin);
  chunks = (got+7) / rs_dsize ;
  if (((got+7) % rs_dsize ) > 0)
     chunks++ ;
    //encode
   bep_size=chunks;
   //get the buffers we need (here we know how large):
   ec_buf=malloc(rs_bsize*bep_size*sizeof(dtype));  
   bk_buf=malloc(rs_bsize*bep_size*sizeof(dtype));  
   if(ec_buf==NULL ||  bk_buf==NULL)
   {
              fprintf(stderr,"%s: out of memory, bailing.\n",argv[0]);
              exit(-1);
   }
   fseek(fpin, 0L, SEEK_SET);   /* reset to start of file */
   /* find extension of original file */
   strcpy(revfilename, argv[1]);
   revstring(revfilename);
   /* strrev(revfilename); */
 //  printf("rev filename is %s \n", revfilename); 
   p=strtok(revfilename, ".");
   strcpy(orgfile_extension, p);
   revstring(orgfile_extension);
   extlength = strlen(orgfile_extension) ;
//  printf(" filename ext is %s\n", orgfile_extension);

   strcpy(orgfilename, revfilename+ extlength+1) ;
   revstring(orgfilename);
   strcpy(outputfilename, orgfilename);
   strcat(outputfilename, ".rs1");
               
  if ((fpout=fopen(outputfilename, "wb"))==NULL)
  {
	  printf("Cannot open %s for output \n", outputfilename);
	  exit(-1);
  }
   /* write header for .rs2 file */

   databyte = (unsigned char) ( rs_dsize - ( got % rs_dsize)) ; /* surplus in filelength */
   ec_buf[0]= databyte;
   databyte = (unsigned char) ( chunks % 256) ;
   ec_buf[1]= databyte ;
   databyte = (unsigned char) (chunks/256) ;
   ec_buf[2]= databyte ;

   strncpy(ec_buf+3, orgfile_extension, 3);   /* max length ext is 3 */
   got = fread(ec_buf+7, 1 , rs_dsize-7, fpin);
   if (got != (rs_dsize-7))
   {
       printf("input file too short \n" );
       exit(-1);
   }            
    rse32(ec_buf,ec_buf+rs_dsize);
             
                for (i=1;i<bep_size;i++)
                {
                        got = fread((ec_buf+i*rs_bsize), 1,rs_dsize, fpin);

			if (!got) break; 
                        if (got < rs_dsize)
                        {
							for (k=i*rs_bsize+got ; k < i*rs_bsize + rs_dsize ; k++)
								    ec_buf[k] = 0;
                        }
                        //encode rs_dsize bytes of data, append Parity in row
                        rse32((ec_buf+i*rs_bsize),(ec_buf+i*rs_bsize+rs_dsize));
                }
                distribute(ec_buf,tr_buf,bep_size,rs_bsize,0);
                if (bep_size*rs_bsize!=fwrite(tr_buf,sizeof(dtype),bep_size*rs_bsize,fpout))
                {       //write the encoded data
                        perror("serious problem: ");
                        if(ec_buf) free(ec_buf);
                        if(tr_buf) free(tr_buf);
                        exit(-1);
                }
  
  //clean up:
  if(ec_buf) free(ec_buf);
  if(tr_buf) free(tr_buf);
  if(bk_buf) free(bk_buf);
  return 0;
}


