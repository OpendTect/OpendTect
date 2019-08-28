#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2018
________________________________________________________________________

-*/

#include "integerid.h"
#include "typeset.h"
#include "bufstring.h"


namespace Pos
{

/*!\brief is the key to a survey geometry. Only the LineBasedGeom
    GeomSystem has multiple (Survey::Geometry2D) instances.
*/

mDefIntegerIDTypeFull( int, GeomID,

	-999,

	inline bool		is2D() const		{ return nr_>=0; }
	inline bool		is3D() const		{ return nr_<0; }
	inline bool		isSynthetic() const	{ return nr_==-2; }
	inline IDType		lineNr() const		{ return getI(); }
	inline static GeomID	get3D()			{ return GeomID(-1); }
	inline static GeomID	getDefault2D();
	inline BufferString	name() const;

);

#   define mUdfGeomID Pos::GeomID()
#   define mIsUdfGeomID(gid) ((gid).isInvalid())


} //namespace Pos


mExpClass(Basic) GeomIDSet : public TypeSet<Pos::GeomID>
{
public:

    mUseType( Pos,	GeomID );
    typedef TypeSet<GeomID::IDType>	IntSet;

			GeomIDSet()			{}
			GeomIDSet( GeomID gid )	{ add(gid); }
    explicit		GeomIDSet(const IntSet&);

    void		getIntSet(IntSet&) const;

};


mGlobal(Basic) BufferString nameOf(Pos::GeomID);
mGlobal(Basic) Pos::GeomID geomIDOf(const DBKey&);
mGlobal(Basic) Pos::GeomID getDefault2DGeomID();
inline BufferString Pos::GeomID::name() const { return nameOf(*this); }
inline Pos::GeomID Pos::GeomID::getDefault2D() { return getDefault2DGeomID(); }


inline OD::GeomSystem geomSystemOf( Pos::GeomID gid )
{
    return gid.getI() >= (Pos::GeomID::IDType)OD::LineBasedGeom
	 ? OD::LineBasedGeom
	 : (gid.isSynthetic() ? OD::SynthGeom : OD::VolBasedGeom);
}

inline const char* toString( Pos::GeomID gid )
{
    return toString( gid.getI() );
}

inline GeomIDSet::GeomIDSet( const IntSet& ints )
{
    for ( auto i : ints )
	add( GeomID(i) );
}

inline void GeomIDSet::getIntSet( IntSet& ints ) const
{
    for ( auto gid : *this )
	ints.add( gid.getI() );
}

// This prevents usage of mIsUdf(geomid). Use !geomid.isValid() instead.
namespace Values
{ template <> bool Undef<Pos::GeomID>::isUdf(Pos::GeomID) = delete; }
