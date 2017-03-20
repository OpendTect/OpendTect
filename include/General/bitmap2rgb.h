#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2007
________________________________________________________________________

-*/

#include "generalmod.h"
#include "array2dbitmap.h"

namespace FlatView { class Appearance; }
namespace Geom { template <class T> class Point2D; }
namespace OD { class RGBImage; }

/*!\brief Draws bitmaps on RGBArray according to FlatView specs.
  Assumes bitmaps are 100% aligned with array, only sizes may differ.
*/

mExpClass(General) BitMap2RGB
{
public:

			BitMap2RGB(const FlatView::Appearance&,OD::RGBImage&);
			~BitMap2RGB();

    void		draw(const A2DBitMap* wva,const A2DBitMap* vd,
			     const Geom::Point2D<int>& offset,
			     bool clearexisting=true);

    OD::RGBImage&	rgbArray();
    void		setRGBArray(const OD::RGBImage&);

protected:

    const FlatView::Appearance& app_;
    OD::RGBImage&	array_;

    void		drawVD(const A2DBitMap&,const Geom::Point2D<int>&);
    void		drawWVA(const A2DBitMap&,const Geom::Point2D<int>&);

};
