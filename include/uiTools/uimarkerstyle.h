#ifndef uimarkerstyle_h
#define uimarkerstyle_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2010
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
				Interval<int> sizerange=Interval<int>(1,99),
				int nrexcluded=0,
				const OD::MarkerStyle3D::Type* excluded=0);

    OD::MarkerStyle3D::Type getType() const;
    Color		getColor() const;
    int			getSize() const;

    void		setMarkerStyle(const OD::MarkerStyle3D& style);
    void		getMarkerStyle(OD::MarkerStyle3D& style) const;

    NotifierAccess*	sizeChange();
    NotifierAccess*	typeSel();
    NotifierAccess*	colSel();

protected:

    uiSlider*				sizefld_;
    uiGenInput*				typefld_;
    uiColorInput*			colselfld_;
    EnumDefImpl<OD::MarkerStyle3D::Type> markertypedef_;
};

#endif
