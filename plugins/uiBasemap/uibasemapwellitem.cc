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
uiBasemapWellGroup::uiBasemapWellGroup( uiParent* p )
    : uiBasemapIOObjGroup(p,mIOObjContext(Well))
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


// uiBasemapWellItem
const char* uiBasemapWellItem::iconName() const
{ return "well"; }

uiBasemapGroup* uiBasemapWellItem::createGroup( uiParent* p )
{ return new uiBasemapWellGroup( p ); }

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
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( basemapobjs_.validIdx(idx) )
	{
	    mDynamicCastGet(Basemap::WellObject*,obj,basemapobjs_[idx])
	    if ( obj ) obj->setMultiID( mid );
	}
	else
	{
	    Basemap::WellObject* obj = new Basemap::WellObject( mid );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapWellTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    mnu.insertItem( new uiAction("Show in 3D"), 1 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapWellTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
    {
	BMM().edit( getFamilyID(), ID() );
    }
    else if ( mnuid==1 )
    {
	const int nrobjs = basemapobjs_.size();
	for ( int idx=0; idx<nrobjs; idx++ )
	{
	    mDynamicCastGet(Basemap::WellObject*,obj,basemapobjs_[idx])
	    if ( !obj ) continue;

	    ODMainWin()->sceneMgr().addWellItem( obj->getMultiID() );
	}
    }
    else
	return false;

    return true;
}

