#ifndef menuhandler_h
#define menuhandler_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id: menuhandler.h,v 1.1 2005-07-07 21:45:26 cvskris Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"
#include "callback.h"

class BufferStringSet;
class MenuItem;
class MenuHandler;

class MenuItemHolder : public CallBackClass
{
public:
    				MenuItemHolder();
    virtual			~MenuItemHolder();

    virtual void		addItem( MenuItem*, bool manage=false );
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

    const MenuItem*		getItem( int idx ) const;
    MenuItem*			getItem( int idx );
    int				itemIndex( const MenuItem* ) const;
    int				itemIndex( int id ) const;
    MenuItem*			findItem( int id );
    const MenuItem*		findItem( int id ) const;
    MenuItem*			findItem(const char*);
    const MenuItem*		findItem(const char*) const;

    const ObjectSet<MenuItem>&	getItems() const;

protected:
    friend			class MenuHandler;
    virtual void		assignItemID( MenuItem& );
    				/*!< Get a unique id for this item. */

    ObjectSet<MenuItem>		items;
    BoolTypeSet			manageitems;
    MenuItemHolder*		parent;
};

/*!A generic representation of an item in a menu. */

class MenuItem : public MenuItemHolder
{
public:
    				MenuItem( const char* text=0, int placement=-1,
					  const CallBack& cb=CallBack(0,0) );

    void			createItems( const BufferStringSet& );

    BufferString		text;
    				/*< The text that should be on the item. */
    int				placement;
    				/*!< Gives the system an indication where in the
				     menu the item should be placed. Items will
				     be placed in increasing order of placement.				*/
    CallBack			cb;
    				/*< is called if item is selected in the menu.*/

    int				id;
    				/*!< This item's unique id. */
    bool			checked;
    				/*!< If true, a check-mark will be put infront
				    of the items text */
    bool			enabled;
    				/*!< If false, the item will be visble, but
				    not selectable. */
};


/*! A generic representation of a menu. It allows anyone to add their own
    custom menuitems to it. The principle is that the menu triggers
    it's createnotifier just before the menu should be displayed, and
    the application adds the items it wants into the menu. When the user
    has clicked on something in the menu, the handlenotifier is triggered and
    the application checks what should be done.

Usage:

    \code
    menu->createnotifier.notify( mCB( this, myclassname, createMenuCB ));
    menu->handlenotifier.notify( mCB( this, myclassname, handleMenuCB ));
    \endcode

    Upon a create notification, your class might do something like this:
    \code
    void myclass::createMenuCB( CallBacker* callback )
    {
	uiMenuHandler* menu = dynamic_cast<uiMenuHandler*>(callback);

	mAddMenuItem( menu, &mymenuitem, true, false );
	mAddMenuItem( menu, &mysubmenu, true, false );
	mAddMenuItem( &mysubmenu, &mysubmenuitem1, true, false );
	mAddMenuItem( &mysubmenu, &mysubmenuitem2, true, false );
    }
    \endcode

The code will make a menu with two items, and the second item will have a
submenu with two items. The first boolean says wether the item should be
enabled, the second one says where there should be a check before it.

    Upon a handle notification, your class might do something like this:
    \code
    void myclass::handleMenuCB(CallBacker* callback )
    {
	mCBCapsuleUnpackWithCaller( int, mnuid, caller, callback );
	uiMenuHandler* menu = dynamic_cast<uiMenuHandler*>(caller);
	if ( mnuid==-1 || menu->isHandled() )
	    return;

	if ( mnuid==mymenuitem.id )
	{
	    menu->setIsHandled(true);
	    do_something();
	}
	else if ( mnuid==mysubmenuitem1.id )
	{
	    menu->setIsHandled(true);
	    do_something_else();
	}
	else if ( mnuid==mymenusubitem2.id )
	{
	    menu->setIsHandled(true);
	    do_something_else();
	}
    }
    \endcode
*/
    
class MenuHandler : public MenuItemHolder
{
    				mRefCountImpl(MenuHandler);
public:
    				MenuHandler( int id );

    int				menuID() const { return id_; }
    				/*<Each menu has an unique id where the
				   that identifies it. */
    void			setMenuID( int newid ) { id_=newid; }

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

    static const int		fromTree;
    static const int		fromScene;

protected:
    void			assignItemID( MenuItem& );

    int				freeid;
    int				id_;
    bool			ishandled;
};

#define mResetMenuItem( item ) \
{ \
    (item)->enabled = true; \
    (item)->checked = false; \
    (item)->id = -1; \
}

#define mAddMenuItemWithManageFlag( parent, item, manage, enab, check ) \
{ \
    MenuItem* _item = item; \
    MenuItemHolder* _parent = parent; \
    if ( (_parent)->itemIndex(_item)==-1 ) \
	(_parent)->addItem( _item ); \
   \
    (_item)->enabled = (enab); \
    (_item)->checked = (check); \
}

#define mAddMenuItem( parent, item, enab, check ) \
mAddMenuItemWithManageFlag( parent, item, false, enab, check )


#define mAddManagedMenuItem( parent, item, enab, check ) \
mAddMenuItemWithManageFlag( parent, item, true, enab, check )


#define mAddMenuItemCond( menu, item, enab, check, cond ) \
    if ( cond ) \
	mAddMenuItem( menu, item, enab, check ) \
    else \
	mResetMenuItem( item ) \


#endif
