#ifndef uidpsaddcolumndlg_h
#define uidpsaddcolumndlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          July 2011
 RCS:           $Id$: 
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class uiGenInput;
class uiPushButton;
class uiTable;

class MathExpression;

mClass uiDPSAddColumnDlg : public uiDialog
{
public:
    				uiDPSAddColumnDlg(uiParent*,bool withmathop);
    void			setColInfos(const BufferStringSet& colnms,
	    				    const TypeSet<int>& colids);
    void 			checkMathExpr(CallBacker*);
    void			parsePush(CallBacker*);
    void			updateDisplay();
    const char*			newAttribName() const;
    bool			acceptOK(CallBacker*);

    MathExpression*		mathObject()		{ return mathobj_; }
    const TypeSet<int>&		usedColIDs() const	{ return usedcolids_; }
 
protected:
    BufferStringSet		colnames_;
    TypeSet<int>		colids_;
    TypeSet<int>		usedcolids_;

    BufferString		mathexprstring_;
    MathExpression*		mathobj_;
    uiGenInput*			nmfld_;
    uiGenInput*			inpfld_;
    uiPushButton*		setbut_;
    uiTable*			vartable_;
    bool			withmathop_;
};

#endif
