#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiLineEdit;

mClass(uiGMT) uiGMTAdvGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMTAdvGrp);
public:

    static void		initClass();

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;
    void		reset() override;
protected:

    			uiGMTAdvGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiLineEdit*		inpfld_;
};
