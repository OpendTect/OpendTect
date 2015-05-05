/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		March 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";

#include "uibasemapinlcrlitem.h"

#include "uibasemap.h"
#include "uiioobjselgrp.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uisellinest.h"
#include "uistrings.h"
#include "uitaskrunner.h"

#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"

#include "axislayout.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "survinfo.h"

#include "uimsg.h"

// uiBasemapInlCrlGroup
uiBasemapInlCrlGroup::uiBasemapInlCrlGroup( uiParent* p, bool isadd )
    : uiBasemapGroup(p)
    , mid_(MultiID::udf())
{
    uiSelLineStyle::Setup stu; stu.drawstyle( false );
    LineStyle lst( LineStyle::Solid, 1, Color(0,170,0,0) );
    lsfld_ = new uiSelLineStyle( this, lst, stu );

    addNameField();
}


uiBasemapInlCrlGroup::~uiBasemapInlCrlGroup()
{
}


bool uiBasemapInlCrlGroup::acceptOK()
{
    uiMSG().warning( "Not fully implemented yet" );
    return uiBasemapGroup::acceptOK();
}


bool uiBasemapInlCrlGroup::fillPar( IOPar& par ) const
{
    par.set( sKey::NrItems(), 1 );

    IOPar ipar;
    const bool res = uiBasemapGroup::fillPar( ipar );

    BufferString lsstr;
    lsfld_->getStyle().toString( lsstr );
    ipar.set( sKey::LineStyle(), lsstr );

    const BufferString key = IOPar::compKey( sKeyItem(), 0 );
    par.mergeComp( ipar, key );

    return res;
}


bool uiBasemapInlCrlGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapGroup::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );
    lsfld_->setStyle( ls );

    return res;
}


uiObject* uiBasemapInlCrlGroup::lastObject()
{ return lsfld_->attachObj(); }



// uiBasemapInlItem
int uiBasemapInlItem::defaultZValue() const
{ return 100; }

const char* uiBasemapInlItem::iconName() const
{ return "basemap-inl"; }

uiBasemapGroup* uiBasemapInlItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapInlCrlGroup( p, isadd ); }

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
    const IOPar prevpar = pars_;
    uiBasemapTreeItem::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

//    mDynamicCastGet(Basemap::InlObject*,obj,basemapobjs_[0])
//    if ( obj )
//    {
//	if ( hasParChanged(prevpar,par,sKey::LineStyle()) )
//	    obj->setLineStyle( 0, ls );
//    }

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

const char* uiBasemapCrlItem::iconName() const
{ return "basemap-crl"; }

uiBasemapGroup* uiBasemapCrlItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapInlCrlGroup( p, isadd ); }

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
    const IOPar prevpar = pars_;
    uiBasemapTreeItem::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

//    mDynamicCastGet(Basemap::CrlObject*,obj,basemapobjs_[0])
//    if ( obj )
//    {
//	if ( hasParChanged(prevpar,par,sKey::LineStyle()) )
//	    obj->setLineStyle( 0, ls );
//    }

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


