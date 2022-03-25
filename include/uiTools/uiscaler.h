#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uistrings.h"

class Scaler;
class uiGenInput;
class uiCheckBox;


mExpClass(uiTools) uiScaler : public uiGroup
{ mODTextTranslationClass(uiScaler);
public:

			uiScaler(uiParent*, const uiString &txt=
				 uiStrings::sEmptyString(), // "Scale values"
				 bool linear_only=false);

    Scaler*		getScaler() const;
    void		setInput(const Scaler&);
    void		setUnscaled();

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    uiCheckBox*		ynfld;
    uiGenInput*		typefld;
    uiGenInput*		linearfld;
    uiGenInput*		basefld;

    void		doFinalize(CallBacker*);
    void		typeSel(CallBacker*);
};


