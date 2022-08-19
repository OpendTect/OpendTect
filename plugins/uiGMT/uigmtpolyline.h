#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
