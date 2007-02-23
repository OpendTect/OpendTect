#ifndef flatviewbitmapmgr_h
#define flatviewbitmapmgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: flatviewbitmapmgr.h,v 1.1 2007-02-23 09:35:33 cvsbert Exp $
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

    void			clearAll();
};

} // namespace FlatView


#endif
