#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiMultCompDlg();

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
				~MCompDlg();

	void			selChg(CallBacker*);
	uiListBox*		outlistfld_;
	uiGenInput*		useallfld_;
    };

    BufferStringSet	compnms_;
    MCompDlg*		dlg_;
};
