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
#include "bin2d.h"


/*!\brief represents a unique trace position coupled to a survey geometry

A TrcKey contains the identifying position indices of a seismic trace position.
For 2D data, it will contain the GeomID of the line and the trace number.
For 3D data, it will hold the BinID.

Setting the position to a BinID causes the TrcKey to become a 3D TrcKey, setting
to Bin2D (GeomID and trace number) will turn it into a 2D TrcKey.

Note that this class is only needed if you need a channel of mixed 2D and 3D
positions (and this is not very often!); in most cases you can and *should*
keep this issue outside the position identification.

*/


mExpClass(Basic) TrcKey
{
public:

    mUseType( OD,	GeomSystem );
    mUseType( Pos,	GeomID );
    mUseType( Pos,	IdxPair );
    mUseType( Survey,	Geometry );
    mUseType( Geometry,	pos_type );
    mUseType( Geometry,	linenr_type );
    mUseType( Geometry,	trcnr_type );
    mUseType( Geometry,	dist_type );

			TrcKey()		{ *this = udf(); }

    explicit		TrcKey(const BinID&);	    //!< The 3D choice
    explicit		TrcKey( const Bin2D& b2d )
			    : TrcKey(b2d.geomID(),b2d.trcNr())	{}
			TrcKey(GeomID,trcnr_type);
			TrcKey(GeomSystem,const BinID&);
			TrcKey(const BinID&,bool is2d);
    static TrcKey	getSynth(trcnr_type);
			mImplSimpleEqOpers2Memb(TrcKey,geomsystem_,pos_)

    GeomSystem		geomSystem() const	{ return geomsystem_; }
    bool		is2D() const		{ return ::is2D(geomsystem_); }
    bool		is3D() const		{ return ::is3D(geomsystem_); }
    bool		isSynthetic() const
			{ return geomsystem_==OD::SynthGeom; }

    bool		isUdf() const;	//!< just examines inl/crl
    bool		exists() const;	//!< checks in geometry

    const BinID&	position() const		{ return pos_; }
    GeomID		geomID() const;
    linenr_type		lineNr() const			{ return pos_.row(); }
    trcnr_type		trcNr() const			{ return pos_.col(); }
    const BinID&	binID() const			{ return position(); }
    Bin2D		bin2D() const;
    IdxPair		idxPair() const;
    pos_type		inl() const			{ return pos_.row(); }
    pos_type		crl() const			{ return pos_.col(); }

			// These set the GeomSystem of the TrcKey:
    TrcKey&		setGeomID(GeomID);
    TrcKey&		setGeomSystem(GeomSystem);
    TrcKey&		setPos(const BinID&);
    TrcKey&		setPos( const Bin2D& b2d )
			{ return setPos( b2d.geomID(), b2d.trcNr() ); }
    inline TrcKey&	setPos( GeomID gid, trcnr_type trcnr )
			{ setGeomID( gid ); return setTrcNr( trcnr ); }
    inline TrcKey&	setIs3D()
			{ geomsystem_ = OD::VolBasedGeom; return *this; }
    inline TrcKey&	setIs2D()
			{ geomsystem_ = OD::LineBasedGeom; return *this; }
    inline TrcKey&	setIs2D( bool yn )
			{ yn ? setIs2D() : setIs3D(); return *this; }
    inline TrcKey&	setIsSynthetic()
			{ geomsystem_ = OD::SynthGeom; return *this; }
    inline TrcKey&	setUdf()
			{ *this = udf(); return *this; }

			// These do not change the GeomSystem of the TrcKey:
    inline TrcKey&	setLineNr( linenr_type nr )
			{ pos_.row() = nr; return *this; }
    inline TrcKey&	setTrcNr( trcnr_type tnr )
			{ pos_.col() = tnr; return *this; }
    inline TrcKey&	setInl( pos_type nr )
			{ return setLineNr(nr); }
    inline TrcKey&	setCrl( pos_type nr )
			{ return setTrcNr(nr); }

    TrcKey&		setFrom(const Coord&);
    Coord		getCoord() const;
    dist_type		sqDistTo(const TrcKey&) const;
    dist_type		distTo(const TrcKey&) const;
    const Geometry&	geometry() const;

    TrcKey		getFor(GeomID) const;
    TrcKey		getFor3D() const
			{ return getFor(GeomID::get3D()); }
    TrcKey		getFor2D( linenr_type lnr ) const
			{ return getFor(GeomID(lnr)); }

    BufferString	usrDispStr() const;

    static const TrcKey& udf();

protected:

    GeomSystem		geomsystem_		= OD::VolBasedGeom;
    BinID		pos_;

public:

    static GeomID	gtGeomID(GeomSystem,linenr_type);

};


mExpClass(Basic) TrcKeyPath : public TypeSet<TrcKey>
{
public:
		    TrcKeyPath()				{}
    explicit	    TrcKeyPath( size_type sz )
			: TypeSet<TrcKey>(sz,TrcKey::udf())	{}

};


/*!\brief TrcKey and a value. */

class BinIDValue;

mExpClass(Basic) TrcKeyValue
{
public:

    mUseType( TrcKey,	linenr_type );
    mUseType( TrcKey,	trcnr_type );

    inline		TrcKeyValue( const TrcKey& tk=TrcKey::udf(),
				     float v=mUdf(float) )
			    : tk_(tk)
			    , val_(v)					{}
			TrcKeyValue(const BinIDValue&);

    linenr_type		lineNr() const		{ return tk_.lineNr(); }
    trcnr_type		trcNr() const		{ return tk_.trcNr(); }
    TrcKeyValue&	setLineNr( linenr_type nr )
			{ tk_.setLineNr(nr); return *this; }
    TrcKeyValue&	setTrcNr( trcnr_type nr )
			{ tk_.setTrcNr(nr); return *this; }

    inline bool		operator==( const TrcKeyValue& oth ) const
			{ return oth.tk_==tk_ && mIsEqual(oth.val_,val_,1e-5); }
    inline bool		operator!=( const TrcKeyValue& oth ) const
			{ return !(*this==oth); }

    inline bool		isDefined() const
			{ return !tk_.isUdf() && !mIsUdf(val_); }
    inline bool		isUdf() const		{ return !isDefined(); }
    static const TrcKeyValue& udf();
    inline void		setUdf()		{ *this = udf(); }

    TrcKey		tk_;
    float		val_;

};
