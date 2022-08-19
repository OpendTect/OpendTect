#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiseissel.h"
#include "veldesc.h"

class uiGenInput;
class uiLabeledComboBox;

/*!Group that allows the user to edit StaticsDesc information. */

mExpClass(uiSeis) uiStaticsDesc : public uiGroup
{ mODTextTranslationClass(uiStaticsDesc);
public:

    				uiStaticsDesc(uiParent*,const StaticsDesc* s=0);

    bool			get(StaticsDesc&,bool displayerrors) const;
    void			set(const StaticsDesc&);
    bool			updateAndCommit(IOObj&,bool displayerrors);

protected:

    void			updateFlds(CallBacker*);

    uiIOObjSel*			horfld_;
    uiGenInput*			useconstantvelfld_;
    uiGenInput*			constantvelfld_;
    uiLabeledComboBox*		horattribfld_;
};
