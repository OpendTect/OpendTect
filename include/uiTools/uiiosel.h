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

mClass(uiTools) uiIOSelect : public uiGroup
{
public:

    mClass(uiTools) Setup
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


mClass(uiTools) uiIOFileSelect : public uiIOSelect
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


/*!
\defgroup uiTools uiTools
User Interface Tools

  The uiTools module is a collection of tools that are based on the uiBase
  module (and thus on Qt services) but that combine uiBase services without
  accessing Qt at all. Throughout OpnedTect, Qt is not directly accessed
  anywhere else than in uiBase, not from uiTools either.

  Some commonly used classes:
<ul>
 <li>uiGenInput is the most common user interface element we use. It represents a generalized input field for strings, numbers, bools, positions and more. The idea is to tell the class the characteristics of the data you need to input. uiGenInput will then select the best user interfac element for that type of data.
 <li>uiTaskRunner executes the work encapsulated in any Task object, like Executor's. An Executor object holds an amount of work that is probably too big to be finished instantly, hence the user must be able to see the progress and must be able to stop it.
 <li>uiFileBrowser shows the contents of text files. If editable, the user can edit the text and save it.
</ul>

*/


#endif

