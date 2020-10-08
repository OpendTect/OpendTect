#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris / Bert
 Date:		2013 / Nov 2018
________________________________________________________________________

-*/

#include "coord.h"
#include "binid.h"
#include "factory.h"
#include "bin2d.h"
#include "namedobj.h"
#include "refcount.h"
class TrcKey;
template <class T> class TypeSet;


namespace Survey
{

class Geometry2D;
class Geometry3D;
mUseType( Pos, GeomID );


/*!\brief describes the Geometry of a survey; it tells you where traces can be
  located and makes sure you can work with both an indexing system and
  coordinates.

For 3D, this is the survey's inline/crossline system.
For 2D, each line has its own trace numbers with corresponding coordinates.
For synthetic data there is no subclass; the traces are not in a geometry.

There is, at the moment, no way to get Geometry's from other surveys than the
current survey. But, from 7.0 many stored 3D objects keep a copy of the
geometry in their storage format, provided by Geometry3D::get/setMapInfo().

Note that the Z range available is a hint to what the Z range could be in the
survey's Z Domain. To find out the real Z range (and Z Domain) of an object, you
need to turn to the actual object.

*/

mExpClass(Basic) Geometry : public RefCount::Referenced
			  , public ObjectWithName
{
public:

    mUseType( OD,		GeomSystem );
    mUseType( OD,		SnapDir );
    mUseType( Pos,		IdxPair );
    typedef Pos::Index_Type	idx_type;
    typedef idx_type		pos_type;
    typedef Pos::rg_type	pos_rg_type;
    typedef Pos::steprg_type	pos_steprg_type;
    typedef Pos::Z_Type		z_type;
    typedef ZSampling		z_steprg_type;
    typedef GeomID::IDType	linenr_type;
    typedef Pos::TraceNr_Type	trcnr_type;
    typedef Pos::Distance_Type	dist_type;

    virtual Geometry*	clone() const			= 0;
    virtual GeomSystem	geomSystem() const		= 0;
    bool		is2D() const
			{ return geomSystem() != OD::VolBasedGeom; }
    bool		is3D() const
			{ return geomSystem() == OD::VolBasedGeom; }
    virtual bool	isEmpty() const		{ return false; }

    GeomID		geomID() const		{ return geomid_; }

    static const Geometry3D&	get3D(OD::SurvLimitType slt=OD::FullSurvey);
    static const Geometry&	get(GeomID);
    static const Geometry&	get(const TrcKey&);
    static const Geometry2D&	get2D(GeomID);
    static const Geometry2D&	get2D(const char* linename);
    static GeomID		getGeomID(const char* linenm);
    static void			list2D(GeomIDSet&,BufferStringSet* nms=nullptr);
    static bool			isPresent(GeomID);
    static bool			isUsable(GeomID);

    inline idx_type	idx4TrcNr(pos_type) const;
    inline idx_type	idx4Z(z_type) const;
    inline pos_type	trcNr4Idx(idx_type) const;
    inline z_type	z4Idx(int) const;

    dist_type		distanceTo(const Coord&) const;
    void		getNearestTracePosition(const Coord&,TrcKey&,
				dist_type* d=nullptr) const;
    void		getTracePosition(const Coord&,TrcKey&,
				dist_type maxdist=mUdf(dist_type)) const;
    dist_type		averageTrcDist() const;

    pos_steprg_type&	trcNrRange()		{ return trcnrrg_; }
    const pos_steprg_type& trcNrRange() const	{ return trcnrrg_; }
    void		snapTrcNr(pos_type&,SnapDir d=OD::SnapNearest) const;
    void		snapTrcNrStep(pos_type&) const;

			// Z Range values are in the SI()'s ZDomain
    z_steprg_type&	zRange()		{ return zrg_; }
    const z_steprg_type& zRange() const		{ return zrg_; }
    void		snapZ(z_type&,SnapDir d=OD::SnapNearest) const;
    void		snapZStep(z_type&) const;

    Geometry2D*		as2D()			{ return gtAs2D(); }
    const Geometry2D*	as2D() const		{ return gtAs2D(); }
    Geometry3D*		as3D()			{ return gtAs3D(); }
    const Geometry3D*	as3D() const		{ return gtAs3D(); }

    static GeomID	cSynthGeomID();

    static bool		includes(const TrcKey&);

    static Coord	toCoord(GeomSystem,linenr_type,trcnr_type);
    static Coord	toCoord( GeomSystem gs, const IdxPair& ip )
			{ return toCoord(gs,ip.lineNr(),ip.trcNr());}
    static Coord	toCoord( const BinID& bid )
			{ return toCoord(OD::VolBasedGeom,bid.inl(),bid.crl());}
    static Coord	toCoord( GeomID gid, trcnr_type tnr )
			{ return toCoord(OD::LineBasedGeom,gid.lineNr(),tnr); }
    static Coord	toCoord( const Bin2D& b2d )
			{ return toCoord(OD::LineBasedGeom,
					 b2d.lineNr(),b2d.trcNr()); }
    static Coord	toCoord(const TrcKey&);

protected:
			~Geometry();
			Geometry(GeomID);
			Geometry( const Geometry& oth )	{ *this = oth; }
    Geometry&		operator =(const Geometry&);

    const GeomID	geomid_;
    pos_steprg_type	trcnrrg_;
    z_steprg_type	zrg_;

    static void		snapPos(pos_type&,const pos_steprg_type&,SnapDir);
    static void		snapStep(pos_type&,const pos_steprg_type&);

    virtual Geometry2D*	gtAs2D() const				= 0;
    virtual Geometry3D*	gtAs3D() const				= 0;

public:

    mDeprecated GeomID	getID() const		{ return geomID(); }
    mDeprecated const char* getName() const	{ return name().str(); }

    enum RelationType	{ UnRelated=0, Related, SubSet, SuperSet, Identical };
			    /*!< 'Related': same transform but no inclusion. */
    RelationType	compare(const Geometry&,bool usezrg) const;
    bool		isCompatibleWith(const Geometry&) const;

    void		setGeomID( GeomID gid )
			{ mNonConst(geomid_) = gid; }
    static Geometry2D&	get2D4Edit( GeomID gid )
			{ return const_cast<Geometry2D&>( get2D(gid) ); }

    z_type		zScale() const; //!< see SI().zScale()

};


inline Geometry::idx_type Geometry::idx4TrcNr( pos_type trcnr ) const
{ return trcNrRange().nearestIndex( trcnr ); }
inline Geometry::idx_type Geometry::idx4Z( z_type z ) const
{ return zRange().nearestIndex( z ); }
inline Geometry::pos_type Geometry::trcNr4Idx( idx_type idx ) const
{ return trcNrRange().atIndex( idx ); }
inline Geometry::z_type Geometry::z4Idx( idx_type idx ) const
{ return zRange().atIndex( idx ); }


} //namespace Survey
