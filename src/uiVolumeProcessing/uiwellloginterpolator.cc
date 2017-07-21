/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : Sep. 2010
-*/


#include "uiwellloginterpolator.h"
#include "wellloginterpolator.h"

#include "uigridder2d.h"
#include "gridder2d.h"
#include "survinfo.h"
#include "volprocchain.h"

#include "uidialog.h"
#include "uigeninput.h"
#include "uiinterpollayermodel.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uivolprocchain.h"

#include "od_helpids.h"

namespace VolProc
{

uiWellLogInterpolator::uiWellLogInterpolator( uiParent* p,
					      WellLogInterpolator& hwi,
					      bool is2d )
    : uiStepDialog( p, WellLogInterpolator::sFactoryDisplayName(), &hwi, is2d )
    , hwinterpolator_( hwi )
{
    setHelpKey( mODHelpKey(mWellLogInterpolHelpID) );

    layermodelfld_ = new uiInterpolationLayerModel( this );

    uiWellExtractParams::Setup su;
    su.withextractintime(false).withsampling(true);

    welllogsel_ = new uiMultiWellLogSel( this, true, &su );
    welllogsel_->attach( alignedBelow, layermodelfld_ );

    algosel_ = new uiGridder2DSel( this, hwi.getGridder(), hwi.getTrendOrder());
    algosel_->attach( alignedBelow, welllogsel_ );

    addNameFld( algosel_ );

    postFinalise().notify( mCB(this,uiWellLogInterpolator,finaliseCB) );
}


uiWellLogInterpolator::~uiWellLogInterpolator()
{
}


void uiWellLogInterpolator::finaliseCB( CallBacker* )
{
    layermodelfld_->setModel( hwinterpolator_.getLayerModel() );
    initWellLogSel();
}


uiStepDialog* uiWellLogInterpolator::createInstance( uiParent* p, Step* vs,
						     bool is2d )
{
    mDynamicCastGet( WellLogInterpolator*, wli, vs );
    return wli ? new uiWellLogInterpolator( p, *wli, is2d ) : 0;
}


bool uiWellLogInterpolator::acceptOK()
{
    if ( !uiStepDialog::acceptOK() )
	return false;

    InterpolationLayerModel* mdl = layermodelfld_->getModel();
    hwinterpolator_.setLayerModel( mdl );

    IOPar par;
    algosel_->fillPar( par, true );
    if ( !algosel_->message().isEmpty() )
	uiMSG().warning( algosel_->message() );

    hwinterpolator_.usePar( par );

    DBKeySet wellids;
    BufferStringSet lognms;
    welllogsel_->getSelWellIDs( wellids );
    welllogsel_->getSelLogNames( lognms );

    if ( wellids.isEmpty() || lognms.isEmpty() )
    {
	uiMSG().error(uiStrings::phrSelect(tr(
					     "at least one well and one log")));
	return false;
    }

    hwinterpolator_.setWellData( wellids, lognms.get(0) );
    if ( welllogsel_->isWellExtractParamsUsed() )
	hwinterpolator_.setWellExtractParams(
					 *welllogsel_->getWellExtractParams() );
    PolyTrend::Order trend;
    PtrMan<IOPar> gridpar = par.subselect( Gridder2D::sKeyGridder() );
    if ( !gridpar ||
	 !PolyTrend::OrderDef().parse(*gridpar,PolyTrend::sKeyOrder(),trend) )
	return true;

    uiString msg;
    if ( !PolyTrend::getOrder(wellids.size(),trend,&msg) )
    {
	uiMSG().warning( msg );
	par.set( PolyTrend::sKeyOrder(), PolyTrend::OrderDef().getKey(trend) );
    }

    return true;
}


void uiWellLogInterpolator::initWellLogSel()
{
    BufferStringSet wellnms;
    hwinterpolator_.getWellNames( wellnms );
    welllogsel_->setSelWellNames( wellnms );

    BufferStringSet lognms;
    lognms.add( hwinterpolator_.getLogName() );
    welllogsel_->setSelLogNames( lognms );
    if ( welllogsel_->isWellExtractParamsUsed() )
	welllogsel_->setWellExtractParams(
				hwinterpolator_.getWellExtractParams() );
}

} // namespace VolProc
