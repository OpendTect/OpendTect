#ifndef uimathpropeddlg_h
#define uimathpropeddlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          Nov 2013
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "mathproperty.h"
#include "propertyref.h"
#include "uidialog.h"

class MathExpression;
class uiMathExpression;
class uiMathExpressionVariable;
class uiComboBox;
class uiListBox;
class uiPushButton;

mExpClass(uiTools) uiMathPropEdDlg : public uiDialog
{
public:
			uiMathPropEdDlg(uiParent*,MathProperty&,
					const PropertyRefSelection&);

			~uiMathPropEdDlg();

    void		insProp(CallBacker*);
    void		rockPhysReq(CallBacker*);
    void		updVarsOnScreen(CallBacker* cb=0);
    void		updateMathExpr();
    bool		acceptOK(CallBacker*);
    void		replPushed(CallBacker*);
    void		replaceInputsInFormula();
    BufferString	formulaStr() const;

protected:

    MathProperty&       prop_;                                                  
    uiMathExpression*   formfld_;                                               
    uiListBox*          propfld_;                                               
    MathExpression*     expr_;                                                  
    BufferStringSet     inputunits_;                                            
    int                 nrvars_;                                                
    uiPushButton*       replbut_;                                               
    uiComboBox*         outunfld_;
    ObjectSet<uiMathExpressionVariable> inpdataflds_;                           
};

#endif
