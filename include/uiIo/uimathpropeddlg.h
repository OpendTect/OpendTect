#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          Nov 2013
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

namespace Math { class Formula; }
class MathProperty;
class PropertySelection;
class uiMathFormula;


mExpClass(uiIo) uiMathPropEdDlg : public uiDialog
{ mODTextTranslationClass(uiMathPropEdDlg);
public:

			uiMathPropEdDlg(uiParent*,MathProperty&,
					const PropertySelection&);

			~uiMathPropEdDlg();

protected:

    MathProperty&	prop_;
    uiMathFormula*	formfld_;
    const PropertySelection& prs_;

    void		inpSel(CallBacker*);
    void		formSet(CallBacker*);
    void		rockPhysReq(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		setPType4Inp(int);

};

