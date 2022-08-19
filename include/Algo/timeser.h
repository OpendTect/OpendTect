#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

 
#include "algomod.h"
#include "gendefs.h"

extern "C" {

mGlobal(Algo) void BFhighpass(int npoles,float f3db,int n,const float* arrin,
			      float* arrout);
mGlobal(Algo) void BFlowpass (int npoles,float f3db,int n,const float* arrin,
			      float* arrout);
mGlobal(Algo) void AntiAlias(float frac,int n,const float* arrin,float* arrout);
mGlobal(Algo) void Convolve(int lx,int ifx,const float* x,
			    int ly,int ify,const float* y,
			    int lz,int ifz,float* z);
mGlobal(Algo) void Resample(int stepsize,int ns,const float* arrin,
			    float* arrout);
mGlobal(Algo) void Hilbert(int n,float* x,float* y);


#define mHalfHilbertLength 30

}
