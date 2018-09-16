#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2001
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "uistrings.h"
class uiLabel;
class uiButton;
class uiCheckBox;
class uiComboBox;


/*!
\ingroup uiTools
\brief UI element for selection of data objects
*/

mExpClass(uiTools) uiIOSelect : public uiGroup
{ mODTextTranslationClass(uiIOSelect);
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup(const uiString& seltext=uiString::empty())
			    : seltxt_(seltext)
			    , withclear_(false)
			    , buttontxt_(uiStrings::sSelect())
			    , compact_(false)
			    , optional_(false)
			    , keepmytxt_(false)
			    , szpol_(uiObject::Medium)
			    , optionsselectable_(true)		{}

	mDefSetupMemb(uiString,seltxt)
	mDefSetupMemb(uiString,buttontxt)
	mDefSetupMemb(bool,withclear)
	mDefSetupMemb(bool,compact)
	mDefSetupMemb(bool,optional)
	mDefSetupMemb(bool,keepmytxt)
	mDefSetupMemb(uiObject::SzPolicy,szpol)
	mDefSetupMemb(bool,optionsselectable)
    };

			uiIOSelect(uiParent*,const Setup&,const CallBack&);
			~uiIOSelect();

    virtual bool	forRead() const		    { return true; }
    bool		isEmpty() const;
    const char*		getInput() const;
    const char*		getKey() const;
    void		setInput(const char* key);
			//!< Will fetch user name using userNameFromKey
    void		setInputText(const char*);
			//!< As if user typed it manually
    void		setEntries(const BufferStringSet& keys,
				   const BufferStringSet& names);

    int			nrItems() const;
    int			getCurrentItem() const;
    void		setCurrentItem(int);
    const char*		getItem(int) const;
    bool		isChecked() const; //!< only useful when optional_
    void		setChecked(bool yn); //!< does something when optional_

    virtual void	updateHistory(IOPar&) const;
    virtual void	getHistory(const IOPar&);
    void		addToHistory(const char*);
    void		addToHistory(const BufferStringSet&);

    virtual void	setEmpty();
    virtual void	processInput()		{}
    void		setReadOnly(bool readonly=true);

    void		addButton(uiButton*,bool insbut); //!< before finalise
    void		doSel(CallBacker*);
			//!< Called by Select button push.
			//!< Make sure selok_ is true if that is the case!

    Notifier<uiIOSelect> selectionDone;
    Notifier<uiIOSelect> optionalChecked;

    const uiString&	labelText() const;
    void		setLabelText(const uiString&);
    void		setLabelSelectable(bool yn=true);

    void		setHSzPol(uiObject::SzPolicy);

    uiComboBox*		inpBox()		{ return inp_; }
    virtual uiObject*	endObj(bool left);

protected:

    BufferStringSet	entries_;
    bool		haveempty_;
    bool		selok_;
    bool		keepmytxt_;
    CallBack		doselcb_;
    ObjectSet<uiButton>	insbuts_;
    ObjectSet<uiButton>	extbuts_;

    uiComboBox*		inp_;
    uiButton*		selbut_;
    uiLabel*		lbl_;
    uiCheckBox*		optbox_;

    void		optCheck(CallBacker*);
    void		selDone(CallBacker*);
			//!< Subclass must call it - base class can't
			//!< determine whether a selection was successful.

    virtual const char*	userNameFromKey( const char* s ) const	{ return s; }
			//!< If 0 returned, then if possible,
			//!< that entry is not entered in the entries_.

    int			nrSpec() const;
    void		checkState() const;
    void		updateFromEntries();
    bool		haveEntry(const char*) const;

    virtual void	objSel()		{}
			//!< notification when user selects from combo

    void		doFinalise(CallBacker*);
};
