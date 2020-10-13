#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2010 (org 2005 / 2008)
________________________________________________________________________

-*/

#include "coord.h"
#include "bin2d.h"
#include "od_iosfwd.h"
#include "survgeom.h"
#include "typeset.h"
class LineSubSel;
class LineSubSelSet;

namespace PosInfo
{

class LineData;

/*!\brief the key (trace number) and the actual position (Coord) */

mExpClass(Basic) Line2DPos
{
public:

    typedef Pos::TraceNr_Type	trcnr_type;

		Line2DPos( trcnr_type n=0 ) : nr_(n)	{}
    bool	operator ==( const Line2DPos& p ) const	{ return nr_ == p.nr_; }
    bool	operator !=( const Line2DPos& p ) const	{ return nr_ != p.nr_; }
    bool	operator >( const Line2DPos& p ) const	{ return nr_ > p.nr_; }
    bool	operator <( const Line2DPos& p ) const	{ return nr_ < p.nr_; }
    bool	operator >=( const Line2DPos& p ) const	{ return nr_>=p.nr_; }
    bool	operator <=( const Line2DPos& p ) const	{ return nr_<=p.nr_; }

    trcnr_type	nr_;
    Coord	coord_;

};


/*!\brief Line2DPos with a z value */

mExpClass(Basic) Line2DPos3D : public Line2DPos
{
public:

    mUseType( SurvGeom,	z_type );

			Line2DPos3D( trcnr_type n=0, z_type z=mUdf(z_type) )
			    : Line2DPos(n), z_(z)		{}

    z_type	z_;
};


/*!\brief The full positional 'skeleton' for a 2D line.

  Note that only the bend points are kept. As a result, trace-by-trace
  iteration is done by iterating over the trcNrRange(). The size() reported
  is the number of bend points, and indexOf() will often return -1.

 */

mExpClass(Basic) Line2DData
{
public:

    mUseType( Pos,			GeomID );
    mUseType( Line2DPos,		trcnr_type );
    typedef TypeSet<Line2DPos>		PosSet;
    mUseType( PosSet,			idx_type );
    mUseType( PosSet,			size_type );
    typedef Interval<trcnr_type>	trcnr_rg_type;
    typedef StepInterval<trcnr_type>	trcnr_steprg_type;
    typedef TypeSet<trcnr_type>		TrcNrSet;
    typedef TypeSet<idx_type>		IdxSet;
    typedef Pos::Distance_Type		dist_type;
    typedef float			z_type;
    typedef StepInterval<z_type>	z_steprg_type;

			Line2DData(const char* lnm=0);

			Line2DData( const Line2DData& l2d )
			: zrg_(l2d.zRange())
			, lnm_(l2d.lineName())
			, posns_(l2d.positions()) {}

    const OD::String&	lineName() const	{ return lnm_; }
    GeomID		geomID() const;
    z_steprg_type&	zRange()		{ return zrg_; }
    const z_steprg_type& zRange() const		{ return zrg_; }

    bool		isEmpty() const		{ return posns_.isEmpty(); }
    bool		validIdx( idx_type idx ) const
						{ return posns_.validIdx(idx); }
    size_type		size() const		{ return posns_.size();}
    trcnr_type		trcNr(idx_type) const;
    Bin2D		bin2D(idx_type) const;
    Coord		coord(idx_type) const;
    bool		isPresent( trcnr_type tnr ) const
						{ return indexOf(tnr) >= 0; }
    idx_type		indexOf(trcnr_type) const;
    const PosSet&	positions() const	{ return posns_; }

    const TrcNrSet&	getBendPoints() const;
    void		getBendPositions(PosSet&) const;
    void		setBendPoints(const TrcNrSet&);

    idx_type		nearestIdx( const Coord& crd ) const
						{ return gtIndex(crd); }
    idx_type		nearestIdx(const Coord&,const trcnr_rg_type&) const;

    bool		getPos(const Coord& crd,Line2DPos& l2p,
			       dist_type* dist=0) const;
    bool		getPos(const Coord& crd,Line2DPos& l2p,
			       dist_type threshold_distance) const;
    bool		getPos(trcnr_type,Line2DPos&) const;

    void		add(const Line2DPos&);
    void		remove(trcnr_type);
    void		removeByIdx(idx_type);
    void		setEmpty()			     { posns_.erase(); }
    void		setLineName(const char*);
    void		setPositions( const PosSet& posns )  { posns_ = posns; }
    void		setZRange( const z_steprg_type& zr ) { zrg_ = zr; }
    void		limitTo(trcnr_type start,trcnr_type stop);

    void                dump(od_ostream&,bool pretty=true) const;
    bool		read(od_istream&,bool asc);
    bool		write(od_ostream&,bool asc,bool newlns=false) const;

    trcnr_steprg_type	trcNrRange() const;
    Coord		getNormal(trcnr_type trcnr) const;
    void		getTrcDistStats(dist_type& max,dist_type& median) const;
    dist_type		distBetween(trcnr_type start,trcnr_type stop) const;

    bool		coincidesWith(const Line2DData&) const;
			    /*!< do the lines have an overlap
				 and the same trace numbering system? */

    void		getSegments(LineData&) const;
    LineSubSel*		getSubSel() const;

    void		setGeomID(GeomID) const;

protected:

    BufferString	lnm_;
    z_steprg_type	zrg_;
    PosSet		posns_;
    IdxSet		bendpoints_;
    mutable GeomID	geomid_;

    idx_type		gtIndex(trcnr_type,bool&) const;
    idx_type		gtIndex(const Coord&,dist_type* sqdist=0) const;
    idx_type		getClosestBPSegment(const Coord&) const;
				//!< the index of the BP starting the segment

    friend class	Line2DDataIterator;

};


mExpClass(Basic) Line2DDataSet : public ManagedObjectSet<Line2DData>
{
public:

    mUseType( Pos,	GeomID );

    idx_type		lineIndexOf(GeomID) const;
    Line2DData*		find( GeomID gid )	{ return doFind( gid ); }
    const Line2DData*	find( GeomID gid ) const { return doFind( gid ); }

    void		getSubSel(LineSubSelSet&) const;
    od_int64		totalNrPositions() const;

protected:

    Line2DData*		doFind(GeomID) const;

};


/*!\brief Iterates through Line2DData. */

mExpClass(Basic) Line2DDataIterator
{
public:

    mUseType( Line2DData,	trcnr_type );
    mUseType( Line2DData,	idx_type );
    mUseType( Line2DData,	GeomID );

			Line2DDataIterator( const Line2DData& ld )
			    : ld_(ld), idx_(-1)	{}

    inline bool		next()
			{
			    idx_++;
			    return idx_ < ld_.posns_.size();
			}

    inline void		reset()		{ idx_ = -1; }
    inline const Line2DPos& line2DPos() const { return ld_.posns_[idx_]; }
    inline GeomID	geomID() const	{ return ld_.geomID(); }
    inline trcnr_type	trcNr() const
			{ return idx_>=0 ? ld_.posns_[idx_].nr_
					 : mUdf(trcnr_type); }
    inline void		setTrcNr( trcnr_type trcnr )
			{ idx_ = ld_.indexOf( trcnr ); }

    const Line2DData&	ld_;
    idx_type		idx_;

};


} // namespace PosInfo
