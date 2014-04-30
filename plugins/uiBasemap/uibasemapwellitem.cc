/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id: uibasemapwellitem.h 34190 2014-04-16 20:09:04Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/

#include "uibasemapwellitem.h"

#include "uiaction.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uistrings.h"

#include "basemapwell.h"
#include "welltransl.h"


// uiBasemapWellGroup
uiBasemapWellGroup::uiBasemapWellGroup( uiParent* p, const char* nm )
    : uiBasemapGroup(p,nm)
{
    IOObjContext ctxt = mIOObjContext( Well );
    ctxt.forread = true;
    wellsfld_ = new uiIOObjSelGrp( this, ctxt, 0, true );

    addNameField( wellsfld_->attachObj() );
}


uiBasemapWellGroup::~uiBasemapWellGroup()
{
}


bool uiBasemapWellGroup::fillPar( IOPar& par ) const
{
    return true;
}


bool uiBasemapWellGroup::usePar( const IOPar& par )
{
    return true;
}


// uiBasemapWellItem
const char* uiBasemapWellItem::iconName() const
{ return "well"; }


void uiBasemapWellItem::add()
{
    ObjectSet<MultiID> wellids;
    applMgr().selectWells( wellids );
    if ( wellids.isEmpty() )
	return;

    for ( int idx=0; idx<wellids.size(); idx++ )
    {
	Basemap::WellObject* obj = new Basemap::WellObject( *wellids[idx] );
	addBasemapObject( *obj );
	obj->updateGeometry();
    }

    const char* nm = "Wells";
    addTreeItem( *new uiBasemapWellTreeItem(nm) );
}


void uiBasemapWellItem::edit()
{
}


// uiBasemapWellTreeItem
uiBasemapWellTreeItem::uiBasemapWellTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


bool uiBasemapWellTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapWellTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
    {
    }
    else
	return false;

    return true;
}

