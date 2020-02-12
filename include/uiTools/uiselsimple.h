#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2001
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uistring.h"
#include "bufstringset.h"

class uiGroup;
class uiListBox;
class uiGenInput;
class uiCheckList;
class uiListBoxFilter;

/*!\brief Select entry from list */

mExpClass(uiTools) uiSelectFromList : public uiDialog
{ mODTextTranslationClass(uiSelectFromList);
public:

    mExpClass(uiTools) Setup : public uiDialog::Setup
    {
    public:
			Setup( const uiString& wintitl,
			       const uiStringSet& its )
			    : uiDialog::Setup(wintitl,mNoDlgTitle,mNoHelpKey)
			    , uiitems_(its)
			    , current_(0)		{}
			Setup( const uiString& wintitl,
			       const BufferStringSet& its )
			    : uiDialog::Setup(wintitl,mNoDlgTitle,mNoHelpKey)
			    , items_(its)
			    , current_(0)		{}

	mDefSetupMemb(int,current);
	uiStringSet	uiitems_;
	BufferStringSet	items_;

    };

			uiSelectFromList(uiParent*,const Setup&);
			~uiSelectFromList();

    int			selection() const	{ return setup_.current_; }
			//!< -1 = no selection made (cancelled or 0 list items)

    uiListBox*		selFld()		{ return selfld_; }
    uiListBoxFilter*	filtFld()		{ return filtfld_; }
    uiObject*		bottomFld(); //!< is selFld()

protected:

    Setup		setup_;

    uiListBox*		selfld_;
    uiListBoxFilter*	filtfld_;

    void		filtChg(CallBacker*);
    virtual bool	acceptOK();

private:

    void		init(const char**,int,const char*);

};


/*!\brief Get a name from user, whilst displaying names that already exist */

mExpClass(uiTools) uiGetObjectName : public uiDialog
{ mODTextTranslationClass(uiGetObjectName);
public:

    mExpClass(uiTools) Setup : public uiDialog::Setup
    {
    public:
			Setup( const uiString& wintitl,
			       const BufferStringSet& its )
			    : uiDialog::Setup(wintitl,mNoDlgTitle,mNoHelpKey)
			    , items_(its)
			    , inptxt_( uiStrings::sName())	{}

	mDefSetupMemb(BufferString,deflt);
	mDefSetupMemb(uiString,inptxt);

	const BufferStringSet	items_;

    };

			uiGetObjectName(uiParent*,const Setup&);
			~uiGetObjectName()	{}

    const char*		text() const;

    uiGenInput*		inpFld()		{ return inpfld_; }
    uiListBox*		selFld()		{ return listfld_; }
    uiGroup*		bottomFld(); //!< is inpFld()

protected:

    uiGenInput*		inpfld_;
    uiListBox*		listfld_;

    void		selChg(CallBacker*);
    virtual bool	acceptOK();

};


/*!\brief Get an action from a series of possibilities from user */

mExpClass(uiTools) uiGetChoice : public uiDialog
{ mODTextTranslationClass(uiGetChoice);
public:

			uiGetChoice(uiParent*,const uiString& question=
				    uiString::empty(),
				    bool allowcancel=true,
				    const HelpKey& helpkey=mNoHelpKey);
			uiGetChoice(uiParent*,
				    const uiStringSet& options,
				    const uiString& question=
				    uiString::empty(),
				    bool allowcancel=true,
				    const HelpKey& helpkey=mNoHelpKey);
			uiGetChoice(uiParent*,uiDialog::Setup,
				    const uiStringSet& options, bool wc);

    void		addChoice(const uiString& txt,const char* iconnm=0);
    void		setDefaultChoice(int);
    int			choice() const		{ return choice_; }
			//!< on cancel will be -1

    uiCheckList*	checkList();
    uiGroup*		bottomFld(); //!< is checkList()

protected:

    uiCheckList*	inpfld_;
    int			choice_;
    const bool		allowcancel_;

    virtual bool	acceptOK();
    virtual bool	rejectOK();

};
