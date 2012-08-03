#ifndef uiselsimple_h
#define uiselsimple_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.h,v 1.19 2012-08-03 13:01:15 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class uiListBox;
class uiGenInput;
class uiCheckList;
class BufferStringSet;

/*!\brief Select entry from list */

mClass(uiTools) uiSelectFromList : public uiDialog
{ 	
public:

    mClass(uiTools) Setup : public uiDialog::Setup
    {
    public:
			Setup( const char* wintitl, const BufferStringSet& its )
			: uiDialog::Setup(wintitl,0,mNoHelpID)
			, items_(its)
			, current_(0)		{}

	mDefSetupMemb(int,current);

	const BufferStringSet&	items_;

    };

			uiSelectFromList(uiParent*,const Setup&);
			~uiSelectFromList()	{}

    int			selection() const	{ return setup_.current_; }
    			//!< -1 = no selection made (cancelled or 0 list items)

    uiListBox*		selFld()		{ return selfld_; }
    uiGenInput*		filtFld()		{ return filtfld_; }

protected:

    Setup		setup_;

    uiListBox*		selfld_;
    uiGenInput*		filtfld_;

    void		filtChg(CallBacker*);
    virtual bool	acceptOK(CallBacker*);

private:

    void		init(const char**,int,const char*);

};


/*!\brief Get a name from user, whilst displaying names that already exist */

mClass(uiTools) uiGetObjectName : public uiDialog
{ 	
public:

    mClass(uiTools) Setup : public uiDialog::Setup
    {
    public:
			Setup( const char* wintitl,const BufferStringSet& its )
			: uiDialog::Setup(wintitl,0,mNoHelpID)
			, items_(its)
			, inptxt_("Name")	{}

	mDefSetupMemb(BufferString,deflt);
	mDefSetupMemb(BufferString,inptxt);

	const BufferStringSet&	items_;

    };

			uiGetObjectName(uiParent*,const Setup&);
			~uiGetObjectName()	{}

    const char*		text() const;

    uiGenInput*		inpFld()		{ return inpfld_; }
    			//!< Is the lowest field
    uiListBox*		selFld()		{ return listfld_; }

protected:

    uiGenInput*		inpfld_;
    uiListBox*		listfld_;

    void		selChg(CallBacker*);
    virtual bool	acceptOK(CallBacker*);

};


/*!\brief Get an action from a series of possibilities from user */

mClass(uiTools) uiGetChoice : public uiDialog
{ 	
public:

			uiGetChoice(uiParent*,
				    const BufferStringSet& options,
				    const char* question=0,
				    bool allowcancel=true,
				    const char* helpid=mNoHelpID);

			uiGetChoice(uiParent*,uiDialog::Setup,
				    const BufferStringSet& options, bool wc);

    void		setDefaultChoice(int);
    int			choice() const		{ return choice_; }
    			//!< on cancel will be -1

protected:

    uiCheckList*	inpfld_;
    int			choice_;
    const bool		allowcancel_;

    virtual bool	acceptOK(CallBacker*);
    virtual bool	rejectOK(CallBacker*);

};


#endif

