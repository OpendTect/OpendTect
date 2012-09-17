#ifndef timeser_H
#define timeser_H

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: timeser.h,v 1.2 2011/07/23 22:06:06 cvskris Exp $
________________________________________________________________________

-*/
 
#include "gendefs.h"

#ifdef __cpp__
extern "C" {
#endif

mGlobal void BFhighpass(int npoles,float f3db,int n,const float* arrin,float* arrout);
mGlobal void BFlowpass (int npoles,float f3db,int n,const float* arrin,float* arrout);
mGlobal void AntiAlias(float frac,int n,const float* arrin,float* arrout);
mGlobal void Convolve(int lx,int ifx,const float* x,int ly,int ify,const float* y,
               int lz,int ifz,float* z);
mGlobal void Resample(int stepsize,int ns,const float* arrin,float* arrout);
mGlobal void Hilbert(int n,float* x,float* y);


#define mHalfHilbertLength 30

#ifdef __cpp__
}
#endif


#endif
