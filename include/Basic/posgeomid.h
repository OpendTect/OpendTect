#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "commontypes.h"
#include "integerid.h"


namespace Pos
{

mExpClass(Basic) GeomID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;

    inline bool		isValid() const override
			{ return asInt()>=OD::GeomSynth && !isUdf(); }

    inline OD::GeomSystem geomSystem() const
			{
			    if ( asInt() >= int(OD::Geom2D) )
				return OD::Geom2D;
			    if ( asInt() == int(OD::GeomSynth) )
				return OD::GeomSynth;
			    return OD::Geom3D;
			}

    static inline GeomID udf()		{ return GeomID(); }

    inline bool		is2D() const
			{ return geomSystem() == OD::Geom2D; }
    inline bool		is3D() const
			{ return geomSystem() == OD::Geom3D; }
    inline bool		isSynth() const
			{ return geomSystem() == OD::GeomSynth; }
};

} // namespace Pos

namespace Values
{

template<>
mClass(Basic) Undef<Pos::GeomID>
{
public:

    static Pos::GeomID	val()				{ return Pos::GeomID();}
    static bool		hasUdf()			{ return true; }
    static bool		isUdf( const Pos::GeomID& gid )	{ return gid.isUdf(); }
    static void		setUdf( Pos::GeomID& gid )	{ gid = Pos::GeomID(); }

};

} // namespace Values
