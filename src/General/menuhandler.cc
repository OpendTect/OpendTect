/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "menuhandler.h"

#include "bufstringset.h"
#include "errh.h"
#include "string2.h"
#include "threadwork.h"


MenuItemHolder::MenuItemHolder()
    : parent_(0)
    , removal(this)
{}


MenuItemHolder::~MenuItemHolder()
{
    removal.trigger();
    removeItems();
}


void MenuItemHolder::addItem( MenuItem* item, bool manage )
{
    if ( !item ) return;

    items_ += item;
    manageitems_ += manage;
    item->parent_ = this;

    item->removal.notify( mCB(this,MenuItemHolder,itemIsDeletedCB) );
    assignItemID(*item);
}


void MenuItemHolder::removeItems()
{
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	items_[idx]->removal.remove( mCB(this,MenuItemHolder,itemIsDeletedCB) );

	if ( manageitems_[idx] )
	    delete items_[idx];
	else if ( items_[idx]->parent_==this )
	    items_[idx]->parent_ = 0;
    }

    items_.erase();
    manageitems_.erase();
}


int MenuItemHolder::nrItems() const { return items_.size(); }


const MenuItem* MenuItemHolder::getItem( int idx ) const
{ return const_cast<MenuItemHolder*>(this)->getItem(idx); }


MenuItem* MenuItemHolder::getItem( int idx )
{ return idx>=0 && idx<nrItems() ? items_[idx] : 0; }


int MenuItemHolder::itemIndex( const MenuItem* item ) const
{ return items_.indexOf(item); }


int MenuItemHolder::itemIndex( int searchid ) const
{
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	if ( items_[idx]->id==searchid )
	    return idx;
    }

    return -1;
}



MenuItem* MenuItemHolder::findItem( int searchid )
{
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	if ( items_[idx]->id==searchid )
	    return items_[idx];
    }

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	MenuItem* item = items_[idx]->findItem( searchid );
	if ( item ) return item;
    }

    return 0;
}


const MenuItem* MenuItemHolder::findItem( const char* txt ) const
{ return const_cast<MenuItemHolder*>(this)->findItem(txt); }


MenuItem* MenuItemHolder::findItem( const char* txt )
{
    BufferString tofindtxt = txt;
    removeCharacter( tofindtxt.buf(), '&' );
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	BufferString itmtxt = items_[idx]->text;
	removeCharacter( itmtxt.buf(), '&' );
	if ( itmtxt == tofindtxt )
	    return items_[idx];
    }

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	MenuItem* item = items_[idx]->findItem( txt );
	if ( item ) return item;
    }

    return 0;
}


const MenuItem* MenuItemHolder::findItem( int searchid ) const
{ return const_cast<MenuItemHolder*>(this)->findItem(searchid); }


const ObjectSet<MenuItem>& MenuItemHolder::getItems() const
{ return items_; }


void MenuItemHolder::itemIsDeletedCB( CallBacker* cb )
{
    const int idx = items_.indexOf( reinterpret_cast<MenuItem*>(cb) );
    if ( idx==-1 )
	pErrMsg( "Hugh?" );
    else
    {
	items_.removeSingle( idx );
	manageitems_.removeSingle( idx );
    }
}


void MenuItemHolder::assignItemID( MenuItem& item )
{
    if ( parent_ ) parent_->assignItemID( item );
}


// MenuItem

static int itemid = 0;

MenuItem::MenuItem( const char* txt, int pl )
    : text(txt)
    , placement(pl)
    , checkable(false)
    , checked(false)
    , enabled(true)
{
    id = itemid++;
}


void MenuItem::createItems( const BufferStringSet& names )
{
    removeItems();

    for ( int idx=0; idx<names.size(); idx++ )
	addItem( new MenuItem(names.get(idx)), true );
}



MenuHandler::MenuHandler( int id )
    : id_(id)
    , initnotifier(this)
    , createnotifier(this)
    , handlenotifier(this)
    , queueid_(
	Threads::WorkManager::twm().addQueue( Threads::WorkManager::Manual ) )
{}


MenuHandler::~MenuHandler()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );
}


bool MenuHandler::isHandled() const
{ return ishandled_; }


void MenuHandler::setIsHandled( bool yn )
{ ishandled_ = yn; }


void MenuHandler::executeQueue()
{
    Threads::WorkManager::twm().executeQueue( queueid_ );
}


void MenuHandler::assignItemID( MenuItem& itm )
{
    if ( itm.id < 0 )
	itm.id = itemid++;

    for ( int idx=0; idx<itm.items_.size(); idx++ )
	assignItemID( *itm.items_[idx] );
}


MenuItemHandler::MenuItemHandler( MenuHandler& mh, const char* nm,
				  const CallBack& cb, const char* parenttext,
				  int placement )
    : menuitem_(nm,placement)
    , cb_(cb)
    , menuhandler_(mh)
    , doadd_(true)
    , isenabled_(true)
    , ischecked_(false)
    , parenttext_(parenttext)
{
    menuhandler_.createnotifier.notify( mCB(this,MenuItemHandler,createMenuCB));
    menuhandler_.handlenotifier.notify( mCB(this,MenuItemHandler,handleMenuCB));
}


MenuItemHandler::~MenuItemHandler()
{
    menuhandler_.createnotifier.remove( mCB(this,MenuItemHandler,createMenuCB));
    menuhandler_.handlenotifier.remove( mCB(this,MenuItemHandler,handleMenuCB));
}


void MenuItemHandler::createMenuCB(CallBacker*)
{
    if ( doadd_ && shouldAddMenu() )
    {
	MenuItemHolder* parentitem = menuhandler_.findItem( parenttext_ );
	if ( !parentitem ) parentitem = &menuhandler_;

	mAddMenuItem( parentitem, &menuitem_, isenabled_&&shouldBeEnabled(),
		      ischecked_ || shouldBeChecked() );
    }
    else
    {
	mResetMenuItem( &menuitem_ );
    }
}


void MenuItemHandler::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,mnuid,cb);
    if ( menuhandler_.isHandled() || mnuid!=menuitem_.id )
	return;

    cb_.doCall( this );
    menuhandler_.setIsHandled( true );
}


void MenuItemHandler::setIcon( const char* fnm )
{ menuitem_.iconfnm = fnm; }

