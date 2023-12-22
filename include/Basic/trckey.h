#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
#include "posgeomid.h"

namespace Survey { class Geometry; }

/*!
\brief Represents a unique trace position in one of the surveys that OpendTect
is managing.

The class is a combination of a unique survey ID and a bin position ID which is
currently implemented using a BinID (2D trace number is the crossline).

*/


mExpClass(Basic) TrcKey
{
public:

    typedef IdxPair::IdxType	IdxType;

			//Undefined
			TrcKey();
			//3D
    explicit		TrcKey(const BinID&); // default 3D surv ID
			//2D
			TrcKey(Pos::GeomID,Pos::TraceID);
			//2D or 3D (not synthetic)
			TrcKey(const Pos::IdxPair&,bool is2d);
			//Any type
			TrcKey(OD::GeomSystem,const Pos::IdxPair&);
			~TrcKey();
    static TrcKey	getSynth(Pos::TraceID);

    bool		operator==(const TrcKey&) const;
    bool		operator!=( const TrcKey& oth ) const
			{ return !(*this==oth); }

    OD::GeomSystem	geomSystem() const	{ return geomsystem_; }
    bool		is2D() const		{ return ::is2D(geomsystem_); }
    bool		is3D() const		{ return ::is3D(geomsystem_); }
    bool		isSynthetic() const
					{ return ::isSynthetic(geomsystem_); }

    inline bool		isUdf() const			{ return pos_.isUdf(); }
    bool		exists() const; //!< checks in geometry

    const BinID&	position() const		{ return pos_; }
    Pos::GeomID		geomID() const;
    IdxType		lineNr() const			{ return pos_.row(); }
    IdxType		trcNr() const			{ return pos_.col(); }
    const BinID&	binID() const			{ return pos_; }
    Pos::IdxPair	idxPair() const;
    IdxType		inl() const			{ return pos_.inl(); }
    IdxType		crl() const			{ return pos_.crl(); }

    TrcKey&		setGeomID(Pos::GeomID);
    TrcKey&		setGeomSystem(OD::GeomSystem);
    TrcKey&		setPosition(const BinID&); //3D only
    TrcKey&		setPosition(const Pos::IdxPair&,bool is2d);
    inline TrcKey&	setPosition( Pos::GeomID gid, IdxType trcnr )
			{ return setGeomID( gid ).setTrcNr( trcnr ); }
    inline TrcKey&	setIs3D()	{ return setGeomSystem( OD::Geom3D ); }
    inline TrcKey&	setIs2D()	{ return setGeomSystem( OD::Geom2D ); }
    inline TrcKey&	setIs2D( bool yn )
			{ return yn ? setIs2D() : setIs3D(); }
    inline TrcKey&	setIsSynthetic() { return setGeomSystem(OD::GeomSynth);}
    inline TrcKey&	setUdf()	 { *this = udf(); return *this; }

			// These do not change the GeomSystem of the TrcKey:
    inline TrcKey&	setLineNr( IdxType nr )
			{ pos_.row() = nr; return *this; }
    inline TrcKey&	setTrcNr( IdxType tnr )
			{ pos_.col() = tnr; return *this; }
    inline TrcKey&	setInl( IdxType nr )
			{ return setLineNr(nr); }
    inline TrcKey&	setCrl( IdxType nr )
			{ return setTrcNr(nr); }

    TrcKey&		setFrom(const Coord&);	//!< Uses OD::GeomSystem
    Coord		getCoord() const;	//!< Uses OD::GeomSystem
    double		sqDistTo(const TrcKey&) const;
    double		distTo(const TrcKey&) const;
    const Survey::Geometry& geometry() const;

    TrcKey		getFor(Pos::GeomID) const;
    TrcKey		getFor3D() const;
    TrcKey		getFor2D( IdxType lnr ) const;

    BufferString	usrDispStr() const;

    static const TrcKey& udf();

private:

    OD::GeomSystem	geomsystem_;
    BinID		pos_;

public:

    static Pos::GeomID	gtGeomID(OD::GeomSystem,IdxType linenr=-1);

public:

    typedef OD::GeomSystem	SurvID;

    mDeprecated("Use setGeomSystem")
    OD::GeomSystem	survID() const		{ return geomSystem(); }

    mDeprecated("Use setGeomSystem")
    TrcKey&		setSurvID(OD::GeomSystem);

    mDeprecated("Use OD::GeomSystem and Pos::IdxPair")
    explicit		TrcKey(OD::GeomSystem,const BinID&);

    mDeprecated("Use setPosition")
    inline TrcKey&	setBinID( const BinID& bid )
			{ return setPosition(bid); }

    mDeprecated("Use position()")
    const BinID& pos() const	{ return pos_; }
    mDeprecated("Use setPosition")
    void		setPos( const BinID& bid )	{ setPosition(bid); }
    mDeprecated("Use a set function")
    IdxType&		lineNr();
    mDeprecated("Use a set function")
    IdxType&		trcNr();

    mDeprecated("Use OD::GeomSystem")
    static OD::GeomSystem std2DSurvID()	{ return OD::Geom2D; }
    mDeprecated("Use OD::GeomSystem")
    static OD::GeomSystem std3DSurvID()	{ return OD::Geom3D; }
    mDeprecated("Use OD::GeomSystem")
    static OD::GeomSystem cUndefSurvID()	{ return OD::GeomSynth; }

    mDeprecated("Use global function")
    static bool		is2D(  OD::GeomSystem gs ) { return ::is2D( gs ); }

    mDeprecated("Use OD::GeomSystem and the geometry manager")
    static Pos::GeomID	geomID(OD::GeomSystem,const BinID&);


};
