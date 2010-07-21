#ifndef uimarkerstyle3d_h
#define uimarkerstyle3d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2010
 RCS:           $Id: uimarkerstyle.h,v 1.1 2010-07-21 07:09:21 cvskris Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

#include "draw.h"

class uiColorInput;
class uiGenInput;
class uiSliderExtra;


mClass uiMarkerStyle3D : public uiGroup
{
public:
			uiMarkerStyle3D(uiParent*,bool withcolor,
				const Interval<int>& sizerange,
				int nrexcluded=0,
				const MarkerStyle3D::Type* excluded=0);
    NotifierAccess*	sliderMove();
    NotifierAccess*	typeSel();
    NotifierAccess*	colSel();

    void    		setMarkerStyle(const MarkerStyle3D& style);
    void		getMarkerStyle(MarkerStyle3D& style) const;

protected:
    TypeSet<MarkerStyle3D::Type>	types_;

    uiSliderExtra*			sliderfld_;
    uiGenInput*				typefld_;
    uiColorInput*			colselfld_;
};

#endif
