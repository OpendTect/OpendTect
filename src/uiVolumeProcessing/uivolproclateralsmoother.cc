/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolproclateralsmoother.h"

#include "survinfo.h"
#include "uimsg.h"
#include "volprocsmoother.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uigeninput.h"
#include "uivolprocchain.h"
#include "volproclateralsmoother.h"
#include "od_helpids.h"


namespace VolProc
{

uiLateralSmoother::uiLateralSmoother( uiParent* p, LateralSmoother* hf,
					bool is2d )
    : uiStepDialog( p, LateralSmoother::sFactoryDisplayName(), hf, is2d )
    , smoother_( hf )
    , inllenfld_(0)
{
    setHelpKey( mODHelpKey(mLateralSmootherHelpID) );
    const Array2DFilterPars* pars = hf ? &hf->getPars() : 0;

    uiGroup* stepoutgroup = new uiGroup( this, "Stepout" );
    stepoutgroup->setFrame( true );
    const BinID step( SI().inlStep(), is2d ? 1 : SI().crlStep() );

    if ( !is2d )
    {
	uiString linesolabel = uiStrings::phrJoinStrings( uiStrings::sInline(),
							uiStrings::sStepout());
	inllenfld_ = new uiLabeledSpinBox( stepoutgroup, linesolabel,
					    0, "Inline_spinbox" );
	inllenfld_->box()->setInterval( 0, 200*step.inl(), step.inl() );
	if ( pars )
	    inllenfld_->box()->setValue( step.inl()*pars->stepout_.row() );
    }

    uiString trcsolabel = uiStrings::phrJoinStrings(
		is2d ? uiStrings::sTraceNumber() : uiStrings::sCrossline(),
		uiStrings::sStepout() );
    crllenfld_ = new uiLabeledSpinBox( stepoutgroup, trcsolabel, 0,
				       "Crline_spinbox" );
    crllenfld_->box()->setInterval( 0, 200*step.crl(), step.crl() );
    if ( pars )
	crllenfld_->box()->setValue( step.crl()*pars->stepout_.col() );
    if ( inllenfld_ )
	crllenfld_->attach( alignedBelow, inllenfld_ );

    replaceudfsfld_ = new uiGenInput( stepoutgroup,
	    tr("Overwrite undefined values"),
	    BoolInpSpec( pars && pars->filludf_ ));
    replaceudfsfld_->attach( alignedBelow, crllenfld_ );

    stepoutgroup->setHAlignObj( crllenfld_ );

    ismedianfld_ = new uiGenInput( this, uiStrings::sType(),
	    BoolInpSpec( pars && pars->type_==Stats::Median,
	     uiStrings::sMedian(), uiStrings::sAverage()) );
    ismedianfld_->valueChanged.notify( mCB(this,uiLateralSmoother,updateFlds) );
    ismedianfld_->attach( alignedBelow, stepoutgroup );

    weightedfld_ = new uiGenInput( this, tr("Weighted"),
	    BoolInpSpec( pars && !mIsUdf(pars->rowdist_) ) );
    weightedfld_->attach( alignedBelow, ismedianfld_ );

    mirroredgesfld_ = new uiGenInput( this, tr("Mirror edges"),
	    BoolInpSpec( smoother_ ? smoother_->getMirrorEdges() : true ) );
    mirroredgesfld_->attach( alignedBelow, weightedfld_ );

    const char* udfhanlingstrs[] =
	{ "Average", "Fixed value", "Interpolate", 0 };
    udfhandling_ = new uiGenInput( this, tr("Undefined substitution"),
	    StringListInpSpec( udfhanlingstrs ) );
    udfhandling_->attach( alignedBelow, mirroredgesfld_ );
    udfhandling_->valueChanged.notify( mCB(this,uiLateralSmoother,updateFlds) );

    udffixedvalue_ = new uiGenInput( this, tr("Fixed value"),
	    FloatInpSpec( mUdf(float) ) );
    udffixedvalue_->attach( alignedBelow, udfhandling_ );

    if ( smoother_ && smoother_->getInterpolateUdfs() )
	udfhandling_->setValue( 2 );
    else if ( smoother_ && !mIsUdf(smoother_->getFixedValue() ) )
    {
	udffixedvalue_->setValue( smoother_->getFixedValue() );
	udfhandling_->setValue( 1 );
    }
    else
    {
	udfhandling_->setValue( 0 );
    }

    addNameFld( udffixedvalue_ );
    updateFlds( 0 );
}


uiLateralSmoother::~uiLateralSmoother()
{}


uiStepDialog* uiLateralSmoother::createInstance( uiParent* parent, Step* ps,
						 bool is2d )
{
    mDynamicCastGet( LateralSmoother*, hf, ps );
    if ( !hf ) return 0;

    return new uiLateralSmoother( parent, hf, is2d );
}


bool uiLateralSmoother::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    Array2DFilterPars pars;
    pars.type_ = ismedianfld_->getBoolValue() ? Stats::Median : Stats::Average;
    pars.rowdist_ = !ismedianfld_->getBoolValue()&&weightedfld_->getBoolValue()
	? 1
	: mUdf(float);

    pars.stepout_.row() = !inllenfld_ ? 0 :
		mNINT32(inllenfld_->box()->getFValue()/SI().inlStep() );
    pars.stepout_.col() =
		mNINT32(crllenfld_->box()->getFValue()/SI().crlStep() );
    pars.filludf_ = replaceudfsfld_->getBoolValue();

    smoother_->setPars( pars );

    smoother_->setMirrorEdges( mirroredgesfld_->getBoolValue() );
    smoother_->setInterpolateUdfs( udfhandling_->getIntValue()==2 );
    if ( udfhandling_->getIntValue()==1 )
    {
	const float val = udffixedvalue_->getFValue();
	if ( mIsUdf(val) )
	{
	    uiMSG().error( tr("Fixed value must be defined") );
	    return false;
	}

	smoother_->setFixedValue( val );
    }
    else
    {
	smoother_->setFixedValue( mUdf(float) );
    }

    return true;
}


void uiLateralSmoother::updateFlds( CallBacker* )
{
    weightedfld_->display( !ismedianfld_->getBoolValue() );
    udfhandling_->display( !ismedianfld_->getBoolValue() );
    mirroredgesfld_->display( !ismedianfld_->getBoolValue() );
    udffixedvalue_->display( !ismedianfld_->getBoolValue() &&
			     udfhandling_->getIntValue()==1 );
}

} // namespace VolProc
