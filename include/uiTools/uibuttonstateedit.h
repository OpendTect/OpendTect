#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class uiGenInput;

/*!Simple field to edit OD::ButtonState. */

mExpClass(uiTools) uiButtonStateEdit : public uiGroup
{ mODTextTranslationClass(uiButtonStateEdit)
public:
    		uiButtonStateEdit(uiParent*,const uiString& label,
				  int initialstate);
    int		getState() const;
protected:
    static uiString	createName(int);
    uiGenInput*		combobox_;
    TypeSet<int>	states_;
};
