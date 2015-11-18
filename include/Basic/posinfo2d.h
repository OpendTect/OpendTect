#ifndef posinfo2d_h
#define posinfo2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2010 (org 2005 / 2008)
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "typeset.h"
#include "coord.h"
#include "bufstring.h"
#include "od_iosfwd.h"


namespace PosInfo
{

/*!
\brief One position on a 2D line.
*/

mExpClass(Basic) Line2DPos
{
public:

		Line2DPos( int n=0 ) : nr_(n)		{}
    bool	operator ==( const Line2DPos& p ) const	{ return nr_ == p.nr_; }
    bool	operator !=( const Line2DPos& p ) const	{ return nr_ != p.nr_; }
    bool	operator >( const Line2DPos& p ) const	{ return nr_ > p.nr_; }
    bool	operator <( const Line2DPos& p ) const	{ return nr_ < p.nr_; }
    bool	operator >=( const Line2DPos& p ) const	{ return nr_>=p.nr_; }
    bool	operator <=( const Line2DPos& p ) const	{ return nr_<=p.nr_; }

    int		nr_;
    Coord	coord_;

};


/*!
\brief Line2DPos with a z value.
*/

mExpClass(Basic) Line2DPos3D : public Line2DPos
{
public:
		Line2DPos3D( int n=0, float z=mUdf(float) )
		    : Line2DPos(n), z_(z)		{}

    float	z_;
};


/*!
\brief Position info for a 2D line.
*/

mExpClass(Basic) Line2DData
{
public:
			Line2DData(const char* lnm=0);

			Line2DData(const Line2DData& l2d)
			: zrg_(l2d.zRange())
			, lnm_(l2d.lineName())
			, posns_(l2d.positions()){}

    const StepInterval<float>& zRange() const		{ return zrg_; }
    const OD::String&	lineName() const		{ return lnm_; }
    void		setZRange( const StepInterval<float>& zrg )
							{ zrg_ = zrg; }
    void		setLineName( const char* lnm )	{ lnm_ = lnm; }
    const TypeSet<Line2DPos>&  positions() const	{ return posns_; }
    bool		isEmpty() const		{ return posns_.isEmpty(); }

    void		add(const Line2DPos&);
    void		remove(int trcnr);
    void		setEmpty()		{ posns_.erase(); }
    void		limitTo(Interval<int> trcrg);

    int			indexOf(int trcnr) const;
    int			nearestIdx( const Coord& crd ) const
						{ return gtIndex(crd); }
    int			nearestIdx(const Coord&,
				   const Interval<int>& trcnrrg) const;

    bool		getPos(const Coord& crd,Line2DPos& l2p,
			       float* dist=0) const;
    bool		getPos(const Coord& crd,Line2DPos& l2p,
			       float threshold_distance) const;
    bool		getPos(int trcnr,Line2DPos&) const;

    void                dump(od_ostream&,bool pretty=true) const;
    bool		read(od_istream&,bool asc);
    bool		write(od_ostream&,bool asc,bool newlns=false) const;

    StepInterval<Pos::TraceID>	trcNrRange() const;
    Coord		getNormal(int trcnr) const;
    void		compDistBetwTrcsStats(float& max, float& median) const;
    float		distBetween(int startnr,int stopnr) const;

    bool		coincidesWith(const Line2DData&) const;
			/*!< A true return value means they have at least one
			  trace number in common and all common trace numbers
			  have the same coordinates on either line. */

protected:

    StepInterval<float> zrg_;
    BufferString	lnm_;
    TypeSet<Line2DPos>  posns_;

    int			gtIndex(int,bool&) const;
    int			gtIndex(const Coord&,double* sqdist=0) const;

    friend class	Line2DDataIterator;

};


/*!\brief Iterates through Line2DData. */

mExpClass(Basic) Line2DDataIterator
{
public:

			Line2DDataIterator( const Line2DData& ld )
			    : ld_(ld), idx_(-1)	{}

    inline bool		next()
			{
			    idx_++;
			    return idx_ < ld_.posns_.size();
			}

    inline void		reset()		{ idx_ = -1; }
    inline const Line2DPos& line2DPos() const { return ld_.posns_[idx_]; }
    inline int		trcNr() const	{ return ld_.posns_[idx_].nr_; }

    const Line2DData&	ld_;
    int			idx_;

};


} // namespace PosInfo

#endif


