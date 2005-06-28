#ifndef uimenuhandler_h
#define uimenuhandler_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id: uimenuhandler.h,v 1.1 2005-06-28 15:59:10 cvskris Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "refcount.h"
#include "position.h"

class uiPopupMenu;
class uiMenuItem;

/*! An extensibe menu for visual objects. Every instance that is interested in
    putting in items in the menu should hook up to the
    uiMenuHandler::createnotifier and uiMenuHandler::handlenotifier.
    \code
    uiVisPartServer* visserv = getItFromSomeWhere();
    uiMenuHandler* menu = visser->getMenu(id);
    menu->createnotifier.notify( mCB( this, myclassname, createMenuCB ));
    menu->handlenotifier.notify( mCB( this, myclassname, handleMenuCB ));
    \endcode

    Upon a create notification, your class might do something like this:
    \code
    void myclass::createMenuCB(CallBacker* callback )
    {
	uiMenuHandler* menu = dynamic_cast<uiMenuHandler*>(callback);
	myitemid = menu->addItem( new uiMenuItem("My itemtext") );

	uiPopUpMenu* mymenu = new uiPopupMenu(menu->getParent(), "My menu",-1);
	mysubitem1id = menu->getFreeID();
	mymenu->insertItem( new uiMenuItem("My subitem 1", mysubitem1id) );
	mysubitem2id = menu->getFreeID();
	mymenu->insertItem( new uiMenuItem("My subitem 2", mysubitem2id) );
	menu->addItem( mymenu );
    }
    \endcode

    Upon a handle notification, your class might do something like this:
    \code
    void myclass::handleMenuCB(CallBacker* callback )
    {
	mCBCapsuleUnpackWithCaller( int, mnuid, caller, callback );
	uiMenuHandler* menu = dynamic_cast<uiMenuHandler*>(caller);
	if ( mnuid==-1 || menu->isHandled() )
	    return;

	if ( mnuid==myitemid )
	{
	    menu->setIsHandled(true);
	    do_something();
	}
	else if ( mnuid==mysubitem1id )
	{
	    menu->setIsHandled(true);
	    do_something_else();
	}
	else if ( mnuid==mysubitem2id )
	{
	    menu->setIsHandled(true);
	    do_something_else();
	}
    }
    \endcode
*/


class uiMenuHandler : public CallBackClass
{
    				mRefCountImpl(uiMenuHandler);
public:
    				uiMenuHandler( uiParent*, int id );

    int				id() const { return id_; }
    void			setID( int newid ) { id_=newid; }
    uiParent*			getParent() const { return parent; }

    bool			executeMenu(int menutype,
	    				    const TypeSet<int>* path=0 );
    				/*!<\param menutype is an integer that specifies
				   	   what type of menu should be
					   generated. Two numbers are reserved,
					   and the user of the class may use his
					   own codes for other circumstances.
					   The two defined values are:
					   - menutype==fromTree  menu generated
					   	from (a right-click on) the
						treeitem.
					   - menutype==fromScene menu generated
					   	from the scene.
				    \param path If menutype==fromScene the path
				     	   of selection (i.e. a list of the
					   ids of the paht, from scene to picked
					   object).

				*/
    int				getMenuType() const { return menutype; }
    				/*!<\returns the \a menutype specified in
					  uiMenuHandler::executeMenu.
				    \note does only give a valid
				          answer if called from a callback,
					  notified by
					  uiMenuHandler::createnotifier
					  or uiMenuHandler::handlenotifier.  */
    const TypeSet<int>*		getPath() const { return path; }
    				/*!<\returns The path of selection (i.e. a list
				  	   of the ids of the paht, from scene
					   to picked object). */
    const Coord3&		getPickedPos() const { return positionxyz; }
    void			setPickedPos(const Coord3& pickedpos)
					{ positionxyz=pickedpos; }
    

    Notifier<uiMenuHandler>	createnotifier;
    CNotifier<uiMenuHandler,int> handlenotifier;
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
    int				addItem( uiMenuItem*, int placementidx=-1 );
    				/*!<\param placementidx determines where the 
						item should be placed in the
						menu. Items are placed in
						order of decreasing
						placementindexes.
				    \returns the id that will be returned
						if this item is selected. */
    void			addSubMenu( uiPopupMenu*, int placementidx=-1 );
    				/*!<\param placementidx determines where the 
						submenu should be placed in the
						menu. Items are placed in
						order of decreasing
						placementindexes.
				    \note Caller must make sure that all
				    	  subitems gets valid ids and that
					  getFreeID() is called until it
					  returns an id that is equal to the
					  hightest id used in the submenu.  */
    uiPopupMenu*		getMenu( const char* name );
    				/*!\ returns a submenu, so items can be added
				     to it. */

    int				getFreeID() { return freeid++; }
    int				getCurrentID() const { return freeid; }

    static const int		fromTree;
    static const int		fromScene;

protected:
    ObjectSet<uiPopupMenu>	menus;
    TypeSet<int>		menuplacement;
    ObjectSet<uiMenuItem>	items;
    TypeSet<int>		itemids;
    TypeSet<int>		itemplacement;
    int				freeid;
    uiParent*			parent;
    int				menutype;
    const TypeSet<int>*		path;
    Coord3			positionxyz;
    int				id_;
    bool			ishandled;
};


#endif
