#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
