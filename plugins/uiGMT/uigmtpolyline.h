#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class CtxtIOObj;
class uiColorInput;
class uiGenInput;
class uiIOObjSel;
class uiSelLineStyle;

mClass(uiGMT) uiGMTPolylineGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMTPolylineGrp);
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMTPolylineGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    CtxtIOObj&		ctio_;

    uiIOObjSel*		inpfld_;
    uiGenInput*		namefld_;
    uiSelLineStyle*	lsfld_;
    uiColorInput*	fillcolfld_;

    void		objSel(CallBacker*);
};

