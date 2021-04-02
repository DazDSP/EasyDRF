/******************************************************************************\
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	c++ Mathematic Library (Matlib)
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

#include "MatlibStdToolbox.h"


/* Implementation *************************************************************/
CMatlibVector<CReal> Sort(const CMatlibVector<CReal>& rvI)
{
	const int iSize = rvI.GetSize();
	const int iEnd = iSize - 1;
	CMatlibVector<CReal>	fvRet(iSize, VTY_TEMP);
	CReal					rSwap;

	/* Copy input vector in output vector */
	fvRet = rvI;

	/* Loop through the array one less than its total cell count */
	for (int i = 0; i < iEnd; i++)
	{
		/* Loop through every cell (value) in array */
		for (int j = 0; j < iEnd; j++)
		{
			/* Compare the values and switch if necessary */
			if (fvRet[j] > fvRet[j + 1])
			{
				rSwap = fvRet[j];
				fvRet[j] = fvRet[j + 1];
				fvRet[j + 1] = rSwap;
			}
		}
	}

	return fvRet;
}

CMatlibMatrix<CReal> Eye(const int iLen)
{
	CMatlibMatrix<CReal> matrRet(iLen, iLen, VTY_TEMP);

	/* Set all values except of the diagonal to zero, diagonal entries = 1 */
	for (int i = 0; i < iLen; i++)
	{
		for (int j = 0; j < iLen; j++)
		{
			if (i == j)
				matrRet[i][j] = (CReal) 1.0;
			else
				matrRet[i][j] = (CReal) 0.0;
		}
	}

	return matrRet;
}

CMatlibMatrix<CComplex> Diag(const CMatlibVector<CComplex>& cvI)
{
	const int iSize = cvI.GetSize();
	CMatlibMatrix<CComplex> matcRet(iSize, iSize, VTY_TEMP);

	/* Set the diagonal to the values of the input vector */
	for (int i = 0; i < iSize; i++)
	{
		for (int j = 0; j < iSize; j++)
		{
			if (i == j)
				matcRet[i][j] = cvI[i];
			else
				matcRet[i][j] = (CReal) 0.0;
		}
	}

	return matcRet;
}

CMatlibMatrix<CComplex> Transp(const CMatlibMatrix<CComplex>& cmI)
{
	const int iRowSize = cmI.GetRowSize();
	const int iColSize = cmI.GetColSize();

	/* Swaped row and column size due to transpose operation */
	CMatlibMatrix<CComplex> matcRet(iColSize, iRowSize, VTY_TEMP);

	/* Transpose matrix */
	for (int i = 0; i < iRowSize; i++)
		for (int j = 0; j < iColSize; j++)
				matcRet[j][i] = cmI[i][j];

	return matcRet;
}

CMatlibMatrix<CComplex> Inv(const CMatlibMatrix<CComplex>& matrI)
{
/*
	Parts of the following code are taken from Ptolemy
	(http://ptolemy.eecs.berkeley.edu/)

	The input matrix must be square, this is NOT checked here!
*/
	_COMPLEX	temp;
	int			row, col, i;

	const int iSize = matrI.GetColSize();
	CMatlibMatrix<CComplex> matrRet(iSize, iSize, VTY_TEMP);

	/* Make a working copy of input matrix */
	CMatlibMatrix<CComplex> work(matrI);

	/* Set result to be the identity matrix */
	matrRet = Eye(iSize);

	for (i = 0; i < iSize; i++) 
	{
		/* Check that the element in (i,i) is not zero */
		if ((Real(work[i][i]) == 0) && (Imag(work[i][i]) == 0))
		{
			/* Swap with a row below this one that has a non-zero element
			   in the same column */
			for (row = i + 1; row < iSize; row++)
				if ((Real(work[i][i]) != 0) || (Imag(work[i][i]) != 0))
					break;

// TEST
if (row == iSize)
{
//				Error::abortRun("couldn't invert matrix, possibly singular.\n");
	matrRet = Eye(iSize);
	return matrRet;
}

			/* Swap rows */
			for (col = 0; col < iSize; col++)
			{
				temp = work[i][col];
				work[i][col] = work[row][col];
				work[row][col] = temp;
				temp = matrRet[i][col];
				matrRet[i][col] = matrRet[row][col];
				matrRet[row][col] = temp;
			}
		}

		/* Divide every element in the row by element (i,i) */
		temp = work[i][i];
		for (col = 0; col < iSize; col++)
		{
			work[i][col] /= temp;
			matrRet[i][col] /= temp;
		}

		/* Zero out the rest of column i */
		for (row = 0; row < iSize; row++)
		{
			if (row != i)
			{
				temp = work[row][i];
				for (col = iSize - 1; col >= 0; col--)
				{
					work[row][col] -= (temp * work[i][col]);
					matrRet[row][col] -= (temp * matrRet[i][col]);
				}
			}
		}
	}

	return matrRet;
}

CMatlibVector<CComplex> Fft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans)
{
	int						i;
	CFftPlans*				pCurPlan;
	fftw_complex*			pFftwComplexIn;
	fftw_complex*			pFftwComplexOut;

	const int				n(cvI.GetSize());

	CMatlibVector<CComplex>	cvReturn(n, VTY_TEMP);

	/* If input vector has zero length, return */
	if (n == 0)
		return cvReturn;

	/* Check, if plans are already created, else: create it */
	if (!FftPlans.IsInitialized())
	{
		pCurPlan = new CFftPlans;
		pCurPlan->Init(n);
	}
	else
	{
		/* Ugly, but ok: We transform "const" object in "non constant" object
		   since we KNOW that the original object is not constant since it
		   was already initialized! */
		pCurPlan = (CFftPlans*) &FftPlans;
	}

	pFftwComplexIn = pCurPlan->pFftwComplexIn;
	pFftwComplexOut = pCurPlan->pFftwComplexOut;

	/* fftw (Homepage: http://www.fftw.org/) */
	for (i = 0; i < n; i++)
	{
		pFftwComplexIn[i].re = cvI[i].real();
		pFftwComplexIn[i].im = cvI[i].imag();
	}

	/* Actual fftw call */
	fftw_one(pCurPlan->FFTPlForw, pFftwComplexIn, pFftwComplexOut);

	for (i = 0; i < n; i++)
		cvReturn[i] = CComplex(pFftwComplexOut[i].re, pFftwComplexOut[i].im);

	if (!FftPlans.IsInitialized())
		delete pCurPlan;

	return cvReturn;
}

CMatlibVector<CComplex> Ifft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans)
{
	int						i;
	CFftPlans*				pCurPlan;
	fftw_complex*			pFftwComplexIn;
	fftw_complex*			pFftwComplexOut;

	const int				n(cvI.GetSize());

	CMatlibVector<CComplex>	cvReturn(n, VTY_TEMP);

	/* If input vector has zero length, return */
	if (n == 0)
		return cvReturn;

	/* Check, if plans are already created, else: create it */
	if (!FftPlans.IsInitialized())
	{
		pCurPlan = new CFftPlans;
		pCurPlan->Init(n);
	}
	else
	{
		/* Ugly, but ok: We transform "const" object in "non constant" object
		   since we KNOW that the original object is not constant since it
		   was already initialized! */
		pCurPlan = (CFftPlans*) &FftPlans;
	}

	pFftwComplexIn = pCurPlan->pFftwComplexIn;
	pFftwComplexOut = pCurPlan->pFftwComplexOut;

	/* fftw (Homepage: http://www.fftw.org/) */
	for (i = 0; i < n; i++)
	{
		pFftwComplexIn[i].re = cvI[i].real();
		pFftwComplexIn[i].im = cvI[i].imag();
	}

	/* Actual fftw call */
	fftw_one(pCurPlan->FFTPlBackw, pFftwComplexIn, pFftwComplexOut);
	
	const CReal scale = (CReal) 1.0 / n;
	for (i = 0; i < n; i++)
		cvReturn[i] = CComplex(pFftwComplexOut[i].re * scale,
			pFftwComplexOut[i].im * scale);

	if (!FftPlans.IsInitialized())
		delete pCurPlan;

	return cvReturn;
}

CMatlibVector<CComplex> rfft(CMatlibVector<CReal>& fvI, const CFftPlans& FftPlans)
{
	int					i;
	CFftPlans*			pCurPlan;
	fftw_real*			pFftwRealIn;
	fftw_real*			pFftwRealOut;

	const int			iSizeI = fvI.GetSize();
	const int			iLongLength(iSizeI);
	const int			iShortLength(iLongLength / 2);
	const int			iUpRoundShortLength((iLongLength + 1) / 2);
	
	CMatlibVector<CComplex>	cvReturn(iShortLength
		/* Include Nyquist frequency in case of even N */ + 1, VTY_TEMP);

	/* If input vector has zero length, return */
	if (iLongLength == 0)
		return cvReturn;

	/* Check, if plans are already created, else: create it */
	if (!FftPlans.IsInitialized())
	{
		pCurPlan = new CFftPlans;
		pCurPlan->Init(iLongLength);
	}
	else
	{
		/* Ugly, but ok: We transform "const" object in "non constant" object
		   since we KNOW that the original object is not constant since it
		   was already initialized! */
		pCurPlan = (CFftPlans*) &FftPlans;
	}

	pFftwRealIn = pCurPlan->pFftwRealIn;
	pFftwRealOut = pCurPlan->pFftwRealOut;

	/* fftw (Homepage: http://www.fftw.org/) */
	for (i = 0; i < iSizeI; i++)
		pFftwRealIn[i] = fvI[i];

	/* Actual fftw call */
	rfftw_one(pCurPlan->RFFTPlForw, pFftwRealIn, pFftwRealOut);

	/* Now build complex output vector */
	/* Zero frequency */
	cvReturn[0] = pFftwRealOut[0];
	for (i = 1; i < iUpRoundShortLength; i++)
		cvReturn[i] = CComplex(pFftwRealOut[i], pFftwRealOut[iLongLength - i]);

	/* If N is even, include Nyquist frequency */
	if (iLongLength % 2 == 0)
		cvReturn[iShortLength] = pFftwRealOut[iShortLength];

	if (!FftPlans.IsInitialized())
		delete pCurPlan;

	return cvReturn;
}

CMatlibVector<CReal> rifft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans)
{
/*
	This function only works with EVEN N!
*/
	int					i;
	CFftPlans*			pCurPlan;
	fftw_real*			pFftwRealIn;
	fftw_real*			pFftwRealOut;

	const int			iShortLength(cvI.GetSize() - 1); /* Nyquist frequency! */
	const int			iLongLength(iShortLength * 2);

	CMatlibVector<CReal> fvReturn(iLongLength, VTY_TEMP);

	/* If input vector is too short, return */
	if (iShortLength <= 0)
		return fvReturn;

	/* Check, if plans are already created, else: create it */
	if (!FftPlans.IsInitialized())
	{
		pCurPlan = new CFftPlans;
		pCurPlan->Init(iLongLength);
	}
	else
	{
		/* Ugly, but ok: We transform "const" object in "non constant" object
		   since we KNOW that the original object is not constant since it
		   was already initialized! */
		pCurPlan = (CFftPlans*) &FftPlans;
	}

	pFftwRealIn = pCurPlan->pFftwRealIn;
	pFftwRealOut = pCurPlan->pFftwRealOut;

	/* Now build half-complex-vector */
	pFftwRealIn[0] = cvI[0].real();
	for (i = 1; i < iShortLength; i++)
	{
		pFftwRealIn[i] = cvI[i].real();
		pFftwRealIn[iLongLength - i] = cvI[i].imag();
	}
	/* Nyquist frequency */
	pFftwRealIn[iShortLength] = cvI[iShortLength].real(); 

	/* Actual fftw call */
	rfftw_one(pCurPlan->RFFTPlBackw, pFftwRealIn, pFftwRealOut);

	/* Scale output vector */
	const CReal scale = (CReal) 1.0 / iLongLength;
	for (i = 0; i < iLongLength; i++) 
		fvReturn[i] = pFftwRealOut[i] * scale;

	if (!FftPlans.IsInitialized())
		delete pCurPlan;

	return fvReturn;
}


/* FftPlans implementation -------------------------------------------------- */
CFftPlans::~CFftPlans()
{
	if (bInitialized)
	{
		/* Delete old plans and intermediate buffers */
		rfftw_destroy_plan(RFFTPlForw);
		rfftw_destroy_plan(RFFTPlBackw);
		fftw_destroy_plan(FFTPlForw);
		fftw_destroy_plan(FFTPlBackw);

		delete[] pFftwRealIn;
		delete[] pFftwRealOut;
		delete[] pFftwComplexIn;
		delete[] pFftwComplexOut;
	}
}

void CFftPlans::Init(const int iFSi)
{
	if (bInitialized)
	{
		/* Delete old plans and intermediate buffers */
		rfftw_destroy_plan(RFFTPlForw);
		rfftw_destroy_plan(RFFTPlBackw);
		fftw_destroy_plan(FFTPlForw);
		fftw_destroy_plan(FFTPlBackw);

		delete[] pFftwRealIn;
		delete[] pFftwRealOut;
		delete[] pFftwComplexIn;
		delete[] pFftwComplexOut;
	}

	/* Create new plans and intermediate buffers */
	pFftwRealIn = new fftw_real[iFSi];
	pFftwRealOut = new fftw_real[iFSi];
	pFftwComplexIn = new fftw_complex[iFSi];
	pFftwComplexOut = new fftw_complex[iFSi];

	RFFTPlForw = rfftw_create_plan(iFSi, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
	RFFTPlBackw = rfftw_create_plan(iFSi, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
	FFTPlForw = fftw_create_plan(iFSi, FFTW_FORWARD, FFTW_ESTIMATE);
	FFTPlBackw = fftw_create_plan(iFSi, FFTW_BACKWARD, FFTW_ESTIMATE);

	bInitialized = true;
}
