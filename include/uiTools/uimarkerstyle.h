#pragma once
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
class uiCheckBox;
class uiGenInput;
class uiSpinBox;


mExpClass(uiTools) uiMarkerStyle : public uiGroup
{ mODTextTranslationClass(uiMarkerStyle)
public:
	mExpClass(uiTools) Setup
	{
	public:
			Setup( const uiString& txt=uiString::empty())
			    : lbltxt_(txt)
			    , wshape_(true)
			    , wcolor_(true)
			    , wtransparency_(false)
			    , wsz_(true)
			{}

	    mDefSetupMemb(uiString,lbltxt)
	    mDefSetupMemb(bool,wshape)
	    mDefSetupMemb(bool,wcolor)
	    mDefSetupMemb(bool,wsz)
	    mDefSetupMemb(bool,wtransparency)
	};

    void		setColor(const Color&);
    bool		showMarker() const;
    void		setShowMarker(bool);
    Color		getColor() const;
    void		setSize(int);
    int			getSize() const;

    Notifier<uiMarkerStyle> change;

protected:

			uiMarkerStyle(uiParent*);
			~uiMarkerStyle();

    uiGenInput*		typefld_;
    uiSpinBox*		sizefld_;
    uiColorInput*	colorfld_;

    virtual void	initGrp(CallBacker*);
    void		changeCB(CallBacker*);
    void		needmarkerCB(CallBacker*);
    void		createFlds(const uiStringSet&,const Setup&);
};


mExpClass(uiTools) uiMarkerStyle2D : public uiMarkerStyle
{ mODTextTranslationClass(uiMarkerStyle2D)
public:

			uiMarkerStyle2D(uiParent*,const Setup&,
				const TypeSet<OD::MarkerStyle2D::Type>* excl=0);

    void		setType(OD::MarkerStyle2D::Type);
    OD::MarkerStyle2D::Type getType() const;

    void		setMarkerStyle(const OD::MarkerStyle2D&);
    void		getMarkerStyle(OD::MarkerStyle2D&) const;

protected:
    TypeSet<OD::MarkerStyle2D::Type>	types_;
};


mExpClass(uiTools) uiMarkerStyle3D : public uiMarkerStyle
{ mODTextTranslationClass(uiMarkerStyle3D)
public:

			uiMarkerStyle3D(uiParent*,const Setup&,
				const TypeSet<OD::MarkerStyle3D::Type>* excl=0);

    void		setType(OD::MarkerStyle3D::Type);
    OD::MarkerStyle3D::Type getType() const;

    void		setMarkerStyle(const OD::MarkerStyle3D&);
    void		getMarkerStyle(OD::MarkerStyle3D&) const;

protected:
    TypeSet<OD::MarkerStyle3D::Type>	types_;
};
