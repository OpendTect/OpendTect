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

class IOPar;
class Color;
class BinID;
class Coord;
class Coord3;
class TrcKey;
class IdxPair;
class MultiID;
class FixedString;
class BufferString;
namespace OD { class String; }
template<class T> class TypeSet;
template<class T> class Interval;
template<class T> class ObjectSet;
template<class T> class StepInterval;

typedef od_int32	Index_Type;
typedef od_int32	Index_Delta_Type;

enum CaseSensitivity	{ CaseSensitive=0, CaseInsensitive=1 };
enum Pol2D3D		{ Only3D=-1, Both2DAnd3D=0, Only2D=1 };


namespace Pos
{

class IdxPair;

typedef ::Index_Type	Index_Type;
typedef ::Index_Type	Index_Delta_Type;
typedef Index_Type	SurvID;
typedef Index_Type	LineID;
typedef Index_Type	TraceID;
typedef Index_Type	GeomID;

typedef double		Ordinate_Type;
typedef double		Distance_Type;

}

#endif
