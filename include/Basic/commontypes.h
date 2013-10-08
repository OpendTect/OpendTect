#ifndef commontypes_h
#define commontypes_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Salil
 Date:		Oct 2013
 RCS:		$Id$
________________________________________________________________________

*/

#ifndef commondefs_h
# include "commondefs.h"
#endif

#ifdef __cpp__

class BinID;
class Coord;
class Coord3;
class TrcKey;

namespace Pos
{

class IdxPair;

typedef od_int32	Index_Type;
typedef od_int32	Index_Delta_Type;
typedef Index_Type	SurvID;
typedef Index_Type	LineID;
typedef Index_Type	TraceID;
typedef Index_Type	GeomID;

typedef double		Ordinate_Type;
typedef double		Distance_Type;

}

#endif /* ifdef __cpp__ */


#endif
