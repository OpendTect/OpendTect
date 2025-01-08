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

			GeomID();
			~GeomID();

    bool		isValid() const override;

    OD::GeomSystem	geomSystem() const;

    static inline GeomID udf()		{ return GeomID(); }

    bool		is2D() const;
    bool		is3D() const;
    bool		isSynth() const;
};


/*!
\brief Class providing a current line key.
*/

mExpClass(Basic) GeomIDProvider
{
public:
    virtual		~GeomIDProvider()		{}
    virtual GeomID	geomID() const			= 0;
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
