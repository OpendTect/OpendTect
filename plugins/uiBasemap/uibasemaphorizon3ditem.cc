/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id:  ";

#include "uibasemaphorizon3ditem.h"

#include "basemaphorizon3d.h"

#include "uibasemap.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uistrings.h"
#include "uirgbarray.h"
#include "uitaskrunner.h"
#include "uiworld2ui.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"

#include "coltabindex.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "ioman.h"
#include "survinfo.h"

// uiBasemapHorizon3DGroup
uiBasemapHorizon3DGroup::uiBasemapHorizon3DGroup( uiParent* p, bool isadd )
    : uiBasemapGroup(p)
{
    ioobjfld_ = new uiIOObjSelGrp( this, mIOObjContext(EMHorizon3D),
				   uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );
    ioobjfld_->selectionChanged.notify(
				   mCB(this,uiBasemapHorizon3DGroup,selChg));

    addNameField();
}


uiBasemapHorizon3DGroup::~uiBasemapHorizon3DGroup()
{
}


bool uiBasemapHorizon3DGroup::acceptOK()
{
    const bool res = uiBasemapGroup::acceptOK();

    uiTaskRunner uitr( &BMM().getBasemap() );
    for ( int idx=0; idx<mids_.size(); idx++ )
	EM::EMM().loadIfNotFullyLoaded( mids_[idx], &uitr );

    return res;
}


bool uiBasemapHorizon3DGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapGroup::fillPar( par );

    const int nritems = mids_.size();
    par.set( sKey::NrItems(), nritems );
    for ( int idx=0; idx<nritems; idx++ )
    {
	IOPar ipar;
	ipar.set( sKey::Name(), IOM().nameOf(mids_[idx]) );
	ipar.set( IOPar::compKey(sKey::ID(),0), mids_[idx] );
	const BufferString key = IOPar::compKey( sKeyItem(), idx );
	par.mergeComp( ipar, key );
    }

    return res;
}


bool uiBasemapHorizon3DGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapGroup::usePar( par );
    int nritems = 0;
    par.get( sKey::NrItems(), nritems );

    TypeSet<MultiID> mids( nritems, MultiID::udf() );
    for ( int idx=0; idx<nritems; idx++ )
	par.get( IOPar::compKey(sKey::ID(),idx), mids[idx] );

    ioobjfld_->setChosen( mids );

    return res;
}


uiObject* uiBasemapHorizon3DGroup::lastObject()
{ return ioobjfld_->attachObj(); }


void uiBasemapHorizon3DGroup::selChg(CallBacker *)
{
    ioobjfld_->getChosen( mids_ );
    const int nrsel = ioobjfld_->nrChosen();
    if ( nrsel==1 )
	setItemName( IOM().nameOf(ioobjfld_->currentID()) );
    else
    {
	BufferString typestr = ioobjfld_->getContext().trgroup->userName();
	typestr.add( "s" );
	setItemName( typestr );
    }
}


// uiBasemapHorizon3DTreeItem
uiBasemapHorizon3DTreeItem::uiBasemapHorizon3DTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapHorizon3DTreeItem::~uiBasemapHorizon3DTreeItem()
{
}


bool uiBasemapHorizon3DTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

    MultiID mid;
    if ( !par.get(IOPar::compKey(sKey::ID(),0),mid) ) return false;

    uiTaskRunner uitr( &BMM().getBasemap() );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid, &uitr );
    mDynamicCastGet(EM::Horizon3D*,horizonptr,emobj);

    // will be changed to flatviewbitmapmgr.h after some changes were made
    const uiWorld2Ui& transf( BMM().getBasemap().transform().world2UiData() );
    float worldxmin, worldxmax;
    transf.getWorldXRange( worldxmin, worldxmax );

    float worldymin, worldymax;
    transf.getWorldYRange( worldymin, worldymax );

    const StepInterval<int> inlrg = horizonptr->range().lineRange();
    const StepInterval<int> crlrg = horizonptr->range().trcRange();
    const StepInterval<float> zrg = horizonptr->getZRange();

    const int uixmax = transf.toUiX( worldxmax );
    const int uiymax = transf.toUiY( worldymin );

    uiRGBArray* rgbarr = new uiRGBArray( true );
    rgbarr->setSize( uixmax, uiymax );
    rgbarr->clear( Color::NoColor() );

    ColTab::Sequence sequence( 0 );
    ColTab::Mapper mapper;
    mapper.setRange( horizonptr->getZRange() );
    ColTab::IndexedLookUpTable index( sequence, 255, &mapper );

    BinID bid;
    Coord pt;
    for ( int idw=0; idw<rgbarr->getSize(true); ++idw)
    {
	const int idx = transf.toWorldX( idw );
	for ( int idh=0; idh<rgbarr->getSize(false); ++idh )
	{
	    const int idy = transf.toWorldY( idh );
	    pt = Coord( mCast(double,idx), mCast(double,idy) );
	    bid = SI().transform( pt );

	    const double zvalue = horizonptr->getZ( bid );
	    if ( !mIsUdf(zvalue) )
		rgbarr->set( idw, idh, index.color(zvalue) );
	}
    }


    if ( !basemapobjs_.isEmpty() )
    {
	mDynamicCastGet(Basemap::Horizon3DObject*,obj,basemapobjs_[0])
	if ( !obj ) return false;

	obj->setImage( 0, rgbarr );
	obj->updateGeometry();
    }
    else
    {
	Basemap::Horizon3DObject* obj = new Basemap::Horizon3DObject();
	obj->setImage( 0, rgbarr );

	addBasemapObject( *obj );
	obj->updateGeometry();
    }

    return true;
}


bool uiBasemapHorizon3DTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapHorizon3DTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
	BMM().edit( getFamilyID(), ID() );
    else
	return false;

    return true;
}


// uiBasemapHorizon3DItem
const char* uiBasemapHorizon3DItem::iconName() const
{ return "Horizon3D"; }

uiBasemapGroup* uiBasemapHorizon3DItem::createGroup( uiParent* p, bool isadd )
{
    return new uiBasemapHorizon3DGroup( p, isadd );
}

uiBasemapTreeItem* uiBasemapHorizon3DItem::createTreeItem( const char* nm )
{ return new uiBasemapHorizon3DTreeItem( nm ); }
