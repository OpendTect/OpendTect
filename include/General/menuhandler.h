#ifndef menuhandler_h
#define menuhandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "refcount.h"
#include "position.h"
#include "callback.h"

class BufferStringSet;
class MenuItem;
class MenuHandler;

mExpClass(General) MenuItemHolder : public CallBacker
{
public:
    				MenuItemHolder();
    virtual			~MenuItemHolder();
    Notifier<MenuItemHolder>	removal;
    				/*!< triggers when class is deleted */

    virtual void		addItem(MenuItem*,bool manage=false);
    				/*!<\param manage specified wether the class
				  will delete the item when it's not
				  needed any longer. Mostly used
				  when doing: \code
				  item->addItem(new MenuItem("Menu text"),true);
				  \endcode.
				
				  Each item will get a unique id, that is
				  the mechanism to see which item was
				  selected in the menu. */
    void			removeItems();
    int				nrItems() const;

    const MenuItem*		getItem(int idx) const;
    MenuItem*			getItem(int idx);
    int				itemIndex(const MenuItem*) const;
    int				itemIndex(int id) const;
    MenuItem*			findItem(int id);
    const MenuItem*		findItem(int id) const;
    MenuItem*			findItem(const char*);
    const MenuItem*		findItem(const char*) const;

    const ObjectSet<MenuItem>&	getItems() const;

protected:
    friend			class MenuHandler;
    void			itemIsDeletedCB(CallBacker*);
    virtual void		assignItemID(MenuItem&);
    				/*!< Get a unique id for this item. */

    MenuItemHolder*		parent_;

private:
    ObjectSet<MenuItem>		items_;
    BoolTypeSet			manageitems_;
};

/*!A generic representation of an item in a menu. */

mExpClass(General) MenuItem : public MenuItemHolder
{
public:
    				MenuItem(const char* text=0,int placement=-1);
    void			createItems(const BufferStringSet&);

    BufferString		text;
    				/*< The text that should be on the item. */
    int				placement;
    				/*!< Gives the system an indication where in the
				     menu the item should be placed. Items will
				     be placed in increasing order of placement.				*/
    int				id;
    				/*!< This item's unique id. */
    bool			checkable;
    				/*!< If true, a check-mark can be placed */
    bool			checked;
    				/*!< If true, a check-mark will be put infront
				    of the items text */
    bool			enabled;
    				/*!< If false, the item will be visble, but
				    not selectable. */
    BufferString		iconfnm;
    				//*!< Filename of icon
    BufferString		tooltip;
    				//*!< Tooltip if item is used in toolbar
};


mExpClass(General) SeparatorItem : public MenuItem
{
public:
				SeparatorItem(int plmnt=-1)
				    : MenuItem("Separator",plmnt)	{}
};


/*! A generic representation of a menu. It allows anyone to add their own
    custom menuitems to it. The principle is that the menu triggers
    it's createnotifier just before the menu should be displayed, and
    the application adds the items it wants into the menu. When the user
    has clicked on something in the menu, the handlenotifier is triggered and
    the application checks what should be done.

Usage:

    \code
    menu->createnotifier.notify( mCB(this,myclass,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,myclass,handleMenuCB) );
    \endcode

    Upon a create notification, your class might do something like this:
    \code
    void myclass::createMenuCB( CallBacker* callback )
    {
        mDynamicCastGet( MenuHandler*, menu, callback );

	mAddMenuItem( menu, &mymenuitem, true, false );
	mAddMenuItem( menu, &mysubmenu, true, false );
	mAddMenuItem( &mysubmenu, &mysubmenuitem1, true, false );
	mAddMenuItem( &mysubmenu, &mysubmenuitem2, true, false );
    }
    \endcode

The code will make a menu with two items, and the second item will have a
submenu with two items. The first boolean says whether the item should be
enabled, the second one says where there should be a check before it.

The menuitems are instantiations of MenuItem and should be stored in your
class. They hold information about the item itself (like text, enabled or not
enabled, checked or not checked, information on where in the menu it should
be placed. In addition, it has an unique id that is set when the item is
inserted into the menu.

    Upon a handle notification, your class might do something like this:
    \code
    void myclass::handleMenuCB(CallBacker* callback )
    {
	mCBCapsuleUnpackWithCaller( int, mnuid, caller, callback );
        mDynamicCastGet( MenuHandler*, menu, caller );
	if ( mnuid==-1 || menu->isHandled() )
	    return;

	bool ishandled = true;
	if ( mnuid==mymenuitem.id )
	    do_something();
	else if ( mnuid==mysubmenuitem1.id )
	    do_something_else();
	else if ( mnuid==mymenusubitem2.id )
	    do_something_else();
	else
	    ishandled = false;

	menu->setIsHandled(ishandled);
    }
    \endcode
*/
    
mExpClass(General) MenuHandler : public MenuItemHolder
{				mRefCountImpl(MenuHandler);
public:
    				MenuHandler( int id );

    virtual bool		executeMenu()				= 0;

    int				menuID() const { return id_; }
    				/*<Each menu has an unique id where the
				   that identifies it. */
    void			setMenuID( int newid ) { id_=newid; }

    Notifier<MenuHandler>	initnotifier;
    Notifier<MenuHandler>	createnotifier;
    CNotifier<MenuHandler,int>	handlenotifier;
    bool			isHandled() const;
    				/*!< Should be called as the first thing
				     from callbacks that is triggered from
				     uiMenuHandler::handlenotifier. If
				     isHandled() returns true, the callback
				     should return immediately. */
    void			setIsHandled(bool);
    				/*!<Should be called from callbacks that
				    are triggered from
				    uiMenuHandler::handlenotifier
				    if they have found the menu id they are
				    looking for.  */

    int				queueID() const { return queueid_; }
    				/*!<After a menu is executed, it will
				    execute a queue, identified by this id. */

protected:
    void			assignItemID(MenuItem&);
    void			executeQueue();

    int				id_;
    bool			ishandled_;
    int				queueid_;
};


/*!  handles the MenuItem insertion automaticly. If the menu is selected,
     a callback is triggered. Default behaviour is that the menu item is added
     every time the menu is built, and that it's enabled but not checked.
     That can be changes by setting the doadd_, isenabled_ and ischecked_ or
     by an inheriting object in the shouldAddMenu(), shouldBeEnabled() and
     shouldBeChecked() functions. */

mExpClass(General) MenuItemHandler : public CallBacker
{
public:
			MenuItemHandler(MenuHandler&,const char* nm,
					const CallBack&,const char* parenttxt=0,
					int placement=-1);
			~MenuItemHandler();
    bool		doadd_;
    			/*!<Item is added if true AND shouldAddMenu() retuns
			    true. Default is true. */
    bool		isenabled_;
    			/*!<Item is enabled if true AND shouldBeEnabled() retuns
			    true. Default is true. */
    bool		ischecked_;
    			/*!<Item is checked if true OR shouldBeChecked() retuns
			    true. Default is false. */

    void		setIcon(const char* fnm);
    
protected:

    virtual void	createMenuCB(CallBacker*);	
    virtual void	handleMenuCB(CallBacker*);	

    virtual bool	shouldAddMenu() const			{ return true; }
    virtual bool	shouldBeEnabled() const			{ return true;}
    virtual bool	shouldBeChecked() const			{ return false;}

    MenuItem		menuitem_;
    MenuHandler&	menuhandler_;
    CallBack		cb_;
    BufferString	parenttext_;
};



#define mResetMenuItem( item ) \
{ \
    (item)->enabled = true; \
    (item)->checked = false; \
    (item)->id = -1; \
    (item)->removeItems(); \
}

#define mAddMenuItemWithManageFlag( parent, item, manage, enab, check ) \
{ \
    MenuItem* _item = item; \
    MenuItemHolder* _parent = parent; \
    if ( _parent && (_parent)->itemIndex(_item)==-1 ) \
	(_parent)->addItem( _item ); \
   \
    (_item)->enabled = (enab); \
    (_item)->checked = (check); \
}

#define mAddMenuItem( parent, item, enab, check ) \
mAddMenuItemWithManageFlag( parent, item, false, enab, check )


#define mAddManagedMenuItem( parent, item, enab, check ) \
mAddMenuItemWithManageFlag( parent, item, true, enab, check )


#define mAddMenuItemCond( menu, item, enab, check, cond ) { \
    if ( menu && cond ) \
	mAddMenuItem( menu, item, enab, check ) \
    else \
	mResetMenuItem( item ) } \

//Macro that can poplulate both a toolbar and a menu. 
#define mAddMenuOrTBItem( istoolbar, tbparent, popupparent, item, enab, check )\
    mAddMenuItem( \
	istoolbar?(MenuItemHolder*)(tbparent):(MenuItemHolder*)(popupparent), \
	    item, enab, check )

#endif

