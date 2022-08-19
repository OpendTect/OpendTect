#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "enums.h"
#include "algomod.h"

namespace Stats
{

    /* One-dimensional statistical properties of value series */
enum Type
{
    	Count,
	Average, Median, RMS,
	StdDev, Variance, NormVariance,
	Min, Max, Extreme,
	Sum, SqSum,
	MostFreq
};
mDeclareNameSpaceEnumUtils(Algo,Type)

    /* When resampling a denser sampling */
enum UpscaleType
{
	TakeNearest,
	UseAvg, UseMed, UseRMS, UseMostFreq
};
mDeclareNameSpaceEnumUtils(Algo,UpscaleType)

inline Type typeFor( UpscaleType ut )
{
    return ut == UseAvg ?	Average
	: (ut == UseRMS ?	RMS
	: (ut == UseMostFreq ?	MostFreq
	:			Median));
}

inline UpscaleType upscaleTypeFor( Type st )
{
    return st == Average ?	UseAvg
	: (st == RMS ?		UseRMS
	: (st == MostFreq ?	UseMostFreq
	: (st == Median ?	UseMed
	:			TakeNearest)));
}


} // namespace Stats
