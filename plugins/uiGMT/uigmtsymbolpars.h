#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigroup.h"

class uiComboBox;
class uiGenInput;
class uiColorInput;


mClass(uiGMT) uiGMTSymbolPars : public uiGroup
{ mODTextTranslationClass(uiGMTSymbolPars);
public:
			uiGMTSymbolPars(uiParent*,bool);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    uiComboBox*		shapefld_;
    uiGenInput*		sizefld_;
    uiColorInput*	outcolfld_;
    uiColorInput*	fillcolfld_;
    bool		usewellsymbols_;

    void		fillSel(CallBacker*);
    void		fillShapes();
};
