#ifndef uimultcomputils_h
#define uimultcomputils_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          August 2008
 RCS:           $Id: uimultcomputils.h,v 1.9 2012-08-03 13:01:06 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "bufstringset.h"
#include "uicompoundparsel.h"
#include "uidialog.h"

class LineKey;
class uiGenInput;
class uiLabeledListBox;
class uiListBox;


/*!\brief dialog to select (multiple) component(s) of stored data */

mClass(uiSeis) uiMultCompDlg : public uiDialog
{
public:
			uiMultCompDlg(uiParent*,const BufferStringSet&);

    void		getCompNrs(TypeSet<int>&) const;
    const char*		getCompName(int) const;

protected:

    uiListBox*		compfld_;
};


/*!\brief CompoundParSel to capture and sum up the user-selected components */

mClass(uiSeis) uiMultCompSel : public uiCompoundParSel
{
    public:
			uiMultCompSel(uiParent*);
			~uiMultCompSel();

    void		setUpList(LineKey);
    void		setUpList(const BufferStringSet&);
    bool		allowChoice() const	{ return compnms_.size()>1; }

    protected:
	
    BufferString        getSummary() const;
    void                doDlg(CallBacker*);
    void		prepareDlg();

    mClass(uiSeis) MCompDlg : public uiDialog
    {
	public:
	    			MCompDlg(uiParent*,const BufferStringSet&);

	void			selChg(CallBacker*);
	uiLabeledListBox*	outlistfld_;
	uiGenInput*		useallfld_;
    };

    BufferStringSet	compnms_;
    MCompDlg*		dlg_;
};


#endif

