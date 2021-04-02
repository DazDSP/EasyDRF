#if !defined fir_h
#define fir_h

#define zffiltleadlen 40
#define zffilttraillen 41
#define zffiltlen (zffiltleadlen+zffilttraillen)

void FirInit(int dftsize,ESpecOcc spec);

void DoFir (_COMPLEX * samples,_COMPLEX * outsamples);
void NoFir (_COMPLEX * samples,_COMPLEX * outsamples);

#endif
