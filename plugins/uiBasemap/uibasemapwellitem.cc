/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapwellitem.h"

#include "uiaction.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "uistrings.h"

#include "basemapwell.h"
#include "oduicommon.h"
#include "welltransl.h"


// uiBasemapWellGroup
uiBasemapWellGroup::uiBasemapWellGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,mIOObjContext(Well), isadd)
{
    addNameField();
}


uiBasemapWellGroup::~uiBasemapWellGroup()
{
}


bool uiBasemapWellGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


bool uiBasemapWellGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillPar( par );
    return res;
}


bool uiBasemapWellGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );
    return res;
}



// uiBasemapWellParentTreeItem
const char* uiBasemapWellParentTreeItem::iconName() const
{ return "basemap-well"; }



// uiBasemapWellItem
int uiBasemapWellItem::defaultZValue() const
{ return 100; }

uiBasemapGroup* uiBasemapWellItem::createGroup( uiParent* p,bool isadd )
{ return new uiBasemapWellGroup( p, isadd ); }

uiBasemapParentTreeItem* uiBasemapWellItem::createParentTreeItem()
{ return new uiBasemapWellParentTreeItem(ID()); }

uiBasemapTreeItem* uiBasemapWellItem::createTreeItem( const char* nm )
{ return new uiBasemapWellTreeItem( nm ); }



// uiBasemapWellTreeItem
uiBasemapWellTreeItem::uiBasemapWellTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapWellTreeItem::~uiBasemapWellTreeItem()
{}


bool uiBasemapWellTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

    int nrwells = 0;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrwells );

    while ( nrwells < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrwells; idx++ )
    {
	MultiID wllkey;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),wllkey) )
	    continue;

	if ( basemapobjs_.validIdx(idx) )
	{
	    mDynamicCastGet(Basemap::WellObject*,obj,basemapobjs_[idx])
	    if ( obj ) obj->setKey( wllkey );
	}
	else
	{
	    Basemap::WellObject* obj = new Basemap::WellObject( wllkey );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapWellTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction("Show in 3D"), 1 );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapWellTreeItem::handleSubMenu( int mnuid )
{
    if ( uiBasemapTreeItem::handleSubMenu(mnuid) )
	return true;

    bool handled = true;
    if ( mnuid==1 )
    {
	const int nrobjs = basemapobjs_.size();
	for ( int idx=0; idx<nrobjs; idx++ )
	{
	    mDynamicCastGet(Basemap::WellObject*,obj,basemapobjs_[idx])
	    if ( !obj ) continue;

	    ODMainWin()->sceneMgr().addWellItem( obj->getKey() );
	}
    }
    else
	handled = false;

    return handled;
}
