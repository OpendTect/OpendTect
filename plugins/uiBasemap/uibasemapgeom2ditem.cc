/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapgeom2ditem.h"

#include "uiaction.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uistrings.h"

#include "basemapgeom2d.h"
#include "oduicommon.h"
#include "survgeometrytransl.h"


// uiBasemapGeom2DGroup
uiBasemapGeom2DGroup::uiBasemapGeom2DGroup( uiParent* p )
    : uiBasemapGroup(p)
{
    IOObjContext ctxt( mIOObjContext(Survey::SurvGeom) );
    ctxt.fixTranslator( Survey::dgb2DSurvGeomTranslator::translKey() );
    geom2dfld_ = new uiIOObjSelGrp( this, ctxt,
	uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );

    addNameField( geom2dfld_->attachObj() );
}


uiBasemapGeom2DGroup::~uiBasemapGeom2DGroup()
{
}


bool uiBasemapGeom2DGroup::acceptOK()
{
    const int nrobjs = geom2dfld_->nrChosen();
    if ( nrobjs == 0 )
    {
	uiMSG().error( "Select at least one 2D line" );
	return false;
    }

    const bool res = uiBasemapGroup::acceptOK();
    return res;
}


bool uiBasemapGeom2DGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapGroup::fillPar( par );
    TypeSet<MultiID> mids;
    geom2dfld_->getChosen( mids );
    const int nrobjs = mids.size();
    par.set( sKeyNrObjs(), nrobjs );
    for ( int idx=0; idx<nrobjs; idx++ )
	par.set( IOPar::compKey(sKey::ID(),idx), mids[idx] );

    return res;
}


bool uiBasemapGeom2DGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapGroup::usePar( par );
    int nrobjs = 0;
    par.get( sKeyNrObjs(), nrobjs );
    TypeSet<MultiID> mids( nrobjs, MultiID::udf() );
    for ( int idx=0; idx<nrobjs; idx++ )
	par.get( IOPar::compKey(sKey::ID(),idx), mids[idx] );

    geom2dfld_->setChosen( mids );
    return res;
}


// uiBasemapGeom2DItem
const char* uiBasemapGeom2DItem::iconName() const
{ return "geom2d"; }

uiBasemapGroup* uiBasemapGeom2DItem::createGroup( uiParent* p )
{ return new uiBasemapGeom2DGroup( p ); }

uiBasemapTreeItem* uiBasemapGeom2DItem::createTreeItem( const char* nm )
{ return new uiBasemapGeom2DTreeItem( nm ); }



// uiBasemapGeom2DTreeItem
uiBasemapGeom2DTreeItem::uiBasemapGeom2DTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapGeom2DTreeItem::~uiBasemapGeom2DTreeItem()
{}


bool uiBasemapGeom2DTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

    int nrobjs = 0;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrobjs );

    while ( nrobjs < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrobjs; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( basemapobjs_.validIdx(idx) )
	{
	    mDynamicCastGet(Basemap::Geom2DObject*,obj,basemapobjs_[idx])
	    if ( obj ) obj->setMultiID( mid );
	}
	else
	{
	    Basemap::Geom2DObject* obj =
		new Basemap::Geom2DObject( mid );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapGeom2DTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapGeom2DTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
    {
	BMM().edit( getFamilyID(), ID() );
    }
    else
	return false;

    return true;
}

