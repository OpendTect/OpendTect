/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		March 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapzsliceitem.h"

#include "uiattribpartserv.h"
#include "uibasemapcoltabed.h"
#include "uibitmapdisplay.h"
#include "uigeninput.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiworld2ui.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "coltab.h"
#include "flatposdata.h"
#include "flatview.h"
#include "seisdatapack.h"
#include "seistrctr.h"
#include "settings.h"
#include "survinfo.h"

// uiBasemapZSliceGroup
uiBasemapZSliceGroup::uiBasemapZSliceGroup( uiParent* p, bool isadd )
    : uiBasemapGroup(p)
{
    Viewer2DPosDataSel* posdatasel = new Viewer2DPosDataSel;
    posdatasel->postype_ = Viewer2DPosDataSel::ZSlice;
    posgrp_ = new uiODViewer2DPosGrp( this, posdatasel, false, true );
    posgrp_->inpSelected.notify( mCB(this,uiBasemapZSliceGroup,selChg) );

    addNameField();

    selChg( 0 );
}


uiBasemapZSliceGroup::~uiBasemapZSliceGroup()
{
}


void uiBasemapZSliceGroup::selChg( CallBacker* )
{
    setItemName( posgrp_->posDataSel().selspec_.userRef() );
}


bool uiBasemapZSliceGroup::acceptOK()
{
    if ( !posgrp_->commitSel() )
	return false;

    const bool res = uiBasemapGroup::acceptOK();

    return res;
}


bool uiBasemapZSliceGroup::fillPar( IOPar& par ) const
{
    if ( !uiBasemapGroup::fillPar(par) )
	return false;

    par.set( sKey::NrItems(), 1 );

    IOPar ipar;
    ipar.set( sKey::Name(), posgrp_->posDataSel().selspec_.userRef() );
    posgrp_->fillPar( ipar );

    const BufferString key = IOPar::compKey( sKeyItem(), 0 );
    par.mergeComp( ipar, key );

    return true;
}


bool uiBasemapZSliceGroup::usePar( const IOPar& par )
{
    posgrp_->usePar( par );

    return uiBasemapGroup::usePar( par );
}


uiObject* uiBasemapZSliceGroup::lastObject()
{
    return posgrp_->attachObj();
}



//uiBasemapZSliceObject
uiBasemapZSliceObject::uiBasemapZSliceObject()
    : uiBaseMapObject(0)
    , appearance_(*new FlatView::Appearance)
    , bitmapdisp_(*new uiBitMapDisplay(appearance_))
    , dp_(0)

{
    appearance_.ddpars_.show( false, true );
    bitmapdisp_.getDisplay()->setZValue( 0 );
    itemgrp_.add( bitmapdisp_.getDisplay() );

    BufferString seqnm = ColTab::defSeqName();
    Settings::common().get( "dTect.Color table", seqnm );
    appearance_.ddpars_.vd_.ctab_ = seqnm;

    if ( BMM().getColTabEd() )
	BMM().getColTabEd()->colTabChgd.notify(
			mCB(this,uiBasemapZSliceObject,colTabChgCB) );
}


uiBasemapZSliceObject::~uiBasemapZSliceObject()
{
    itemgrp_.remove( bitmapdisp_.getDisplay(), false );

    delete &appearance_;
    delete &bitmapdisp_;

    DPM(DataPackMgr::FlatID()).release( dp_ );

    if ( BMM().getColTabEd() )
	BMM().getColTabEd()->colTabChgd.remove(
			mCB(this,uiBasemapZSliceObject,colTabChgCB) );
}


void uiBasemapZSliceObject::colTabChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiBasemapColTabEd*,coltabed,cb);
    if ( !coltabed ) return;

    appearance_.ddpars_.vd_ = coltabed->getDisplayPars();
    bitmapdisp_.update();
}


void uiBasemapZSliceObject::setZSlice( const Viewer2DPosDataSel& pdsel )
{
    BufferStringSet dimnames;
    dimnames.add("X").add("Y").add(sKey::Inline()).add(sKey::Crossline());

    StepInterval<double> inlrg, crlrg;
    inlrg.setFrom( pdsel.tkzs_.hsamp_.inlRange() );
    crlrg.setFrom( pdsel.tkzs_.hsamp_.crlRange() );

    uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
    if ( !attrserv ) return;
    attrserv->setTargetSelSpec( pdsel.selspec_ );
    DataPack::ID vwr2ddpid =
	attrserv->createOutput( pdsel.tkzs_, DataPack::cNoID() );

    DataPackMgr& dpmgr = DPM( DataPackMgr::SeisID() );
    mDynamicCastGet(RegularSeisDataPack*,sdp,dpmgr.obtain(vwr2ddpid))
    if ( !sdp ) return;

    Array2DSlice<float> slice2d( sdp->data() );
    slice2d.setDimMap( 0, 0 );
    slice2d.setDimMap( 1, 1 );
    slice2d.setPos( 2, 0 );
    slice2d.init();

    dp_ = new MapDataPack( "ZSlice", &slice2d );
    dp_->setProps( inlrg, crlrg, true, &dimnames );
    DPM(DataPackMgr::FlatID()).addAndObtain( dp_ );
    bitmapdisp_.setDataPack( dp_, false );
    bitmapdisp_.getDisplay()->show();
    update();

    if ( BMM().getColTabEd() )
	BMM().getColTabEd()->setColTab( appearance_.ddpars_.vd_ );
}


void uiBasemapZSliceObject::update()
{
    if ( !dp_ ) return;

    const FlatPosData& pd = dp_->posData();
    StepInterval<double> rg0( pd.range(true) );
    StepInterval<double> rg1( pd.range(false) );
    rg0.sort( true );
    rg1.sort( true );
    bitmapdisp_.setBoundingBox(
	uiWorldRect(rg0.start,rg1.stop,rg0.stop,rg1.start) );
    bitmapdisp_.update();
}


// uiBasemapZSliceTreeItem
uiBasemapZSliceTreeItem::uiBasemapZSliceTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
    , uibmobj_(new uiBasemapZSliceObject)
{
    BMM().getBasemap().worldItemGroup().add( &uibmobj_->itemGrp() );
}


uiBasemapZSliceTreeItem::~uiBasemapZSliceTreeItem()
{
    BMM().getBasemap().worldItemGroup().remove( &uibmobj_->itemGrp(), true );
    delete uibmobj_;
}


bool uiBasemapZSliceTreeItem::usePar( const IOPar& par )
{
    if ( !uiBasemapTreeItem::usePar(par) )
	return false;

    Viewer2DPosDataSel posdatasel;
    posdatasel.usePar( par );

    uibmobj_->setZSlice( posdatasel );

    return true;
}


void uiBasemapZSliceTreeItem::checkCB(CallBacker *)
{
    uibmobj_->show( isChecked() );
}


bool uiBasemapZSliceTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapZSliceTreeItem::handleSubMenu( int mnuid )
{
    return uiBasemapTreeItem::handleSubMenu( mnuid );
}


const char* uiBasemapZSliceTreeItem::parentType() const
{ return typeid(uiBasemapZSliceParentTreeItem).name(); }


// uiBasemapZSliceItem
int uiBasemapZSliceItem::defaultZValue() const
{ return 0; }


const char* uiBasemapZSliceItem::iconName() const
{ return "basemap-zslice"; }


uiBasemapGroup* uiBasemapZSliceItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapZSliceGroup( p, isadd ); }


uiBasemapParentTreeItem* uiBasemapZSliceItem::createParentTreeItem()
{ return new uiBasemapZSliceParentTreeItem( ID() ); }


uiBasemapTreeItem* uiBasemapZSliceItem::createTreeItem( const char* nm )
{ return new uiBasemapZSliceTreeItem( nm ); }
