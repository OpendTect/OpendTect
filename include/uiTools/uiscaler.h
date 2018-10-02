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

class Scaler;
class uiGenInput;
class uiCheckBox;
class uiComboBox;


/*!\brief single line element allowing user to optionally select a scaling */

mExpClass(uiTools) uiScaler : public uiGroup
{ mODTextTranslationClass(uiScaler);
public:

			uiScaler(uiParent*,const uiString& tx=uiString::empty(),
				 bool linear_only=false);
				//!< default text is "Scale values"

    Scaler*		getScaler() const;
    void		setInput(const Scaler&);
    void		setUnscaled();

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    uiCheckBox*		ynfld_;
    uiComboBox*		typefld_		= 0;
    uiGenInput*		linearfld_;
    uiGenInput*		basefld_		= 0;

    void		doFinalise(CallBacker*);
    void		typeSel(CallBacker*);
};
