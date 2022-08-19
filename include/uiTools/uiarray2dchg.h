#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "array2dfilter.h"
#include "uigroup.h"
#include "uidialog.h"
#include "uistring.h"

class uiGenInput;
class uiStepOutSel;


mExpClass(uiTools) uiArr2DFilterPars : public uiGroup
{ mODTextTranslationClass(uiArr2DFilterPars);
public:

				uiArr2DFilterPars(uiParent*,
					const Array2DFilterPars* p=0);

    Array2DFilterPars	getInput() const;

protected:

    uiGenInput*		medianfld_;
    uiStepOutSel*	stepoutfld_;

};


mExpClass(uiTools) uiArr2DFilterParsDlg : public uiDialog
{ mODTextTranslationClass(uiArr2DFilterParsDlg);
public:

			uiArr2DFilterParsDlg(uiParent*,
					     const Array2DFilterPars* p=0);

    Array2DFilterPars	getInput() const
			{ return fld->getInput(); }

    uiArr2DFilterPars*	fld;

};
