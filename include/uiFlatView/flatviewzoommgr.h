#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "geometry.h"
#include "sets.h"

namespace FlatView
{

/*!
\brief Manages zoom factors. Will always provide a new zoom when forward()
called, using fwdFac(). For back, will stop at initial.
*/

mExpClass(uiFlatView) ZoomMgr
{
public:

    typedef Geom::Size2D<double>	Size;
    typedef Geom::Point2D<double>	Point;


			ZoomMgr();
			~ZoomMgr();

    void		setNrViewers(int);

    void		init(const Geom::PosRectangle<double>&);
    void		reInit(const Geom::PosRectangle<double>&);
			//!< Will try to keep as many zooms as possible

    void		init(const TypeSet<Geom::PosRectangle<double> >&);
    void		reInit(const TypeSet<Geom::PosRectangle<double> >&);

    void		add(Size,int vieweridx=0);
			/*!< Will put this Size at the right place and makes it
			current. Also removes zooms larger than this one. */

    Size		current(int vieweridx=0) const;
    Size		back(int vieweridx,bool onlyvertical,
			     bool usefwdfac) const;
			/*!< Never past initial zoom.
			  \param vieweridx
			  \param onlyvertical if
			true, only the height of current size is multiplied by
			1/fwdfac_ to get new size.
			\param usefwdfac if true,
			multiplies current size by 1/fwdfac_ to get new size.
			Else returns previous size. */
    Size		forward(int vieweridx,bool onlyvertical,
				bool usefwdfac) const;
			/*!< Goes on and on.
			  \param vieweridx
			  \param onlyvertical if true, only
			the height of current size is multiplied by fwdfac_ to
			get new size.
			\param usefwdfac if true, or if there is
			no zoom larger than this one, multiplies current size by
			fwdfac_ to get new size. */

    bool		atStart(int vieweridx=-1) const;
			/*!<If vieweridx is not specified, returns true only
			if all viewers are at start. */
    void		toStart(int vieweridx=-1) const;
			/*!<If vieweridx is not specified, all viewers will be
			back to initial zoom. */

    int			nrZooms(int vieweridx=0) const;
    Size		initialSize(int vieweridx=0) const;
    Point               initialCenter(int vieweridx=0) const;

    double		fwdFac() const		{ return fwdfac_; }
    void		setFwdFac(double fac);
			//!< fwdfac_ should be greater than zero and less than
			//!< one.

protected:

    mutable TypeSet<int>	current_;

    struct ViewerZoomData
    {
    				TypeSet<Size>	zooms_;
    				Point		center_;
    };

    ObjectSet<ViewerZoomData>	viewerdata_;

    double			fwdfac_;

};

} // namespace FlatView
