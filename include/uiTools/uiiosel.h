#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
{
mODTextTranslationClass(uiIOSelect)
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup(const uiString& seltext=uiString::empty());
	virtual		~Setup();

	mDefSetupMemb(uiString,seltxt)		//!< empty
	mDefSetupMemb(uiString,buttontxt)	//!< Select
	mDefSetupMemb(bool,withclear)		//!< false
	mDefSetupMemb(bool,optional)		//!< false
	mDefSetupMemb(bool,keepmytxt)		//!< false
    };

			uiIOSelect(uiParent*,const Setup&,const CallBack&);
			~uiIOSelect();

			// before finalize:
    void		addExtSelBut(uiButton*);

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
    void		setTextValidator(const BufferString& regex);
    void		avoidTextValidator();

protected:

    BufferStringSet	entries_;
    bool		haveempty_;
    bool		selok_;
    bool		keepmytxt_;
    CallBack		doselcb_;

    uiComboBox*		inp_;
    uiButton*		selbut_		= nullptr;
    ObjectSet<uiButton>	extselbuts_;
    uiLabel*		lbl_		= nullptr;
    uiCheckBox*		optbox_		= nullptr;

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

    void		doFinalize(CallBacker*);
};


mExpClass(uiTools) uiIOFileSelect : public uiIOSelect
{
mODTextTranslationClass(uiIOFileSelect)
public:
			uiIOFileSelect(uiParent*,const uiString& txt,
					bool for_read,
					const char* inp=nullptr,
					bool withclear=false);
			~uiIOFileSelect();

    void		setFilter( const char* f )	{ filter = f; }
    void		selectDirectory( bool yn=true )	{ seldir = yn; }

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

			// Some standard types of files
    static IOPar&	ixtablehistory();
    static IOPar&	tmpstoragehistory();

protected:

    void		doFileSel(CallBacker*);
    bool		forread;
    BufferString	filter;
    bool		seldir;

};
