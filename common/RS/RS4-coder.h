#if !defined(RS4DEF_)
#define RS4DEF_ 1

int rs4encode(unsigned char* inbuf, unsigned char* outbuf, unsigned int filesize);
int rs4decode(unsigned char* inbuf, unsigned  char* outbuf, unsigned int filesize);
int rs4decodeE(unsigned char* inbuf, unsigned char* inbuf2, unsigned  char* outbuf, unsigned int filesize);
void distribute(unsigned char* src, unsigned char* dst, unsigned int filesize, bool reverse);


#endif
