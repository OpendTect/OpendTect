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


/*!\brief a trace position coupled to a survey geometry

A TrcKey contains the identifying position indices of a seismic trace position.
For 2D data, it will contain the Bin2D (GeomID and trace number).
For 3D data, it will hold the BinID.

As such, TrcKey is a 'Capsule' class, a channel to communicate a position that
can be BinID or Bin2D. Setting the position to a BinID causes the TrcKey to
become a 3D TrcKey, setting to Bin2D will turn it into a 2D TrcKey.

As a result this class is only needed if you need a channel of mixed 2D and 3D
positions (and this is not very often!); in most cases you can and *should*
keep this issue outside the position identification. Why? Because this class
factually circumvents the C++ type system; many errors are caused by not
being careful with the difference between inl/crl and line/trace.

Thus, try to avoid the TrcKey classes:
* In interfaces, consider supporting explicit BinID and/or Bin2D parts
* For nonsequential sets of positions, consider using BinnedValueSet
* In positioning of concrete geometries, consider PosInfo::LineCollData
* When subselecting from an existing geometry, use the Survey::SubSel classes

The usages that are really OK:
* for sets of mixed 2D and 3D positions
* when you need to return a position that can be either 2D or 3D, where the
  caller will do the accordingly correct thing.

and, a bit driven by practicality, you can use the TrcKeyPath:

* when you need ordered traversal of a concrete path that can be 2D or 3D
  (but again this shld really have a '2D or 3D only' settable constraint).

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

    explicit		TrcKey(const BinID&);
    explicit		TrcKey( const Bin2D& b2d )
			    : TrcKey(b2d.geomID(),b2d.trcNr())	{}
			TrcKey(GeomID,trcnr_type);
			TrcKey(const IdxPair&,bool is2d);
			TrcKey(GeomSystem,const IdxPair&);
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
    const BinID&	binID() const			{ return pos_; }
    Bin2D		bin2D() const;
    IdxPair		idxPair() const;
    pos_type		inl() const			{ return pos_.inl(); }
    pos_type		crl() const			{ return pos_.crl(); }

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


/*!\brief a path of trace positions. Note that there is no enforcement of
  'only 2D' or 'only 3D' because it is conceivable that paths consist of both
  2D and 3D positions. This is highly unlikely though. */


mExpClass(Basic) TrcKeyPath : public TypeSet<TrcKey>
{
public:
		TrcKeyPath()				{}
    explicit	TrcKeyPath( size_type sz )
		    : TypeSet<TrcKey>(sz,TrcKey::udf())	{}

    bool	is2D() const    { return isEmpty() ? false : first().is2D(); }

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
