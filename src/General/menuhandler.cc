/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id: menuhandler.cc,v 1.2 2005-08-10 16:17:14 cvskris Exp $
________________________________________________________________________

-*/


#include "menuhandler.h"

#include "bufstringset.h"
#include "errh.h"


MenuItemHolder::MenuItemHolder()
    : parent( 0 )
    , removal(this)
{}


MenuItemHolder::~MenuItemHolder()
{
    removal.trigger();
    removeItems();
}


void MenuItemHolder::addItem( MenuItem* item, bool manage )
{
    items += item;
    manageitems += manage;
    item->parent = this;

    item->removal.notify(mCB(this,MenuItemHolder,itemIsDeletedCB));

    assignItemID(*item);
}


void MenuItemHolder::removeItems()
{
    for ( int idx=0; idx<items.size(); idx++ )
    {
	items[idx]->removal.remove(mCB(this,MenuItemHolder,itemIsDeletedCB));

	if ( manageitems[idx] ) delete items[idx];
	else if ( items[idx]->parent==this ) items[idx]->parent = 0;
    }

    items.erase();
    manageitems.erase();
}


int MenuItemHolder::nrItems() const { return items.size(); }


const MenuItem* MenuItemHolder::getItem( int idx ) const
{ return const_cast<MenuItemHolder*>(this)->getItem(idx); }


MenuItem* MenuItemHolder::getItem( int idx )
{ return idx>=0 && idx<nrItems() ? items[idx] : 0; }


int MenuItemHolder::itemIndex( const MenuItem* item ) const
{ return items.indexOf(item); }


int MenuItemHolder::itemIndex( int searchid ) const
{
    for ( int idx=0; idx<items.size(); idx++ )
    {
	if ( items[idx]->id==searchid )
	    return idx;
    }

    return -1;
}



MenuItem* MenuItemHolder::findItem( int searchid )
{
    for ( int idx=0; idx<items.size(); idx++ )
    {
	if ( items[idx]->id==searchid )
	    return items[idx];
    }

    for ( int idx=0; idx<items.size(); idx++ )
    {
	MenuItem* item = items[idx]->findItem(searchid);
	if ( item ) return item;
    }

    return 0;
}


const MenuItem* MenuItemHolder::findItem( const char* txt ) const
{ return const_cast<MenuItemHolder*>(this)->findItem(txt); }


MenuItem* MenuItemHolder::findItem( const char* txt)
{
    for ( int idx=0; idx<items.size(); idx++ )
    {
	if ( !strcmp(items[idx]->text, txt ) )
	    return items[idx];
    }

    for ( int idx=0; idx<items.size(); idx++ )
    {
	MenuItem* item = items[idx]->findItem(txt);
	if ( item ) return item;
    }

    return 0;
}


const MenuItem* MenuItemHolder::findItem( int searchid ) const
{ return const_cast<MenuItemHolder*>(this)->findItem(searchid); }


const ObjectSet<MenuItem>& MenuItemHolder::getItems() const
{ return items; }


void MenuItemHolder::itemIsDeletedCB(CallBacker* cb)
{
    const int idx = items.indexOf(reinterpret_cast<MenuItem*>(cb));
    if ( idx==-1 )
	pErrMsg( "Hugh?" );
    else
    {
	items.remove(idx);
	manageitems.remove(idx);
    }
}


void MenuItemHolder::assignItemID(MenuItem& item)
{
    if ( parent ) parent->assignItemID(item);
}


MenuItem::MenuItem( const char* txt, int pl, const CallBack& ncb )
    : text( txt )
    , placement( pl )
    , cb( ncb )
    , checked( false )
    , enabled( true )
    , id( -1 )
{}


void MenuItem::createItems( const BufferStringSet& names )
{
    removeItems();

    for ( int idx=0; idx<names.size(); idx++ )
	addItem( new MenuItem(names.get(idx)), true );
}


MenuHandler::MenuHandler( int ni )
    : id_( ni )
    , createnotifier( this )
    , handlenotifier( this )
{
    mRefCountConstructor;
}


MenuHandler::~MenuHandler()
{}


bool MenuHandler::isHandled() const
{ return ishandled; }


void MenuHandler::setIsHandled( bool yn )
{ ishandled = yn; }


void MenuHandler::assignItemID( MenuItem& itm )
{
    itm.id = freeid++;

    for ( int idx=0; idx<itm.items.size(); idx++ )
	assignItemID( *itm.items[idx] );
}
