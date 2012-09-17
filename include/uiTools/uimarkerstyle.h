#ifndef uimarkerstyle_h
#define uimarkerstyle_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2010
 RCS:           $Id: uimarkerstyle.h,v 1.3 2010/08/04 14:49:36 cvsbert Exp $
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

    MarkerStyle3D::Type	getType() const;
    Color		getColor() const;
    int			getSize() const;

    void    		setMarkerStyle(const MarkerStyle3D& style);
    void		getMarkerStyle(MarkerStyle3D& style) const;

protected:
    TypeSet<MarkerStyle3D::Type>	types_;

    uiSliderExtra*			sliderfld_;
    uiGenInput*				typefld_;
    uiColorInput*			colselfld_;
};

#endif
