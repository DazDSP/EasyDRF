#ifdef _MSC_VER

__inline int lrintf2(float flt)
{
    int intgr;
    
    _asm
    {
        fld flt
        fistp intgr
    };
    
    return intgr ;
}
#else
#ifdef __GNUC__
/* must define _GNU_SOURCE here or in the makefile */
#include <math.h>
#else

#define FP_BITS(fp) (*(int *)&(fp))
#define FIST_FLOAT_MAGIC_S (float)(7.0f * 2097152.0f)

static int lrintf(float inval)
{
    float tmp = FIST_FLOAT_MAGIC_S + inval;
    int res = ((FP_BITS(tmp)<<10)-0x80000000);
    return res>>10;
}
#endif
#endif

