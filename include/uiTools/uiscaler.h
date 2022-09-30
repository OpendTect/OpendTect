#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiScaler();

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
