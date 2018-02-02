#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2011
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uigroup.h"
#include "bufstringset.h"

class uiListBox;
class uiToolButton;

/*!\brief Base class for element allowing the building of lists of items
          from available 'ingredients'.

 Let's say you want to make a list of persons. These can be male or female, and
 dep on the gender you want to be able to define different properties.

 uiBuildListFromList::Setup( true, "person" )
	.withtitles(true).avtitle("gender");

 The list of 'available' items is simply "male" and "female".
 Once you create e.g. a female, the option 'female' itself should stay in the
 available list. Thus singleuse must remain false. The order of the defined
 items is not important so we choose 'movable' = true.
 The constructor will now generate default tooltips and titles,
 like "Add person".

 Required:
 * Both sets of names cannot have double entries.
 * You only get requests for edit and remove. You have to manage
   underlying objects yourself. Moreover, you have to use functions like
   'removeItem' and 'handleSuccessfullEdit' to keep the display in sync with
   the actual set that is being built.
 * You must define the avFromDef() function. It should return the 'available'
   item that belongs to a 'defined' item name.

  */

mExpClass(uiTools) uiBuildListFromList : public uiGroup
{ mODTextTranslationClass(uiBuildListFromList);
public:

    mExpClass(uiTools) Setup
    { mODTextTranslationClass(uiBuildListFromList::Setup);
    public:
			Setup(bool itemsmovable,const uiString& avitmtyp,
				const uiString& defitmtyp);

	mDefSetupMemb(bool,movable);
	mDefSetupMemb(bool,withio);		// default: true
	mDefSetupMemb(bool,withtitles);		// default: false
	mDefSetupMemb(bool,singleuse);		// default: false
	mDefSetupMemb(uiString,avitemtype);
	mDefSetupMemb(uiString,defitemtype);
	mDefSetupMemb(uiString,avtitle);  // titles
	mDefSetupMemb(uiString,deftitle);
	mDefSetupMemb(uiString,addtt);	// tooltips
	mDefSetupMemb(uiString,edtt);
	mDefSetupMemb(uiString,rmtt);

    };

			uiBuildListFromList(uiParent*,const Setup&,
					    const char* grpnm=0);
    bool		haveUserChange() const		{ return usrchg_; }

protected:

    Setup		setup_;
    bool		usrchg_;
    mutable uiString	avret_;

    uiListBox*		avfld_;
    uiListBox*		deffld_;
    uiToolButton*	edbut_;
    uiToolButton*	rmbut_;
    uiToolButton*	savebut_;
    uiToolButton*	moveupbut_;
    uiToolButton*	movedownbut_;

    uiToolButton*	lowestStdBut();
    const uiString*	curAvSel() const;		//!< null = no selection
    const char*		curDefSel() const;		//!< null = no selection
    void		setCurDefSel(const char*);	//!< null = first

    virtual void	editReq(bool isadd)		= 0;
    virtual void	removeReq()			= 0;
    virtual uiString	avFromDef(const char*) const	= 0;
    virtual bool	ioReq( bool forsave )		{ return false; }
    virtual void	itemSwitch(int, int)		{}
    virtual void	defSelChg();

    void		setAvailable(const BufferStringSet&); //!< at start
    void		setAvailable(const uiStringSet&); //!< at start
    void		removeItem();
    void		removeAll();
    void		setItemName(const char*);
    void		addItem(const char*);
    void		handleSuccessfullEdit( bool isadd, const char* itmnm )
			{ isadd ? addItem( itmnm ) : setItemName( itmnm ); }

    void		defSelCB( CallBacker* )		{ defSelChg(); }
    void		addCB( CallBacker* )		{ editReq( true ); }
    void		edCB( CallBacker* )		{ editReq( false ); }
    void		rmCB( CallBacker* )		{ removeReq(); }
    void		openCB(CallBacker*);
    void		saveCB(CallBacker*);
    void		moveCB(CallBacker*);

    void		rmItm(int,bool);

};
