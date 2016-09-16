#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2007
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
