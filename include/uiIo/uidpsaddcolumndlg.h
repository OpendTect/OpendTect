#pragma once

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          July 2011
 RCS:           $Id$:
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "bufstringset.h"

class uiGenInput;
class uiPushButton;
class uiTable;
namespace Math { class Expression; }


/*!
\brief Dialog box to add columns in datapointset.
*/

mExpClass(uiIo) uiDPSAddColumnDlg : public uiDialog
{ mODTextTranslationClass(uiDPSAddColumnDlg);
public:
				uiDPSAddColumnDlg(uiParent*,bool withmathop);
    void			setColInfos(const BufferStringSet& colnms,
					    const TypeSet<int>& colids);
    void			checkMathExpr(CallBacker*);
    void			parsePush(CallBacker*);
    void			updateDisplay();
    const char*			newAttribName() const;
    bool			acceptOK(CallBacker*);

    Math::Expression*		mathObject()		{ return mathobj_; }
    const TypeSet<int>&		usedColIDs() const	{ return usedcolids_; }

protected:
    BufferStringSet		colnames_;
    TypeSet<int>		colids_;
    TypeSet<int>		usedcolids_;

    BufferString		mathexprstring_;
    Math::Expression*		mathobj_;
    uiGenInput*			nmfld_;
    uiGenInput*			inpfld_;
    uiPushButton*		setbut_;
    uiTable*			vartable_;
    bool			withmathop_;
};

