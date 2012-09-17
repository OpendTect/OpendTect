/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: uivolproclateralsmoother.cc,v 1.9 2012/07/10 13:06:09 cvskris Exp $";

#include "uivolproclateralsmoother.h"

#include "survinfo.h"
#include "uimsg.h"
#include "volprocsmoother.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uigeninput.h"
#include "uivolprocchain.h"
#include "volproclateralsmoother.h"


namespace VolProc
{


uiLateralSmoother::uiLateralSmoother( uiParent* p, LateralSmoother* hf )
    : uiStepDialog( p, LateralSmoother::sFactoryDisplayName(), hf )
    , smoother_( hf )
{
    setHelpID( "103.6.3" ); 
    const Array2DFilterPars* pars = hf ? &hf->getPars() : 0;

    uiGroup* stepoutgroup = new uiGroup( this, "Stepout" );
    stepoutgroup->setFrame( true );

    inllenfld_ = new uiLabeledSpinBox( stepoutgroup, "In-line stepout", 0,
	    			  	"Inline_spinbox" );

    const BinID step( SI().inlStep(), SI().crlStep() );
    inllenfld_->box()->setInterval( 0, 200*step.inl, step.inl );
    if ( pars )
	inllenfld_->box()->setValue( step.inl*pars->stepout_.row );

    crllenfld_ = new uiLabeledSpinBox( stepoutgroup, "Cross-line stepout", 0,
	    			       "Crline_spinbox" );
    crllenfld_->box()->setInterval( 0, 200*step.crl, step.crl );
    if ( pars )
	crllenfld_->box()->setValue( step.crl*pars->stepout_.col );
    crllenfld_->attach( alignedBelow, inllenfld_ );

    replaceudfsfld_ = new uiGenInput( stepoutgroup,
	    "Overwrite undefined values",
	    BoolInpSpec( pars && pars->filludf_ ));
    replaceudfsfld_->attach( alignedBelow, crllenfld_ );

    stepoutgroup->setHAlignObj( crllenfld_ );

    ismedianfld_ = new uiGenInput( this, sKey::Type,
	    BoolInpSpec( pars && pars->type_==Stats::Median,
			 Stats::TypeNames()[(int)Stats::Median],
			 Stats::TypeNames()[(int)Stats::Average]) );
    ismedianfld_->valuechanged.notify( mCB(this,uiLateralSmoother,updateFlds) );
    ismedianfld_->attach( alignedBelow, stepoutgroup );

    weightedfld_ = new uiGenInput( this, "Weighted",
	    BoolInpSpec( pars && !mIsUdf(pars->rowdist_) ) );
    weightedfld_->attach( alignedBelow, ismedianfld_ );

    mirroredgesfld_ = new uiGenInput( this, "Mirror edges",
	    BoolInpSpec( smoother_ ? smoother_->getMirrorEdges() : true ) );
    mirroredgesfld_->attach( alignedBelow, weightedfld_ );

    const char* udfhanlingstrs[] =
	{ "Average", "Fixed value", "Interpolate", 0 };
    udfhandling_ = new uiGenInput( this, "Undefined substitution",
	    StringListInpSpec( udfhanlingstrs ) );
    udfhandling_->attach( alignedBelow, mirroredgesfld_ );
    udfhandling_->valuechanged.notify( mCB(this,uiLateralSmoother,updateFlds) );

    udffixedvalue_ = new uiGenInput( this, "Fixed value",
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


uiStepDialog* uiLateralSmoother::createInstance( uiParent* parent, Step* ps )
{
    mDynamicCastGet( LateralSmoother*, hf, ps );
    if ( !hf ) return 0;

    return new uiLateralSmoother( parent, hf );
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

    pars.stepout_.row = mNINT32(inllenfld_->box()->getFValue()/SI().inlStep() );
    pars.stepout_.col = mNINT32(crllenfld_->box()->getFValue()/SI().crlStep() );
    pars.filludf_ = replaceudfsfld_->getBoolValue();

    smoother_->setPars( pars );

    smoother_->setMirrorEdges( mirroredgesfld_->getBoolValue() );
    smoother_->setInterpolateUdfs( udfhandling_->getIntValue()==2 );
    if ( udfhandling_->getIntValue()==1 )
    {
	const float val = udffixedvalue_->getfValue();
	if ( mIsUdf(val) )
	{
	    uiMSG().error( "Fixed value must be defined" );
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


};//namespace

