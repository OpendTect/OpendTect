#ifndef stattype_h
#define stattype_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id: stattype.h,v 1.6 2011-01-25 09:45:43 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"

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
DeclareNameSpaceEnumUtils(Type)

    /* When resampling a denser sampling */
enum UpscaleType
{
	TakeNearest,
	UseAvg, UseMed, UseRMS, UseMostFreq
};
DeclareNameSpaceEnumUtils(UpscaleType)


}; // namespace Stats


#endif
