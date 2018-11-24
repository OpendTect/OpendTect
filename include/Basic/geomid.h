#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2018
________________________________________________________________________

-*/

#include "integerid.h"
class DBKey;


namespace Pos
{

/*!\brief is the key to a survey geometry. Only the LineBasedGeom
    GeomSystem has multiple (Survey::Geometry2D) instances.
*/

mDefIntegerIDTypeFull( int, GeomID,

	-999,

	inline bool		is2D() const	{ return nr_>=0; }
	inline IDType		lineNr() const	{ return getI(); }
	inline static GeomID	get3D()		{ return GeomID(-1); }
	explicit		GeomID(const DBKey&);

);

#   define mUdfGeomID Pos::GeomID()
#   define mIsUdfGeomID(gid) ((gid).isInvalid())


} //namespace Pos


mGlobal(Basic) const char* nameOf(Pos::GeomID);


inline OD::GeomSystem geomSystemOf( Pos::GeomID gid )
{
    return gid.getI() >= (Pos::GeomID::IDType)OD::LineBasedGeom
	 ? OD::LineBasedGeom
	 : (gid.getI() == -2 ? OD::SynthGeom : OD::VolBasedGeom);
}


inline const char* toString( Pos::GeomID gid )
{
    return toString( gid.getI() );
}


// This prevents usage of mIsUdf(geomid). Use !geomid.isValid() instead.
namespace Values
{ template <> bool Undef<Pos::GeomID>::isUdf(Pos::GeomID) = delete; }
