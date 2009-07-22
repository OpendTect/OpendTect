#ifndef stattype_h
#define stattype_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id: stattype.h,v 1.3 2009-07-22 16:01:12 cvsbert Exp $
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
