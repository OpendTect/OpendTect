#ifndef posinfo2d_h
#define posinfo2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2010 (org 2005 / 2008)
 RCS:		$Id: posinfo2d.h,v 1.2 2010/08/19 11:28:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "typeset.h"
#include "position.h"
class BinID;


namespace PosInfo
{

/*!\brief One position on a 2D line */

mClass Line2DPos
{
public:

		Line2DPos( int n=0 ) : nr_(n)            {}
    bool	operator ==( const Line2DPos& p ) const { return nr_ == p.nr_; }
    bool	operator !=( const Line2DPos& p ) const { return nr_ != p.nr_; }
    bool	operator >( const Line2DPos& p ) const  { return nr_ > p.nr_; }
    bool	operator <( const Line2DPos& p ) const  { return nr_ < p.nr_; }
    bool	operator >=( const Line2DPos& p ) const  { return nr_>=p.nr_; }
    bool	operator <=( const Line2DPos& p ) const  { return nr_<=p.nr_; }

    int		nr_;
    Coord	coord_;

};


/*!\brief Position info for a 2D line */

mClass Line2DData
{
public:
			Line2DData(const char* lnm=0);

    const StepInterval<float>& zRange() const		{ return zrg_; }
    const BufferString&	lineName() const		{ return lnm_; }
    void		setZRange( const StepInterval<float>& zrg )
							{ zrg_ = zrg; }
    void		setLineName(const BufferString& lnm)
			{ lnm_ = lnm; }
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
	    		       double threshold_distance) const;
    bool		getPos(int trcnr,Line2DPos&) const;

    void                dump(std::ostream&,bool pretty=true) const;
    bool		read(std::istream&,bool asc);
    bool		write(std::ostream&,bool asc,bool newlns=false) const;

    StepInterval<int>	trcNrRange() const;
    Coord		getNormal(int trcnr) const;

protected:

    StepInterval<float> zrg_;
    BufferString	lnm_;			
    TypeSet<Line2DPos>  posns_;

    int			gtIndex(int,bool&) const;
    int			gtIndex(const Coord&,double* sqdist=0) const;

};

} // namespace PosInfo

#endif
