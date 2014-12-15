/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemappicksetitem.h"

#include "uiaction.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "uistrings.h"

#include "basemappickset.h"
#include "oduicommon.h"
#include "picksettr.h"


static IOObjContext getIOObjContext()
{
    IOObjContext ctxt = mIOObjContext( PickSet );
    ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Polygon() );
    return ctxt;
}


// uiBasemapPickSetGroup
uiBasemapPickSetGroup::uiBasemapPickSetGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,getIOObjContext(),isadd)
{
    addNameField();
}


uiBasemapPickSetGroup::~uiBasemapPickSetGroup()
{
}


bool uiBasemapPickSetGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


bool uiBasemapPickSetGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillPar( par );
    return res;
}


bool uiBasemapPickSetGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );
    return res;
}


// uiBasemapPickSetItem
const char* uiBasemapPickSetItem::iconName() const
{ return "picks"; }

uiBasemapGroup* uiBasemapPickSetItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapPickSetGroup( p, isadd ); }

uiBasemapTreeItem* uiBasemapPickSetItem::createTreeItem( const char* nm )
{ return new uiBasemapPickSetTreeItem( nm ); }



// uiBasemapPickSetTreeItem
uiBasemapPickSetTreeItem::uiBasemapPickSetTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapPickSetTreeItem::~uiBasemapPickSetTreeItem()
{}


bool uiBasemapPickSetTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

    int nrpicksets = 0;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrpicksets );

    while ( nrpicksets < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrpicksets; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( basemapobjs_.validIdx(idx) )
	{
	    mDynamicCastGet(Basemap::PickSetObject*,obj,basemapobjs_[idx])
	    if ( obj ) obj->setMultiID( mid );
	}
	else
	{
	    Basemap::PickSetObject* obj = new Basemap::PickSetObject( mid );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapPickSetTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    mnu.insertItem( new uiAction("Show in 3D"), 1 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapPickSetTreeItem::handleSubMenu( int mnuid )
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
	    mDynamicCastGet(Basemap::PickSetObject*,obj,basemapobjs_[idx])
	    if ( !obj ) continue;

	    ODMainWin()->sceneMgr().addPickSetItem( obj->getMultiID() );
	}
    }
    else
	return false;

    return true;
}
