#ifndef flatviewzoommgr_h
#define flatviewzoommgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: flatviewzoommgr.h,v 1.5 2009-01-08 07:14:05 cvsranojay Exp $
________________________________________________________________________

-*/

#include "geometry.h"
#include "sets.h"


namespace FlatView
{

/*!\brief Manages zoom factors. Will always provide a new zoom when forward()
          called, using fwdFac(). For back, will stop at initial. */

mClass ZoomMgr
{
public:

    typedef Geom::Size2D<double>	Size;
    typedef Geom::Point2D<double>	Point;


			ZoomMgr()
			    : cur_(-1)
			    , fwdfac_(0.8)	{}

    void		init(const Geom::Rectangle<double>&);
    void		reInit(const Geom::Rectangle<double>&);
    				//!< Will try to keep as many zooms as possible
    void		add(Size);
    				//!< Will put this Size at the right place
    				//!< and make it current
    				//!< will remove zooms larger than this one

    Size		current() const;
    Size		back() const;		//!< never past initial zoom
    Size		forward() const;	//!< goes on and on
    bool		atStart() const		{ return cur_ < 1; }
    Size		toStart() const;
    int			nrZooms() const		{ return zooms_.size(); }

    double		fwdFac() const		{ return fwdfac_; }
    void		setFwdFac( double fac )	{ fwdfac_ = fac; }

    Point		initialCenter() const	{ return center_; }

protected:

    mutable int		cur_;
    TypeSet<Size>	zooms_;
    Point		center_;
    double		fwdfac_;

};

} // namespace FlatView


#endif
