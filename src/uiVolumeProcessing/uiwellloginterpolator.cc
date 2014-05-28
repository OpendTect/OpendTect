/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : Sep. 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiwellloginterpolator.h"
#include "wellloginterpolator.h"

#include "gridder2d.h"
#include "survinfo.h"
#include "volprocchain.h"

#include "uidialog.h"
#include "uigeninput.h"
#include "uiinterpollayermodel.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uivolprocchain.h"


namespace VolProc
{

uiWellLogInterpolator::uiWellLogInterpolator( uiParent* p,
					      WellLogInterpolator& hwi )
    : uiStepDialog( p, WellLogInterpolator::sFactoryDisplayName(), &hwi )
    , hwinterpolator_( hwi )
{
    setHelpKey( mTODOHelpKey );

    layermodelfld_ = new uiInterpolationLayerModel( this );

    const char* ext[] = { "None", "By top/bottom values only",
			  "By parallel to top/base", 0 };
    extensfld_ = new uiGenInput( this, "Vertical Extension",
	    StringListInpSpec(ext) );
    extensfld_->setText( ext[hwi.extensionMethod()] );
    extensfld_->attach( alignedBelow, layermodelfld_ );

    logextenfld_ = new uiGenInput( this, tr("Log extension if needed"),
	    BoolInpSpec(hwi.useLogExtension()) );
    logextenfld_->attach( alignedBelow, extensfld_ );

    uiWellExtractParams::Setup su;
    su.singlelog(true);
    welllogsel_ = new uiMultiWellLogSel( this, su );
    welllogsel_->attach( alignedBelow, logextenfld_ );

    const char* algos[] = { InverseDistanceGridder2D::sFactoryDisplayName(),
	TriangulatedGridder2D::sFactoryDisplayName(), 0 };
    algosel_ = new uiGenInput( this, tr("Algorithm"), StringListInpSpec(algos));
    algosel_->attach( alignedBelow, welllogsel_ );

    BufferString radiustxt = "Search radius [";
    radiustxt += (SI().xyInFeet() ? "ft" : "m" );
    radiustxt += "]";
    radiusfld_ = new uiGenInput( this, radiustxt.buf(), FloatInpSpec() );
    radiusfld_->attach( alignedBelow, algosel_ );
    radiusfld_->setWithCheck( true );

    const BufferString nm = hwinterpolator_.getGridderName();
    if ( nm == TriangulatedGridder2D::sFactoryKeyword() )
	algosel_->setText( algos[1] );
    else
    {
	algosel_->setText( algos[0] );
	const float radius = hwinterpolator_.getSearchRadius();
	if ( mIsUdf(radius) )
	    radiusfld_->setChecked( false );
	else
	{
	    radiusfld_->setChecked( true );
	    radiusfld_->setValue( radius );
	}
    }
    algosel_->valuechanged.notify( mCB(this,uiWellLogInterpolator,algoChg) );

    addNameFld( radiusfld_ );

    postFinalise().notify( mCB(this,uiWellLogInterpolator,finaliseCB) );
}


uiWellLogInterpolator::~uiWellLogInterpolator()
{
}


void uiWellLogInterpolator::finaliseCB( CallBacker* )
{
    initWellLogSel();
}


uiStepDialog* uiWellLogInterpolator::createInstance( uiParent* p, Step* vs )
{
    mDynamicCastGet( WellLogInterpolator*, hw, vs );
    return hw ? new uiWellLogInterpolator( p, *hw ) : 0;
}


void uiWellLogInterpolator::algoChg( CallBacker* )
{
    radiusfld_->display( !algosel_->getIntValue() );
}


bool uiWellLogInterpolator::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    IOPar laymodpar;
    layermodelfld_->fillPar( laymodpar );
    hwinterpolator_.setLayerModel( laymodpar );

    hwinterpolator_.useLogExtension( logextenfld_->getBoolValue() );
    hwinterpolator_.extensionMethod(
	    (WellLogInterpolator::ExtensionModel)extensfld_->getIntValue() );

    const bool validrad =  !algosel_->getIntValue() &&
	radiusfld_->isChecked() && !mIsUdf(radiusfld_->getfValue());
    const float radius = validrad ? radiusfld_->getfValue() : mUdf(float);
    const char* nm = !algosel_->getIntValue()
	? InverseDistanceGridder2D::sFactoryKeyword()
	: TriangulatedGridder2D::sFactoryKeyword();
    hwinterpolator_.setGridder( nm, radius );

    TypeSet<MultiID> wellids;
    BufferStringSet lognms;
    welllogsel_->getSelWellIDs( wellids );
    welllogsel_->getSelLogNames( lognms );

    if ( wellids.isEmpty() || lognms.isEmpty() )
    {
	uiMSG().error( tr("Please select at least one well and one log") );
	return false;
    }

    hwinterpolator_.setWellData( wellids, lognms.get(0) );
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
}

} // namespace VolProc

