/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		March 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";

#include "uibasemapinlcrlitem.h"

#include "uimenu.h"
#include "uimsg.h"
#include "uistrings.h"


// uiBasemapInlParentTreeItem
bool uiBasemapInlParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sAdd(true)), 0 );
    const int mnuid = mnu.exec();
    if ( mnuid==0 )
    {
	uiMSG().message( "Not implemented yet" );
    }

    return true;
}


const char* uiBasemapInlParentTreeItem::iconName() const
{ return "basemap-inl"; }



// uiBasemapCrlParentTreeItem
bool uiBasemapCrlParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sAdd(true)), 0 );
    const int mnuid = mnu.exec();
    if ( mnuid==0 )
    {
	uiMSG().message( "Not implemented yet" );
    }

    return true;
}


const char* uiBasemapCrlParentTreeItem::iconName() const
{ return "basemap-crl"; }



// uiBasemapInlItem
int uiBasemapInlItem::defaultZValue() const
{ return 100; }

uiBasemapGroup* uiBasemapInlItem::createGroup( uiParent*, bool )
{ return 0; }

uiBasemapParentTreeItem* uiBasemapInlItem::createParentTreeItem()
{ return new uiBasemapInlParentTreeItem(ID()); }

uiBasemapTreeItem* uiBasemapInlItem::createTreeItem( const char* nm )
{ return new uiBasemapInlTreeItem( nm ); }



// uiBasemapInlTreeItem
uiBasemapInlTreeItem::uiBasemapInlTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapInlTreeItem::~uiBasemapInlTreeItem()
{
}


bool uiBasemapInlTreeItem::usePar( const IOPar& par )
{
    return true;
}


bool uiBasemapInlTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapInlTreeItem::handleSubMenu( int mnuid )
{
    return uiBasemapTreeItem::handleSubMenu( mnuid );
}


const char* uiBasemapInlTreeItem::parentType() const
{
    return typeid(uiBasemapInlParentTreeItem).name();
}



// uiBasemapCrlItem
int uiBasemapCrlItem::defaultZValue() const
{ return 100; }

uiBasemapGroup* uiBasemapCrlItem::createGroup( uiParent*, bool )
{ return 0; }

uiBasemapParentTreeItem* uiBasemapCrlItem::createParentTreeItem()
{ return new uiBasemapCrlParentTreeItem(ID()); }

uiBasemapTreeItem* uiBasemapCrlItem::createTreeItem( const char* nm )
{ return new uiBasemapCrlTreeItem( nm ); }



// uiBasemapCrlTreeItem
uiBasemapCrlTreeItem::uiBasemapCrlTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapCrlTreeItem::~uiBasemapCrlTreeItem()
{
}


bool uiBasemapCrlTreeItem::usePar( const IOPar& par )
{
    return true;
}


bool uiBasemapCrlTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapCrlTreeItem::handleSubMenu( int mnuid )
{
    return uiBasemapTreeItem::handleSubMenu( mnuid );
}


const char* uiBasemapCrlTreeItem::parentType() const
{
    return typeid(uiBasemapCrlParentTreeItem).name();
}

