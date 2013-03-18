#ifndef flatviewbitmapmgr_h
#define flatviewbitmapmgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "flatview.h"
#include "array2dbitmap.h"


namespace FlatView
{

/*!
\brief uiFlatView bitmap manager.
*/

mExpClass(uiFlatView) BitMapMgr
{
public:

			BitMapMgr(const Viewer&,bool wva);
			~BitMapMgr()			{ clearAll(); }

    void		setupChg();
    Geom::Point2D<int>	dataOffs(const Geom::PosRectangle<double>&,
	    			 const Geom::Size2D<int>&) const;
    			//!< Returns mUdf(int)'s when outside or incompatible

    bool		generate(const Geom::PosRectangle<double>& wr,
	    			 const Geom::Size2D<int>& bufwrsz,
				 const Geom::Size2D<int>& availpixels);
			//!< fails only when isufficient memory
    const A2DBitMap*	bitMap() const			{ return bmp_; }
    const A2DBitMapGenerator*	bitMapGen() const	{ return gen_; }

protected:

    const Viewer&		vwr_;
    A2DBitMap*			bmp_;
    A2DBitMapPosSetup*		pos_;
    A2DBitMapInpData*		data_;
    A2DBitMapGenerator*		gen_;
    bool			wva_;

    Geom::PosRectangle<double>	wr_;
    Geom::Size2D<int>		sz_;

    void			clearAll();
};


/*!
\brief uiFlatView bitmap generation Task.
*/

mExpClass(uiFlatView) BitMapGenTask : public Task
{
public:
    		BitMapGenTask(BitMapMgr& mgr,
			const Geom::PosRectangle<double>& wr,
			const Geom::Size2D<int>& bufwrsz,
			const Geom::Size2D<int>& pix )
		    : mgr_(mgr), wr_(wr), bufwrsz_(bufwrsz), availpixels_(pix){}

    bool	execute() { return mgr_.generate(wr_,bufwrsz_,availpixels_); }

protected:

    BitMapMgr& 				mgr_;
    const Geom::PosRectangle<double>& 	wr_;
    const Geom::Size2D<int>& 		bufwrsz_;
    const Geom::Size2D<int>& 		availpixels_;
};

} // namespace FlatView


#endif

