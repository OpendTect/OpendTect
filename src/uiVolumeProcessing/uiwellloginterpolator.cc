/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : Sep. 2010
-*/


#include "uiwellloginterpolator.h"
#include "wellloginterpolator.h"

#include "uigridder2d.h"
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

    const char* ext[] = { "None", "By top/bottom values only",
			  "By parallel to top/base", 0 };
    extensfld_ = new uiGenInput( this, tr("Vertical Extension"),
	    StringListInpSpec(ext) );
    extensfld_->setText( ext[hwi.extensionMethod()] );
    extensfld_->attach( alignedBelow, layermodelfld_ );

    logextenfld_ = new uiGenInput( this, tr("Log extension if needed"),
	    BoolInpSpec(hwi.useLogExtension()) );
    logextenfld_->attach( alignedBelow, extensfld_ );

    uiWellExtractParams::Setup su;
    su.withextractintime(false).withsampling(true);

    welllogsel_ = new uiMultiWellLogSel( this, true, &su );
    welllogsel_->attach( alignedBelow, logextenfld_ );

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

    hwinterpolator_.useLogExtension( logextenfld_->getBoolValue() );
    hwinterpolator_.extensionMethod(
	    (WellLogInterpolator::ExtensionModel)extensfld_->getIntValue() );

    IOPar par;
    algosel_->fillPar( par, true );
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
	welllogsel_->setWellExtractParams( hwinterpolator_.getSelParams() );
}

} // namespace VolProc
