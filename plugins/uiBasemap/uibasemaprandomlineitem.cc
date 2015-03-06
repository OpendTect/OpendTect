/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemaprandomlineitem.h"

#include "uiaction.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uisellinest.h"
#include "uistrings.h"

#include "basemaprandomline.h"
#include "oduicommon.h"
#include "randomlinetr.h"


// uiBasemapRandomLineGroup
uiBasemapRandomLineGroup::uiBasemapRandomLineGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,mIOObjContext(RandomLineSet),isadd)
{
    uiSelLineStyle::Setup stu; stu.drawstyle( false );
    LineStyle lst;
    lsfld_ = new uiSelLineStyle( this, lst, stu );
    lsfld_->attach( alignedBelow, uiBasemapIOObjGroup::lastObject() );

    addNameField();
}


uiBasemapRandomLineGroup::~uiBasemapRandomLineGroup()
{
}


bool uiBasemapRandomLineGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


bool uiBasemapRandomLineGroup::fillItemPar( int idx, IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillItemPar( idx, par );

    BufferString lsstr;
    lsfld_->getStyle().toString( lsstr );
    par.set( sKey::LineStyle(), lsstr );

    return res;
}


bool uiBasemapRandomLineGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls; ls.fromString( lsstr );
    lsfld_->setStyle( ls );

    return res;
}


uiObject* uiBasemapRandomLineGroup::lastObject()
{ return lsfld_->attachObj(); }


// uiBasemapRandomLineItem
const char* uiBasemapRandomLineItem::iconName() const
{ return "basemap-randomline"; }


uiBasemapGroup* uiBasemapRandomLineItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapRandomLineGroup( p, isadd ); }


uiBasemapTreeItem* uiBasemapRandomLineItem::createTreeItem( const char* nm )
{ return new uiBasemapRandomLineTreeItem( nm ); }



// uiBasemapRandomLineTreeItem
uiBasemapRandomLineTreeItem::uiBasemapRandomLineTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapRandomLineTreeItem::~uiBasemapRandomLineTreeItem()
{}


bool uiBasemapRandomLineTreeItem::usePar( const IOPar& par )
{
    const IOPar prevpar = pars_;
    uiBasemapTreeItem::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

    int nrobjs = 0;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrobjs );

    while ( nrobjs < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrobjs; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( !basemapobjs_.validIdx(idx) )
	{
	    Basemap::RandomLineObject* obj = new Basemap::RandomLineObject();
	    addBasemapObject( *obj );
	}

	mDynamicCastGet(Basemap::RandomLineObject*,obj,basemapobjs_[idx])
	if ( !obj ) return false;

	if ( hasParChanged(prevpar,par,uiBasemapGroup::sKeyNrObjs()) )
	{
	    // if the number of objects is different, everything needs to be
	    // redraw So...
	    obj->setLineStyle( 0, ls );
	    obj->setMultiID( mid );
	    obj->updateGeometry();
	    continue;
	}

	if ( hasParChanged(prevpar,par,sKey::LineStyle()) )
	    obj->setLineStyle( 0, ls );

	if ( hasSubParChanged(prevpar,par,sKey::ID()) )
	{
	    obj->setMultiID( mid );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapRandomLineTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapRandomLineTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
	BMM().edit( getFamilyID(), ID() );
    else
	return false;

    return true;
}

