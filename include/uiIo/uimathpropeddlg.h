#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

namespace Math { class Formula; }
class MathProperty;
class PropertyRefSelection;
class uiMathFormula;


mExpClass(uiIo) uiMathPropEdDlg : public uiDialog
{ mODTextTranslationClass(uiMathPropEdDlg);
public:
			uiMathPropEdDlg(uiParent*,MathProperty&,
					const PropertyRefSelection&);

			~uiMathPropEdDlg();

protected:

    MathProperty&	prop_;
    uiMathFormula*	formfld_;
    const PropertyRefSelection& prs_;

    void		rockPhysReq(CallBacker*);
    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
