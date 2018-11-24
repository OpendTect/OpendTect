#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2010 (org 2005 / 2008)
________________________________________________________________________

-*/

#include "basicmod.h"
#include "coord.h"
#include "od_iosfwd.h"
#include "survgeom.h"
#include "typeset.h"

namespace PosInfo
{

class LineData;

/*!\brief the key (trace number) and the actual position (Coord) */

mExpClass(Basic) Line2DPos
{
public:

    typedef Pos::TraceNr_Type	tracenr_type;

		Line2DPos( tracenr_type n=0 ) : nr_(n)	{}
    bool	operator ==( const Line2DPos& p ) const	{ return nr_ == p.nr_; }
    bool	operator !=( const Line2DPos& p ) const	{ return nr_ != p.nr_; }
    bool	operator >( const Line2DPos& p ) const	{ return nr_ > p.nr_; }
    bool	operator <( const Line2DPos& p ) const	{ return nr_ < p.nr_; }
    bool	operator >=( const Line2DPos& p ) const	{ return nr_>=p.nr_; }
    bool	operator <=( const Line2DPos& p ) const	{ return nr_<=p.nr_; }

    tracenr_type nr_;
    Coord	coord_;

};


/*!\brief Line2DPos with a z value */

mExpClass(Basic) Line2DPos3D : public Line2DPos
{
public:

    typedef Survey::Geometry::z_type	z_type;

			Line2DPos3D( tracenr_type n=0, z_type z=mUdf(z_type) )
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
    mUseType( Line2DPos,		tracenr_type );
    typedef TypeSet<Line2DPos>		PosSet;
    mUseType( PosSet,			idx_type );
    mUseType( PosSet,			size_type );
    typedef Interval<tracenr_type>	tracenr_rg_type;
    typedef StepInterval<tracenr_type>	tracenr_steprg_type;
    typedef TypeSet<tracenr_type>	TrcNrSet;
    typedef TypeSet<idx_type>		IdxSet;
    typedef Pos::Distance_Type		dist_type;
    typedef float			z_type;
    typedef StepInterval<z_type>	z_steprg_type;

			Line2DData(const char* lnm=0);

			Line2DData(const Line2DData& l2d)
			: zrg_(l2d.zRange())
			, lnm_(l2d.lineName())
			, posns_(l2d.positions()){}

    GeomID		geomID() const;
    const OD::String&	lineName() const	{ return lnm_; }
    z_steprg_type&	zRange()		{ return zrg_; }
    const z_steprg_type& zRange() const		{ return zrg_; }

    bool		isEmpty() const		{ return posns_.isEmpty(); }
    bool		validIdx( idx_type idx ) const
						{ return posns_.validIdx(idx); }
    idx_type		indexOf(tracenr_type) const;
    size_type		size() const		{ return posns_.size();}
    const PosSet&	positions() const	{ return posns_; }

    const TrcNrSet&	getBendPoints() const;
    void		getBendPositions(PosSet&) const;
    void		setBendPoints(const TrcNrSet&);

    idx_type		nearestIdx( const Coord& crd ) const
						{ return gtIndex(crd); }
    idx_type		nearestIdx(const Coord&,const tracenr_rg_type&) const;

    bool		getPos(const Coord& crd,Line2DPos& l2p,
			       dist_type* dist=0) const;
    bool		getPos(const Coord& crd,Line2DPos& l2p,
			       dist_type threshold_distance) const;
    bool		getPos(tracenr_type,Line2DPos&) const;

    void		add(const Line2DPos&);
    void		remove(tracenr_type);
    void		removeByIdx(idx_type);
    void		setEmpty()			     { posns_.erase(); }
    void		setLineName( const char* lnm )	     { lnm_ = lnm; }
    void		setPositions( const PosSet& posns )  { posns_ = posns; }
    void		setZRange( const z_steprg_type& zr ) { zrg_ = zr; }
    void		limitTo(tracenr_type start,tracenr_type stop);

    void                dump(od_ostream&,bool pretty=true) const;
    bool		read(od_istream&,bool asc);
    bool		write(od_ostream&,bool asc,bool newlns=false) const;

    tracenr_steprg_type	trcNrRange() const;
    Coord		getNormal(tracenr_type trcnr) const;
    void		getTrcDistStats(dist_type& max,dist_type& median) const;
    dist_type		distBetween(tracenr_type start,tracenr_type stop) const;

    bool		coincidesWith(const Line2DData&) const;
			    /*!< do the lines have an overlap
				 and the same trace numbering system? */

    void		getSegments(LineData&) const;

protected:

    BufferString	lnm_;
    z_steprg_type	zrg_;
    PosSet		posns_;
    IdxSet		bendpoints_;

    idx_type		gtIndex(tracenr_type,bool&) const;
    idx_type		gtIndex(const Coord&,dist_type* sqdist=0) const;
    idx_type		getClosestBPSegment(const Coord&) const;
				//!< the index of the BP starting the segment

    friend class	Line2DDataIterator;

};


/*!\brief Iterates through Line2DData. */

mExpClass(Basic) Line2DDataIterator
{
public:

    mUseType( Line2DData,	tracenr_type );
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
    inline tracenr_type	trcNr() const
			{ return idx_>=0 ? ld_.posns_[idx_].nr_
					 : mUdf(tracenr_type); }
    inline void		setTrcNr( tracenr_type trcnr )
			{ idx_ = ld_.indexOf( trcnr ); }

    const Line2DData&	ld_;
    idx_type		idx_;

};


} // namespace PosInfo
