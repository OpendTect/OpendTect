#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiGenInput;
class uiIOObjSel;

mClass(uiGMT) uiGMTClipGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMTClipGrp);
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

    static bool		getTerminatingPars(IOPar&);

protected:

    			uiGMTClipGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    void		actionSel(CallBacker*);

    uiGenInput*		actionfld_;
    uiIOObjSel*		polygonfld_;
    uiGenInput*		optionfld_;
};
