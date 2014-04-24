#ifndef uimathpropeddlg_h
#define uimathpropeddlg_h
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
class PropertyRefSelection;
class uiMathFormula;


mExpClass(uiIo) uiMathPropEdDlg : public uiDialog
{
public:

			uiMathPropEdDlg(uiParent*,MathProperty&,
					const PropertyRefSelection&);

			~uiMathPropEdDlg();

protected:

    MathProperty&	prop_;
    uiMathFormula*	formfld_;

    void		rockPhysReq(CallBacker*);
    bool		acceptOK(CallBacker*);

};

#endif
