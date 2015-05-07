/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		May 2015
________________________________________________________________________

-*/
#include "uibasemapfaultsticksetitem.h"

static const char* rcsID mUsedVar = "$Id: $";

#include "uimenu.h"
#include "uimsg.h"
#include "uistrings.h"


// uiBasemapFaultStickSetParentTreeItem
bool uiBasemapFaultStickSetParentTreeItem::showSubMenu()
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


const char* uiBasemapFaultStickSetParentTreeItem::iconName() const
{ return "basemap-fltss"; }



// uiBasemapFaultStickSetItem
int uiBasemapFaultStickSetItem::defaultZValue() const
{ return 100; }

uiBasemapGroup* uiBasemapFaultStickSetItem::createGroup( uiParent*, bool )
{ return 0; }

uiBasemapParentTreeItem* uiBasemapFaultStickSetItem::createParentTreeItem()
{ return new uiBasemapFaultStickSetParentTreeItem(ID()); }

uiBasemapTreeItem* uiBasemapFaultStickSetItem::createTreeItem( const char* nm )
{ return new uiBasemapFaultStickSetTreeItem( nm ); }



// uiBasemapFaultStickSetTreeItem
uiBasemapFaultStickSetTreeItem::uiBasemapFaultStickSetTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapFaultStickSetTreeItem::~uiBasemapFaultStickSetTreeItem()
{
}


bool uiBasemapFaultStickSetTreeItem::usePar( const IOPar& par )
{
    return true;
}


bool uiBasemapFaultStickSetTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapFaultStickSetTreeItem::handleSubMenu( int mnuid )
{
    return uiBasemapTreeItem::handleSubMenu( mnuid );
}


const char* uiBasemapFaultStickSetTreeItem::parentType() const
{
    return typeid(uiBasemapFaultStickSetParentTreeItem).name();
}

