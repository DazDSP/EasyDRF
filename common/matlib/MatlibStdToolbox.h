/******************************************************************************\
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	c++ Mathematic Library (Matlib), standard toolbox
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

#ifndef _MATLIB_STD_TOOLBOX_H_
#define _MATLIB_STD_TOOLBOX_H_

#include "Matlib.h"

/* fftw (Homepage: http://www.fftw.org) */
#ifdef HAVE_DFFTW_H
# include <dfftw.h>
#else
# include "../libs/fftw.h"
#endif

#ifdef HAVE_DRFFTW_H
# include <drfftw.h>
#else
# include "../libs/rfftw.h"
#endif


/* Classes ********************************************************************/
class CFftPlans
{
public:
	CFftPlans() : RFFTPlForw(NULL), RFFTPlBackw(NULL), bInitialized(false) {}
	CFftPlans(const int iFftSize) {Init(iFftSize);}
	virtual ~CFftPlans();

	void Init(const int iFSi);
	inline bool IsInitialized() const {return bInitialized;}

	rfftw_plan		RFFTPlForw{}; //init DM
	rfftw_plan		RFFTPlBackw{}; //init DM
	fftw_plan		FFTPlForw{}; //init DM
	fftw_plan		FFTPlBackw{}; //init DM

	fftw_real*		pFftwRealIn{}; //init DM
	fftw_real*		pFftwRealOut{}; //init DM
	fftw_complex*	pFftwComplexIn{}; //init DM
	fftw_complex*	pFftwComplexOut{}; //init DM

protected:
	void Clean();
	bool bInitialized;
};


/* Helpfunctions **************************************************************/
inline CReal				Min(const CReal& rA, const CReal& rB)
								{return rA < rB ? rA : rB;}
inline CMatlibVector<CReal>	Min(const CMatlibVector<CReal>& fvA, const CMatlibVector<CReal>& fvB)
								{_VECOP(CReal, fvA.GetSize(), Min(fvA[i], fvB[i]));}
template<class T> T			Min(const CMatlibVector<T>& vecI);


inline CReal				Max(const CReal& rA, const CReal& rB)
								{return rA > rB ? rA : rB;}
inline CMatlibVector<CReal>	Max(const CMatlibVector<CReal>& fvA, const CMatlibVector<CReal>& fvB)
								{_VECOP(CReal, fvA.GetSize(), Max(fvA[i], fvB[i]));}
template<class T> T			Max(const CMatlibVector<T>& vecI);
template<class T> void		Max(T& tMaxVal /* out */, int& iMaxInd /* out */,
								const CMatlibVector<T>& vecI /* in */);


inline CMatlibVector<CReal>	Ones(const int iLen)
								{_VECOP(CReal, iLen, (CReal) 1.0);}
inline CMatlibVector<CReal>	Zeros(const int iLen)
								{_VECOP(CReal, iLen, (CReal) 0.0);}


inline CReal				Real(const CComplex& cI)
								{return cI.real();}
inline CMatlibVector<CReal>	Real(const CMatlibVector<CComplex>& cvI)
								{_VECOP(CReal, cvI.GetSize(), Real(cvI[i]));}

inline CReal				Imag(const CComplex& cI)
								{return cI.imag();}
inline CMatlibVector<CReal>	Imag(const CMatlibVector<CComplex>& cvI)
								{_VECOP(CReal, cvI.GetSize(), Imag(cvI[i]));}

inline CComplex					Conj(const CComplex& cI)
									{return conj(cI);}
inline CMatlibVector<CComplex>	Conj(const CMatlibVector<CComplex>& cvI)
									{_VECOP(CComplex, cvI.GetSize(), Conj(cvI[i]));}
inline CMatlibMatrix<CComplex>	Conj(const CMatlibMatrix<CComplex>& cmI)
									{_MATOP(CComplex, cmI.GetRowSize(), cmI.GetColSize(), Conj(cmI[i]));}


/* Absolute and angle (argument) functions */
inline CReal				Abs(const CReal& rI)
								{return fabs(rI);}
inline CMatlibVector<CReal>	Abs(const CMatlibVector<CReal>& fvI)
								{_VECOP(CReal, fvI.GetSize(), Abs(fvI[i]));}

inline CReal				Abs(const CComplex& cI)
								{return abs(cI);}
inline CMatlibVector<CReal>	Abs(const CMatlibVector<CComplex>& cvI)
								{_VECOP(CReal, cvI.GetSize(), Abs(cvI[i]));}

inline CReal				Angle(const CComplex& cI)
								{return arg(cI);}
inline CMatlibVector<CReal>	Angle(const CMatlibVector<CComplex>& cvI)
								{_VECOP(CReal, cvI.GetSize(), Angle(cvI[i]));}


/* Trigonometric functions */
inline CReal				Sin(const CReal& fI)
								{return sin(fI);}
template<class T> inline
CMatlibVector<T>			Sin(const CMatlibVector<T>& vecI) 
								{_VECOP(T, vecI.GetSize(), sin(vecI[i]));}

inline CReal				Cos(const CReal& fI)
								{return cos(fI);}
template<class T> inline
CMatlibVector<T>			Cos(const CMatlibVector<T>& vecI)
								{_VECOP(T, vecI.GetSize(), cos(vecI[i]));}

inline CReal				Tan(const CReal& fI)
								{return tan(fI);}
template<class T> inline
CMatlibVector<T>			Tan(const CMatlibVector<T>& vecI)
								{_VECOP(T, vecI.GetSize(), tan(vecI[i]));}


/* Square root */
inline CReal				Sqrt(const CReal& fI)
								{return sqrt(fI);}
template<class T> inline
CMatlibVector<T>			Sqrt(const CMatlibVector<T>& vecI)
								{_VECOP(T, vecI.GetSize(), sqrt(vecI[i]));}


/* Exponential function */
inline CReal				Exp(const CReal& fI)
								{return exp(fI);}
template<class T> inline
CMatlibVector<T>			Exp(const CMatlibVector<T>& vecI)
								{_VECOP(T, vecI.GetSize(), exp(vecI[i]));}


/* Logarithm */
inline CReal				Log(const CReal& fI)
								{return log(fI);}
template<class T> inline
CMatlibVector<T>			Log(const CMatlibVector<T>& vecI)
								{_VECOP(T, vecI.GetSize(), log(vecI[i]));}

inline CReal				Log10(const CReal& fI)
								{return log10(fI);}
template<class T> inline
CMatlibVector<T>			Log10(const CMatlibVector<T>& vecI)
								{_VECOP(T, vecI.GetSize(), log10(vecI[i]));}


/* Mean, variance and standard deviation */
template<class T> inline
T							Mean(const CMatlibVector<T>& vecI)
								{return Sum(vecI) / vecI.GetSize();}
template<class T> inline
T							Std(CMatlibVector<T>& vecI) 
								{return Sqrt(Var(vecI));}
template<class T> T			Var(const CMatlibVector<T>& vecI);


/* Rounding functions */
inline CReal				Fix(const CReal& fI)
								{return (int) fI;}
inline CMatlibVector<CReal>	Fix(const CMatlibVector<CReal>& fvI)
								{_VECOP(CReal, fvI.GetSize(), Fix(fvI[i]));}

inline CReal				Floor(const CReal& fI)
								{return floor(fI);}
inline CMatlibVector<CReal>	Floor(const CMatlibVector<CReal>& fvI)
								{_VECOP(CReal, fvI.GetSize(), Floor(fvI[i]));}

inline CReal				Ceil(const CReal& fI)
								{return ceil(fI);}
inline CMatlibVector<CReal>	Ceil(const CMatlibVector<CReal>& fvI)
								{_VECOP(CReal, fvI.GetSize(), Ceil(fvI[i]));}

inline CReal				Round(const CReal& fI)
								{return Floor(fI + (CReal) 0.5);}
inline CMatlibVector<CReal>	Round(const CMatlibVector<CReal>& fvI)
								{_VECOP(CReal, fvI.GetSize(), Round(fvI[i]));}


template<class T> T			Sum(const CMatlibVector<T>& vecI);

CMatlibVector<CReal>		Sort(const CMatlibVector<CReal>& rvI);


/* Matrix inverse */
CMatlibMatrix<CComplex>		Inv(const CMatlibMatrix<CComplex>& matrI);

/* Identity matrix */
CMatlibMatrix<CReal>		Eye(const int iLen);

CMatlibMatrix<CComplex>		Diag(const CMatlibVector<CComplex>& cvI);

/* Matrix transpose */
CMatlibMatrix<CComplex>		Transp(const CMatlibMatrix<CComplex>& cmI);
inline
CMatlibMatrix<CComplex>		TranspH(const CMatlibMatrix<CComplex>& cmI)
								{return Conj(Transp(cmI));} /* With conjugate complex */

/* Fourier transformations (also included: real FFT) */
CMatlibVector<CComplex>		Fft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans = CFftPlans());
CMatlibVector<CComplex>		Ifft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans = CFftPlans());
CMatlibVector<CComplex>		rfft(CMatlibVector<CReal>& fvI, const CFftPlans& FftPlans = CFftPlans());
CMatlibVector<CReal>		rifft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans = CFftPlans());


/* Implementation **************************************************************
   (the implementation of template classes must be in the header file!) */
template<class T> inline
T Min(const CMatlibVector<T>& vecI)
{
	const int iSize = vecI.GetSize();
	T fMinRet = vecI[0];
	for (int i = 0; i < iSize; i++)
		if (vecI[i] < fMinRet)
			fMinRet = vecI[i];

	return fMinRet;
}

template<class T> inline
T Max(const CMatlibVector<T>& vecI)
{
	T fMaxRet;
	int iMaxInd; /* Not used in by this function */
	Max(fMaxRet, iMaxInd, vecI);

	return fMaxRet;
}

template<class T> inline
void Max(T& tMaxVal, int& iMaxInd, const CMatlibVector<T>& vecI)
{
	const int iSize = vecI.GetSize();
	tMaxVal = vecI[0]; /* Init actual maximum value */
	iMaxInd = 0; /* Init index of maximum */
	for (int i = 0; i < iSize; i++)
	{
		if (vecI[i] > tMaxVal)
		{
			tMaxVal = vecI[i];
			iMaxInd = i;
		}
	}
}

template<class T> inline
T Sum(const CMatlibVector<T>& vecI)
{
	const int iSize = vecI.GetSize();
	T SumRet = 0;
	for (int i = 0; i < iSize; i++)
		SumRet += vecI[i];

	return SumRet;
}

template<class T> inline
T Var(const CMatlibVector<T>& vecI)
{
	const int iSize = vecI.GetSize();

	/* First calculate mean */
	T tMean = Mean(vecI);

	/* Now variance (sum formula) */
	T tRet = 0;
	for (int i = 0; i < iSize; i++)
		tRet += (vecI[i] - tMean) * (vecI[i] - tMean);

	return tRet / (iSize - 1); /* Normalizing */
}


#endif	/* _MATLIB_STD_TOOLBOX_H_ */
