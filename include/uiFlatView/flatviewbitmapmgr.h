#ifndef flatviewbitmapmgr_h
#define flatviewbitmapmgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: flatviewbitmapmgr.h,v 1.2 2007-08-28 15:25:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "array2dbitmap.h"


namespace FlatView
{

class BitMapMgr
{
public:

			BitMapMgr(const Viewer&,bool wva);
			~BitMapMgr()		{ clearAll(); }

    void		setupChg();
    Geom::Point2D<int>	dataOffs(const Geom::PosRectangle<double>&,
	    			 const Geom::Size2D<int>&) const;
    			//!< Returns mUdf(int)'s when outside or incompatible

    bool		generate(const Geom::PosRectangle<double>&,
	    			 const Geom::Size2D<int>&);
			//!< fails only when isufficient memory
    const A2DBitMap*	bitMap() const	{ return bmp_; }

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

} // namespace FlatView


#endif
