#ifndef flatviewzoommgr_h
#define flatviewzoommgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: flatviewzoommgr.h,v 1.1 2007-02-23 09:35:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "geometry.h"
#include "sets.h"


namespace FlatView
{

/*!\brief Manages zoom factors. Will always provide a new zoom when forward()
          called, using fwdFac(). For back, will stop at initial. */

class ZoomMgr
{
public:

    typedef Geom::Size2D<double>	Size;
    typedef Geom::Point2D<double>	Point;


			ZoomMgr()
			    : cur_(-1)
			    , fwdfac_(0.8)	{}

    void		init(const Geom::Rectangle<double>&);
    void		add(Size);
    				//!< removes zooms 'above' current

    Size		current() const;
    Size		back() const;		//!< never past initial zoom
    Size		forward() const;	//!< goes on and on
    bool		atStart() const		{ return cur_ < 1; }
    Size		toStart() const;
    Point		initialCenter() const	{ return center_; }

    double		fwdFac() const		{ return fwdfac_; }
    void		setFwdFac( double fac )	{ fwdfac_ = fac; }

protected:

    mutable int		cur_;
    TypeSet<Size>	zooms_;
    Point		center_;
    double		fwdfac_;

};

} // namespace FlatView


#endif
