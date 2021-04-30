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
class uiGenInput;
class uiIOObjSel;
class uiGMTSymbolPars;

mClass(uiGMT) uiGMTLocationsGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMTLocationsGrp);
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMTLocationsGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    CtxtIOObj&		ctio_;

    uiIOObjSel*		inpfld_;
    uiGenInput*		namefld_;
    uiGMTSymbolPars*	symbfld_;

    void		objSel(CallBacker*);
};

