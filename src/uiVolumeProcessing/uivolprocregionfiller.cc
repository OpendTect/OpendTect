/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

#include "uivolprocregionfiller.h"
#include "volprocregionfiller.h"

#include "uibodyregiondlg.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uihorauxdatasel.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uitaskrunner.h"
#include "uivolprocchain.h"

#include "emregion.h"
#include "emsurfaceposprov.h"
#include "od_helpids.h"
#include "seisstatscalc.h"
#include "seistrctr.h"
#include "separstr.h"
#include "survinfo.h"


namespace VolProc
{

uiStepDialog* uiRegionFiller::createInstance( uiParent* p, Step* step,
					      bool is2d )
{
    mDynamicCastGet(RegionFiller*,rf,step);
    return rf ? new uiRegionFiller( p, rf, is2d ) : 0;
}


uiRegionFiller::uiRegionFiller( uiParent* p, RegionFiller* rf, bool is2d )
    : uiStepDialog( p, RegionFiller::sFactoryDisplayName(), rf, is2d )
    , regionfiller_( rf )
{
    setHelpKey( mNoHelpKey );

    regiongrp_ = new uiBodyRegionGrp( this, uiBodyRegionGrp::Setup() );
    if ( !rf->region().isEmpty() )
	regiongrp_->setRegion( rf->region() );

    constvalfld_ = new uiGenInput( this, tr("Constant Value"), FloatInpSpec() );
    constvalfld_->setValue( rf->getInsideValue() );
    constvalfld_->setElemSzPol( uiObject::Wide );
    constvalfld_->attach( alignedBelow, regiongrp_ );
    constvalfld_->display(false);

    velocitygrp_ = createVelGrp();
    velocitygrp_->attach( alignedBelow, regiongrp_ );

    startvalCB(0);
    gradientCB(0);

    addNameFld( velocitygrp_ );
}


uiGroup* uiRegionFiller::createVelGrp()
{
    uiGroup* grp = new uiGroup( this, "Velocity Group" );

    uiHorizonAuxDataSel::HorizonAuxDataInfo auxdatainfo( true, this );
    const bool hasauxdata = auxdatainfo.dbkys_.size();
    const uiString fromhorattribstr = tr("From Horizon Data");

    const DBKey& starthorid = regionfiller_->getStartValueHorizonID();
    const bool conststartval = starthorid.isInvalid();
    startvalselfld_ = new uiGenInput( grp, tr("Start value"),
        BoolInpSpec(conststartval,uiStrings::sConstant(false),
                        fromhorattribstr) );
    startvalselfld_->setSensitive( hasauxdata );
    startvalselfld_->valuechanged.notify( mCB(this,uiRegionFiller,startvalCB) );
    startvalselfld_->attach( ensureBelow, regiongrp_ );

    startvalfld_ = new uiGenInput( grp, tr("Start value constant"),
		FloatInpSpec(regionfiller_->getStartValue()) );
    startvalfld_->attach( alignedBelow, startvalselfld_ );
    statsbut_ = new uiPushButton( grp, tr("From Statistics"), false );
    statsbut_->activated.notify( mCB(this,uiRegionFiller,statsPushCB) );
    statsbut_->attach( rightTo, startvalfld_ );

    starthorfld_ = new uiHorizonAuxDataSel( grp, starthorid,
		regionfiller_->getStartValueAuxDataIdx(), &auxdatainfo );
    starthorfld_->attach( alignedBelow, startvalselfld_ );

    const DBKey& gradhorid = regionfiller_->getGradientHorizonID();
    const bool constgrad = gradhorid.isInvalid();
    gradvalselfld_ = new uiGenInput( grp, uiStrings::sGradient(),
        BoolInpSpec(constgrad,uiStrings::sConstant(false),fromhorattribstr) );
    gradvalselfld_->setSensitive( hasauxdata );
    gradvalselfld_->valuechanged.notify( mCB(this,uiRegionFiller,gradientCB) );
    gradvalselfld_->attach( alignedBelow, startvalfld_ );

    float gradient = regionfiller_->getGradientValue();
    if ( !mIsUdf(gradient) )
	gradient /= SI().zDomain().userFactor();
    gradvalfld_ = new uiGenInput( grp, sGradientLabel(),
				  FloatInpSpec(gradient) );
    gradvalfld_->attach( alignedBelow, gradvalselfld_ );

    gradhorfld_ = new uiHorizonAuxDataSel( grp, gradhorid,
	    regionfiller_->getGradientAuxDataIdx(), &auxdatainfo );
    gradhorfld_->attach( alignedBelow, gradvalselfld_ );

    grp->setHAlignObj( startvalselfld_ );
    return grp;
}


uiString uiRegionFiller::sGradientLabel()
{
    return tr("Gradient constant (/%1)").arg( SI().zUnitString());
}

uiRegionFiller::~uiRegionFiller()
{
}


void uiRegionFiller::startvalCB(CallBacker *)
{
    const bool doconst = startvalselfld_->getBoolValue();
    startvalfld_->display( doconst );
    statsbut_->display( doconst );
    starthorfld_->display( !doconst );
}


void uiRegionFiller::gradientCB(CallBacker *)
{
    const bool doconst = gradvalselfld_->getBoolValue();
    gradvalfld_->display( doconst );
    gradhorfld_->display( !doconst );
}


static const char* statsstrs[] =
{
    "Average", "Median", "RMS",
    "StdDev", "Variance", "Min", "Max", 0
};

class uiStatsDlg : public uiDialog
{ mODTextTranslationClass(uiStatsDlg)

public:
uiStatsDlg( uiParent* p, const EM::Region3D& reg )
    : uiDialog(p,Setup(tr("Calculate Data Statistics"),mNoDlgTitle,mNoHelpKey))
    , value_(mUdf(float))
    , region_(reg)
{
    const IOObjContext ctxt = mIOObjContext( SeisTrc );
    inpfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(false,false) );

    statsfld_ = new uiGenInput( this, uiStrings::sStatistics(),
				StringListInpSpec(statsstrs) );
    statsfld_->attach( alignedBelow, inpfld_ );
}


Stats::Type getSelectedStatsType() const
{
    const int selidx = statsfld_->getIntValue();
    Stats::Type type;
    switch ( selidx )
    {
	case 0:		type = Stats::Average; break;
	case 1:		type = Stats::Median; break;
	case 2:		type = Stats::RMS; break;
	case 3:		type = Stats::StdDev; break;
	case 4:		type = Stats::Variance; break;
	case 5:		type = Stats::Min; break;
	case 6:		type = Stats::Max; break;
	default:	type = Stats::Average; break;
    }

    return type;
}


bool acceptOK()
{
    const IOObj* ioobj = inpfld_->ioobj();
    if ( !ioobj ) return false;

    Stats::CalcSetup cs( false );
    cs.require( getSelectedStatsType() );
    Pos::EMRegion3DProvider prov;
    IOPar regionpars; region_.fillPar( regionpars );
    prov.region().usePar( regionpars );
    SeisStatsCalc statscalc( *ioobj, cs, &prov );

    uiTaskRunner dlg( this );
    const bool res = dlg.execute( statscalc );
    if ( !res )
    {
	uiMSG().error( statscalc.message() );
	return false;
    }

    value_ = statscalc.getStats(0).getValue( getSelectedStatsType() );

    return true;
}


double getValue() const
{ return value_; }

enum Type
{
    Average=1, Median=2, RMS=3,
    StdDev=4, Variance=5, Min=7, Max=8
};

    uiSeisSel*		inpfld_;
    uiGenInput*		statsfld_;
    double		value_;

    const EM::Region3D&	region_;
};


void uiRegionFiller::statsPushCB( CallBacker* )
{
    if ( !regiongrp_->accept() ) return;

    uiStatsDlg dlg( this, regiongrp_->region() );
    if ( !dlg.go() ) return;

    constvalfld_->setValue( dlg.getValue() );
}


bool uiRegionFiller::acceptOK()
{
    if ( !uiStepDialog::acceptOK() || !regiongrp_->accept() )
	return false;

    IOPar regionpars;
    regiongrp_->region().fillPar( regionpars );
    regionfiller_->region().usePar( regionpars );
    regionfiller_->setInsideValue( constvalfld_->getFValue() );

    return true;
}

} // namespace VolProc
