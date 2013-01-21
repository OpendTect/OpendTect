#ifndef uiiosel_h
#define uiiosel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "bufstringset.h"
class IOPar;
class uiLabel;
class uiCheckBox;
class uiComboBox;
class uiPushButton;


/*!
\ingroup uiTools
\brief UI element for selection of data objects
*/

mExpClass(uiTools) uiIOSelect : public uiGroup
{
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup( const char* seltext=0 )
			    : seltxt_(seltext)
			    , withclear_(false)
			    , buttontxt_("&Select")
			    , optional_(false)
			    , keepmytxt_(false)		{}

	mDefSetupMemb(BufferString,seltxt)
	mDefSetupMemb(BufferString,buttontxt)
	mDefSetupMemb(bool,withclear)
	mDefSetupMemb(bool,optional)
	mDefSetupMemb(bool,keepmytxt)
    };

			uiIOSelect(uiParent*,const Setup&,const CallBack&);
			~uiIOSelect();

    bool		isEmpty() const;
    const char*		getInput() const;
    const char*		getKey() const;
    void		setInput(const char* key);
			//!< Will fetch user name using userNameFromKey
    void		setInputText(const char*);
    			//!< As if user typed it manually

    int			nrItems() const;
    int			getCurrentItem() const;
    void		setCurrentItem(int);
    const char*		getItem(int) const;
    bool		isChecked() const; //!< only useful when optional_
    void		setChecked(bool yn); //!< does something when optional_

    void		addSpecialItem(const char* key,const char* value=0);
			//!< If value is null, add value same as key

    virtual void	updateHistory(IOPar&) const;
    virtual void	getHistory(const IOPar&);
    void		addToHistory(const char*);
    void		addToHistory(const BufferStringSet&);

    void		clear()			{ setCurrentItem( 0 ); }
    void		setEmpty(bool withclear=false);
    virtual void	processInput()		{}
    void		setReadOnly(bool readonly=true);

    void		doSel(CallBacker*);
    			//!< Called by Select button push.
    			//!< Make sure selok_ is true if that is the case!

    Notifier<uiIOSelect> selectionDone;
    Notifier<uiIOSelect> optionalChecked;

    const char*		labelText() const;
    void		setLabelText(const char*);
    void		setLabelSelectable(bool yn=true);

    void		stretchHor(bool);

    uiComboBox*		inpBox()		{ return inp_; }

protected:

    CallBack		doselcb_;
    BufferStringSet	entries_;
    IOPar&		specialitems;
    bool		selok_;
    bool		keepmytxt_;

    uiComboBox*		inp_;
    uiPushButton*	selbut_;
    uiLabel*		lbl_;
    uiCheckBox*		optbox_;

    void		optCheck(CallBacker*);
    void		selDone(CallBacker*);
			//!< Subclass must call it - base class can't
			//!< determine whether a selection was successful.

    virtual const char*	userNameFromKey( const char* s ) const	{ return s; }
			//!< If 0 returned, then if possible,
			//!< that entry is not entered in the entries_.

    void		checkState() const;
    void		updateFromEntries();
    bool		haveEntry(const char*) const;

    virtual void	objSel()		{}
			//!< notification when user selects from combo

    void		doFinalise(CallBacker*);

};


mExpClass(uiTools) uiIOFileSelect : public uiIOSelect
{
public:
			uiIOFileSelect(uiParent*,const char* txt,
					bool for_read,
					const char* inp=0,
					bool withclear=false);

    void		setFilter( const char* f )	{ filter = f; }
    void		selectDirectory( bool yn=true )	{ seldir = yn; }

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

			// Some standard types of files
    static IOPar&	ixtablehistory();
    static IOPar&	devicehistory();
    static IOPar&	tmpstoragehistory();

protected:

    void		doFileSel(CallBacker*);
    bool		forread;
    BufferString	filter;
    bool		seldir;

};

#endif

