#ifndef uimultcomputils_h
#define uimultcomputils_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          August 2008
 RCS:           $Id: uimultcomputils.h,v 1.5 2009-03-18 11:04:03 cvshelene Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "uicompoundparsel.h"
#include "uidialog.h"

class LineKey;
class uiGenInput;
class uiLabeledListBox;
class uiListBox;


/*!\brief dialog to select a component of stored data,
  will not require display if the number of components available is equal to 1*/

mClass uiMultCompDlg : public uiDialog
{
public:
			uiMultCompDlg(uiParent*,LineKey);

    int			getCompNr() const;
    bool		needDisplay() const		{ return needdisplay_; }

protected:

    uiListBox*		compfld_;
    bool		needdisplay_;
};


/*!\brief CompoundParSel to capture and sum up the user-selected components */

mClass uiMultCompSel : public uiCompoundParSel
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

    mClass MCompDlg : public uiDialog
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
