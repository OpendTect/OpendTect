#ifndef uibuildlistfromlist_h
#define uibuildlistfromlist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2011
 RCS:           $Id: uibuildlistfromlist.h,v 1.2 2011-06-15 09:03:52 cvsbert Exp $
________________________________________________________________________

-*/


#include "uigroup.h"
#include "bufstringset.h"

class uiListBox;
class uiToolButton;

/*!\brief Base class for element allowing the building of lists of items
          from available items.

 Let's say you want to make a list of persons. These can be male or female, and
 dep on the gender you want to be able to define different properties.

 uiBuildListFromList::Setup su( false, "person", true );

 Once you create e.g. a female, the option 'female' itself should stay in the
 'available' list. So, singleuseitems is false. Here, the order of the defined
 items is important so we set 'itemsmovable' to true. The constructor will now
 set default tooltips for add, edit and remove button like "Add person".

 Required:
 * Both sets of names cannot have double entries.
 * You only get requests for add, edit and remove. You have to manage
   underlying objects yourself. Moreover, you have to use 'removeItem' and
   'addItem' to keep the display in sync.

  */

mClass uiBuildListFromList : public uiGroup
{
public:

    mClass Setup
    {
    public:
			Setup(bool singleuseitems,const char* itemtypename,
			      bool itemsmovable=false);

	mDefSetupMemb(bool,singleuse);
	mDefSetupMemb(bool,movable);
	mDefSetupMemb(BufferString,itemtype);
	mDefSetupMemb(BufferString,addtt);
	mDefSetupMemb(BufferString,edtt);
	mDefSetupMemb(BufferString,rmtt);

    };

			uiBuildListFromList(uiParent*,const Setup&,
					    const char* grpnm=0);
    bool		haveUserChange() const		{ return usrchg_; }

protected:

    Setup		setup_;
    bool		usrchg_;

    uiListBox*		avfld_;
    uiListBox*		deffld_;
    uiToolButton*	edbut_;
    uiToolButton*	rmbut_;
    uiToolButton*	moveupbut_;
    uiToolButton*	movedownbut_;

    uiToolButton*	lowestStdBut();
    const char*		curAvSel() const;		//!< null = no selection
    const char*		curDefSel() const;		//!< null = no selection
    void		setCurDefSel(const char*);	//!< null = first

    virtual void	editReq(bool isadd)				= 0;
    virtual void	removeReq()					= 0;
    virtual const char*	avFromDef(const char*) const			= 0;
    virtual void	itemSwitch(const char*,const char*) const	{}

    void		setAvailable(const BufferStringSet&); //!< at start
    void		removeItem(); 
    void		setItemName(const char*); 
    void		addItem(const char*); 
    void		removeAll(); 

    bool		isadd_;

    virtual void	defSelChg(CallBacker*);
    void		addCB( CallBacker* )		{ editReq( true ); }
    void		edCB( CallBacker* )		{ editReq( false ); }
    void		rmCB( CallBacker* )		{ removeReq(); }

    void		rmItm(int,bool);

};


#endif
