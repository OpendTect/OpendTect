#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2009
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
