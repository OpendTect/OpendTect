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
#include "typeset.h"

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

    static int		cDefMaxMarkerSize()	{ return 18; }

protected:

			uiMarkerStyle(uiParent*);

    uiSlider*		sizefld_;
    uiGenInput*		typefld_;
    uiColorInput*	colselfld_;

    TypeSet<int>	types_;
    void		createFlds(const uiStringSet&,bool withcolor,
				   const Interval<int>& szrg);
    void		setMStyle(int typ,int sz, const Color&);
};


mExpClass(uiTools) uiMarkerStyle2D : public uiMarkerStyle
{ mODTextTranslationClass(uiMarkerStyle2D)
public:

			uiMarkerStyle2D(uiParent*,bool withcolor,
				Interval<int> sizerange
				    =Interval<int>(1,cDefMaxMarkerSize()),
				const TypeSet<OD::MarkerStyle2D::Type>* excl=0);

    OD::MarkerStyle2D::Type getType() const;

    void		setMarkerStyle(const OD::MarkerStyle2D& style);
    void		getMarkerStyle(OD::MarkerStyle2D& style) const;

};


mExpClass(uiTools) uiMarkerStyle3D : public uiMarkerStyle
{ mODTextTranslationClass(uiMarkerStyle3D)
public:

			uiMarkerStyle3D(uiParent*,bool withcolor,
				Interval<int> sizerange
				    =Interval<int>(1,cDefMaxMarkerSize()),
				const TypeSet<OD::MarkerStyle3D::Type>* excl=0);

    OD::MarkerStyle3D::Type getType() const;

    void		setMarkerStyle(const OD::MarkerStyle3D&);
    void		getMarkerStyle(OD::MarkerStyle3D&) const;

};


#endif
