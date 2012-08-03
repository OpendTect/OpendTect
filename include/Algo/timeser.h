#ifndef timeser_H
#define timeser_H

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: timeser.h,v 1.3 2012-08-03 13:00:06 cvskris Exp $
________________________________________________________________________

-*/
 
#include "algomod.h"
#include "gendefs.h"

#ifdef __cpp__
extern "C" {
#endif

mGlobal(Algo) void BFhighpass(int npoles,float f3db,int n,const float* arrin,float* arrout);
mGlobal(Algo) void BFlowpass (int npoles,float f3db,int n,const float* arrin,float* arrout);
mGlobal(Algo) void AntiAlias(float frac,int n,const float* arrin,float* arrout);
mGlobal(Algo) void Convolve(int lx,int ifx,const float* x,int ly,int ify,const float* y,
               int lz,int ifz,float* z);
mGlobal(Algo) void Resample(int stepsize,int ns,const float* arrin,float* arrout);
mGlobal(Algo) void Hilbert(int n,float* x,float* y);


#define mHalfHilbertLength 30

#ifdef __cpp__
}
#endif


#endif

