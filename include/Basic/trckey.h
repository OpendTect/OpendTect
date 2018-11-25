#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert/Salil
 Date:		2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "survgeom.h"

/*!\brief represents a unique trace position in a survey geometry

In 2D surveys, it will contain the GeomID of the line, and the trace number.
For the 3D survey, it will have a BinID.

Note that this class is only needed if you mix 2D and 3D positions; in most
cases you can keep this issue outside the position identification.

*/


mExpClass(Basic) TrcKey
{
public:

    mUseType( OD,	GeomSystem );
    mUseType( Pos,	GeomID );
    mUseType( Survey,	Geometry );
    mUseType( Geometry,	pos_type );
    mUseType( Geometry,	linenr_type );
    mUseType( Geometry,	tracenr_type );
    mUseType( Geometry,	dist_type );

			TrcKey()		{ *this = udf(); }

    explicit		TrcKey(const BinID&);	    //!< The 3D choice
			TrcKey(GeomID,tracenr_type); //!< The 2D choice
			TrcKey(GeomSystem,const BinID&);
			TrcKey(const BinID&,bool is2d);
    static TrcKey	getSynth(tracenr_type);
			mImplSimpleEqOpers2Memb(TrcKey,geomsystem_,pos_)

    GeomSystem		geomSystem() const	{ return geomsystem_; }
    bool		is2D() const		{ return ::is2D(geomsystem_); }
    bool		is3D() const		{ return ::is3D(geomsystem_); }
    bool		isSynthetic() const
			{ return geomsystem_==OD::SynthGeom; }

    bool		isUdf() const;	//!< just examines inl/crl
    bool		exists() const;	//!< checks in geometry

    GeomID		geomID() const	{ return geomID(geomSystem(),pos_); }
    static GeomID	geomID(GeomSystem,const BinID&);

    const BinID&	position() const		{ return pos_; }
    pos_type		lineNr() const			{ return pos_.row(); }
    pos_type		trcNr() const			{ return pos_.col(); }
    const BinID&	binID() const			{ return position(); }
    pos_type		inl() const			{ return lineNr(); }
    pos_type		crl() const			{ return trcNr(); }

    TrcKey&		setGeomID(GeomID);
    TrcKey&		setGeomSystem(GeomSystem);
    inline TrcKey&	setPosition( const BinID& bid )
			{ pos_ = bid; return *this; }
    inline TrcKey&	setLineNr( linenr_type nr )
			{ pos_.row() = nr; return *this; }
    inline TrcKey&	setLineNr( GeomID gid )
			{ pos_.row() = gid.lineNr(); return *this; }
    inline TrcKey&	setTrcNr( tracenr_type nr )
			{ pos_.col() = nr; return *this; }
    inline TrcKey&	setBinID( const BinID& bid )
			{ return setPosition(bid); }
    inline TrcKey&	setInl( pos_type nr )
			{ return setLineNr(nr); }
    inline TrcKey&	setCrl( pos_type nr )
			{ return setTrcNr(nr); }
    inline TrcKey&	setIs2D()
			{ geomsystem_ = OD::LineBasedGeom; return *this; }
    inline TrcKey&	setIs3D()
			{ geomsystem_ = OD::VolBasedGeom; return *this; }
    inline TrcKey&	setIsSynthetic()
			{ geomsystem_ = OD::SynthGeom; return *this; }
    inline TrcKey&	setIs2D( bool yn )
			{ yn ? setIs2D() : setIs3D(); return *this; }

    TrcKey&		setFrom(const Coord&);
    Coord		getCoord() const;
    dist_type		distTo(const TrcKey&) const;
    const Geometry&	geometry() const;

    TrcKey		getFor(GeomID) const;
    TrcKey		getFor3D() const
			{ return getFor(GeomID::get3D()); }
    TrcKey		getFor2D( linenr_type lnr ) const
			{ return getFor(GeomID(lnr)); }

    static const TrcKey& udf();

protected:

    GeomSystem		geomsystem_		= OD::VolBasedGeom;
    BinID		pos_;

};

template <class T> class TypeSet;
typedef TypeSet<TrcKey> TrcKeyPath;
