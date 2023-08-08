#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "draw.h"
#include "ranges.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uistrings.h"

#include "mnemonics.h"

class uiCheckBox;
class uiGenInput;
class uiSelLineStyle;
class uiUnitSel;
class UnitOfMeasure;


/*!\brief Selector for Mnemonic properties */

mExpClass(uiTools) uiMnemonicProperties : public uiGroup
{ mODTextTranslationClass(uiMnemonicProperties);
public:

				uiMnemonicProperties(uiParent*);
				~uiMnemonicProperties();

    void			setScale(const Mnemonic::Scale);
    void			setRange(const Interval<float>&);
    void			setLineStyle(const OD::LineStyle&);
    void			setUOM(const UnitOfMeasure*);

    Mnemonic::Scale		getScale() const;
    Interval<float>		getRange() const;
    OD::LineStyle		getLineStyle() const;
    BufferString		getUOMStr() const;
    BufferString		toString() const;

    Notifier<uiMnemonicProperties>	valueChanged;

protected:

    void		changedCB(CallBacker*);

    uiGenInput*		scalefld_;
    uiGenInput*		rangefld_;
    uiSelLineStyle*	linestylefld_;
    uiUnitSel*		uomfld_;
};
