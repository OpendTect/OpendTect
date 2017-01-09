#ifndef uimarkerstyle_h
#define uimarkerstyle_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

#include "draw.h"

class uiColorInput;
class uiGenInput;
class uiSlider;


mExpClass(uiTools) uiMarkerStyle3D : public uiGroup
{ mODTextTranslationClass(uiMarkerStyle3D);
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

    void		setMarkerStyle(const MarkerStyle3D& style);
    void		getMarkerStyle(MarkerStyle3D& style) const;
    void		enableColorSelection(bool);

protected:
    TypeSet<MarkerStyle3D::Type>	types_;

    uiSlider*				sliderfld_;
    uiGenInput*				typefld_;
    uiColorInput*			colselfld_;
};

#endif
