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


mExpClass(uiTools) uiMarkerStyle : public uiGroup
{ mODTextTranslationClass(uiMarkerStyle)
public:
    Color		getColor() const;
    int			getSize() const;

    NotifierAccess*	sizeChange();
    NotifierAccess*	typeSel();
    NotifierAccess*	colSel();

protected:
			uiMarkerStyle(uiParent*,bool withcolor,
				const Interval<int>& sizerange);

    uiSlider*		sizefld_;
    uiGenInput*		typefld_;
    uiColorInput*	colselfld_;
};


mExpClass(uiTools) uiMarkerStyle2D : public uiMarkerStyle
{ mODTextTranslationClass(uiMarkerStyle2D)
public:
			uiMarkerStyle2D(uiParent*,bool withcolor,
				const Interval<int>& sizerange,
				int nrexcluded=0,
				const OD::MarkerStyle2D::Type* excluded=0);

    OD::MarkerStyle2D::Type getType() const;

    void		setMarkerStyle(const OD::MarkerStyle2D& style);
    void		getMarkerStyle(OD::MarkerStyle2D& style) const;

protected:

    void		finalizeDone(CallBacker*);
    EnumDefImpl<OD::MarkerStyle2D::Type> markertypedef_;
};


mExpClass(uiTools) uiMarkerStyle3D : public uiMarkerStyle
{ mODTextTranslationClass(uiMarkerStyle3D)
public:
			uiMarkerStyle3D(uiParent*,bool withcolor,
				const Interval<int>& sizerange,
				int nrexcluded=0,
				const OD::MarkerStyle3D::Type* excluded=0);

    OD::MarkerStyle3D::Type getType() const;

    void		setMarkerStyle(const OD::MarkerStyle3D& style);
    void		getMarkerStyle(OD::MarkerStyle3D& style) const;

protected:

    void		finalizeDone(CallBacker*);
    EnumDefImpl<OD::MarkerStyle3D::Type> markertypedef_;
};

#endif
