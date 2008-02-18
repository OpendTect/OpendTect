#ifndef stattype_h
#define stattype_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id: stattype.h,v 1.2 2008-02-18 16:29:30 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"

namespace Stats
{

    /* Average used to be 'Mean' but that does not sound positive enough */
enum Type	{ Count, Average, Median, StdDev, Variance, Min, Max, MostFreq,
    		  Sum, SqSum, RMS, NormVariance };
DeclareNameSpaceEnumUtils(Type)


}; // namespace Stats


#endif
