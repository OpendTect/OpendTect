#ifndef flatviewbmp2rgb_h
#define flatviewbmp2rgb_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: flatviewbmp2rgb.h,v 1.8 2012-08-03 13:00:57 cvskris Exp $
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "flatview.h"
#include "array2dbitmap.h"
#include "uirgbarray.h"
class HistEqualizer;


namespace FlatView
{

/*!\brief Draws bitmaps on RGBArray according to FlatView specs.
	  Assumes bitmaps are 100% aligned with array, only sizes may differ. */

mClass(uiFlatView) BitMap2RGB
{
public:

			BitMap2RGB(const Appearance&,uiRGBArray&);

    void		draw(const A2DBitMap* wva,const A2DBitMap* vd,
	    		     const Geom::Point2D<int>& offset,
			     bool clearexisting = true);

    uiRGBArray&		rgbArray()		{ return arr_; }
    void		setRGBArr( const uiRGBArray& arr)	{ arr_ = arr; }
    void		setClipperData( const TypeSet<float>& clipdata )
			{ clipperdata_ = clipdata; }

protected:

    const Appearance&	app_;
    uiRGBArray&		arr_;
    HistEqualizer*	histequalizer_;
    TypeSet<float>& 	clipperdata_;

    void		drawVD(const A2DBitMap&,const Geom::Point2D<int>&);
    void		drawWVA(const A2DBitMap&,const Geom::Point2D<int>&);
};

} // namespace FlatView


#endif

