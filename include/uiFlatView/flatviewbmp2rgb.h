#ifndef flatviewbmp2rgb_h
#define flatviewbmp2rgb_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: flatviewbmp2rgb.h,v 1.2 2007-03-10 12:13:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "array2dbitmap.h"
class uiRGBArray;


namespace FlatView
{

/*!\brief Draws bitmaps on RGBArray according to FlatView specs.
	  Assumes bitmaps are 100% aligned with array, only sizes may differ. */

class BitMap2RGB
{
public:

			BitMap2RGB(const Appearance&,uiRGBArray&);

    void		draw(const A2DBitMap* wva,const A2DBitMap* vd);

    uiRGBArray&		rgbArray()		{ return arr_; }

protected:

    const Appearance&	app_;
    uiRGBArray&		arr_;

    void		drawVD(const A2DBitMap&);
    void		drawWVA(const A2DBitMap&);
};

} // namespace FlatView


#endif
