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
class uiSpinBox;


mExpClass(uiTools) uiMarkerStyle2D : public uiGroup
{ mODTextTranslationClass(uiMarkerStyle2D)
public:
	mExpClass(uiTools) Setup
	{
	public:
			Setup( const uiString& txt=uiString::emptyString())
			    : lbltxt_(txt)
			    , shape_(true)
			    , color_(true)
			    , transparency_(false)
			    , sz_(true)
			{}
	    mDefSetupMemb(uiString,lbltxt)
	    mDefSetupMemb(bool,shape)
	    mDefSetupMemb(bool,color)
	    mDefSetupMemb(bool,sz)
	    mDefSetupMemb(bool,transparency)
	    mDefSetupMemb(TypeSet<MarkerStyle2D::Type>,toexclude)
	};
			uiMarkerStyle2D(uiParent*,const Setup&);
			~uiMarkerStyle2D();

    void		setMarkerType(MarkerStyle2D::Type);
    MarkerStyle2D::Type getMarkerType() const;
    void		setMarkerColor(const Color&);
    Color		getMarkerColor() const;
    void		setMarkerSize(int);
    int			getMarkerSize() const;

    void		setMarkerStyle(const MarkerStyle2D&);
    MarkerStyle2D	getMarkerStyle() const;

    Notifier<uiMarkerStyle2D>	changed;

protected:
    TypeSet<MarkerStyle2D::Type>	types_;

    uiGenInput*				typefld_;
    uiColorInput*			colorfld_;
    uiSpinBox*				szfld_;

    void		changeCB(CallBacker*);
};



mExpClass(uiTools) uiMarkerStyle3D : public uiGroup
{ mODTextTranslationClass(uiMarkerStyle3D)
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
