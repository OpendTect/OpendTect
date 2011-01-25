#ifndef stattype_h
#define stattype_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id: stattype.h,v 1.5 2011-01-25 09:40:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"

namespace Stats
{

    /* Average used to be 'Mean' but that does not sound positive enough */
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
