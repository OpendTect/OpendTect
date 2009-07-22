/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: uivolproclateralsmoother.cc,v 1.2 2009-07-22 16:01:43 cvsbert Exp $";

#include "uivolproclateralsmoother.h"

#include "survinfo.h"
#include "uimsg.h"
#include "volprocsmoother.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uigeninput.h"
#include "uivolprocchain.h"
#include "volproclateralsmoother.h"


namespace VolProc
{


void uiLateralSmoother::initClass()
{
    uiChain::factory().addCreator( create, LateralSmoother::sKeyType() );
}    


uiLateralSmoother::uiLateralSmoother( uiParent* p, LateralSmoother* hf )
    : uiStepDialog( p, LateralSmoother::sUserName(), hf )
    , smoother_( hf )
{
    const Array2DFilterPars* pars = hf ? &hf->getPars() : 0;
    ismedianfld_ = new uiGenInput( this, sKey::Type,
	    BoolInpSpec( pars && pars->type_==Stats::Median,
			 Stats::TypeNames()[(int)Stats::Median],
			 Stats::TypeNames()[(int)Stats::Average]) );
    ismedianfld_->valuechanged.notify( mCB(this,uiLateralSmoother,updateFlds) );

    weightedfld_ = new uiGenInput( this, "Weighted",
	    BoolInpSpec( pars && !mIsUdf(pars->rowdist_) ) );
    weightedfld_->attach( alignedBelow, ismedianfld_ );

    uiLabel* label = new uiLabel( this, "Stepout" );
    label->attach( alignedBelow, weightedfld_ );

    uiGroup* stepoutgroup = new uiGroup( this, "Stepout" );
    stepoutgroup->setFrame( true );
    stepoutgroup->attach( alignedBelow, label );

    inllenfld_ = new uiLabeledSpinBox( stepoutgroup, "In-line", 0,
	    			  	"Inline_spinbox" );

    const BinID step( SI().inlStep(), SI().crlStep() );
    inllenfld_->box()->setInterval( 0, 200*step.inl, step.inl );
    if ( pars )
	inllenfld_->box()->setValue( step.inl*pars->stepout_.row );

    crllenfld_ = new uiLabeledSpinBox( stepoutgroup, "Cross-line", 0,
	    			       "Crline_spinbox" );
    crllenfld_->box()->setInterval( 0, 200*step.crl, step.crl );
    if ( pars )
	crllenfld_->box()->setValue( step.crl*pars->stepout_.col );
    crllenfld_->attach( alignedBelow, inllenfld_ );

    stepoutgroup->setHAlignObj( crllenfld_ );
    addNameFld( stepoutgroup );

    updateFlds( 0 );
}


uiStepDialog* uiLateralSmoother::create( uiParent* parent, Step* ps )
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

    pars.stepout_.row = mNINT(inllenfld_->box()->getFValue()/SI().inlStep() );
    pars.stepout_.col = mNINT(crllenfld_->box()->getFValue()/SI().crlStep() );

    smoother_->setPars( pars );

    return true;
}


void uiLateralSmoother::updateFlds( CallBacker* )
{
    weightedfld_->display( !ismedianfld_->getBoolValue() );
}


};//namespace

