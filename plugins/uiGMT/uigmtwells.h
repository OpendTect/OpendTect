#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiListBox;
class uiSpinBox;
class uiGMTSymbolPars;

mClass(uiGMT) uiGMTWellsGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMTSymbolPars);
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMTWellsGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiListBox*		welllistfld_;
    uiGenInput*		namefld_;
    uiGMTSymbolPars*	symbfld_;
    uiCheckBox*		lebelfld_;
    uiComboBox*		lebelalignfld_;
    uiSpinBox*		labelfontszfld_;

    void		choiceSel(CallBacker*);
    void		fillItems();
};
