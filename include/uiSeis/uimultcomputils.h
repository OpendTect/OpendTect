#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          August 2008
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "bufstringset.h"
#include "uicompoundparsel.h"
#include "uidialog.h"
#include "uistring.h"

class LineKey;
class uiGenInput;
class uiListBox;


/*!\brief dialog to select (multiple) component(s) of stored data */

mExpClass(uiSeis) uiMultCompDlg : public uiDialog
{ mODTextTranslationClass(uiMultCompDlg);
public:
			uiMultCompDlg(uiParent*,const BufferStringSet&);

    void		getCompNrs(TypeSet<int>&) const;
    const char*		getCompName(int) const;

protected:

    uiListBox*		compfld_;
};


/*!\brief CompoundParSel to capture and sum up the user-selected components */

mExpClass(uiSeis) uiMultCompSel : public uiCompoundParSel
{ mODTextTranslationClass(uiMultCompSel);
    public:
			uiMultCompSel(uiParent*);
			~uiMultCompSel();

    void		setUpList(const MultiID&);
    void		setUpList(const BufferStringSet&);
    bool		allowChoice() const	{ return compnms_.size()>1; }

    protected:

    BufferString	getSummary() const override;
    void		doDlg(CallBacker*);
    void		prepareDlg();

    mExpClass(uiSeis) MCompDlg : public uiDialog
    { mODTextTranslationClass(MCompDlg);
	public:
				MCompDlg(uiParent*,const BufferStringSet&);

	void			selChg(CallBacker*);
	uiListBox*		outlistfld_;
	uiGenInput*		useallfld_;
    };

    BufferStringSet	compnms_;
    MCompDlg*		dlg_;
};


