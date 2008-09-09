/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: uivolprocsmoother.cc,v 1.4 2008-09-09 08:23:02 cvsbert Exp $";

#include "uivolprocsmoother.h"

#include "survinfo.h"
#include "uimsg.h"
#include "volprocsmoother.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uiwindowfunctionsel.h"
#include "uivolprocchain.h"
#include "volprocsmoother.h"


#define mMaxNrSteps	10

namespace VolProc
{


void uiSmoother::initClass()
{
    VolProc::uiChain::factory().addCreator( create, Smoother::sKeyType() );
}    


uiSmoother::uiSmoother( uiParent* p, Smoother* hf )
    : uiStepDialog( p, uiDialog::Setup( Smoother::sUserName(),
		    Smoother::sUserName(), "0.0.0" ), hf )
    , smoother_( hf )
{
    operatorselfld_ = new uiWindowFunctionSel( this, "Operator",
	    		smoother_->getOperatorName(),
			smoother_->getOperatorParam() );
    operatorselfld_->attach( alignedBelow, namefld_ );

    inllenfld_ = new uiLabeledSpinBox( this, "In-line width", 0,
	    			  	"Inline_spinbox" );

    const BinID step( SI().inlStep(), SI().crlStep() );
    inllenfld_->box()->setInterval( 1, mMaxNrSteps*2*step.inl+1, 2*step.inl );
    inllenfld_->box()->setValue( step.inl*(smoother_->inlSz()-1)+1 );
    inllenfld_->attach( alignedBelow, operatorselfld_ );

    crllenfld_ = new uiLabeledSpinBox( this, "Cross-line width", 0,
	    			       "Crline_spinbox" );
    crllenfld_->box()->setInterval( 1, mMaxNrSteps*2*step.crl+1, 2*step.crl );
    crllenfld_->box()->setValue( step.crl*(smoother_->crlSz()-1)+1 );
    crllenfld_->attach( alignedBelow, inllenfld_ );

    const float zstep = SI().zStep();
    BufferString zlabel = "Vertical size ";
    zlabel += SI().getZUnit(true);

    zlenfld_ = new uiLabeledSpinBox( this, zlabel.buf(), 0, "Z_spinbox" );
    zlenfld_->box()->setInterval( (float) 0, mMaxNrSteps*2*zstep, 2*zstep );
    zlenfld_->box()->setValue( zstep*(smoother_->zSz()-1) );
    zlenfld_->attach( alignedBelow, crllenfld_ );
}


uiSmoother::~uiSmoother()
{
}


uiStepDialog* uiSmoother::create( uiParent* parent, Step* ps )
{
    mDynamicCastGet( Smoother*, hf, ps );
    if ( !hf ) return 0;

    return new uiSmoother( parent, hf );
}


bool uiSmoother::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    const int inlsz = mNINT((inllenfld_->box()->getFValue()-1)/SI().inlStep() )+1;
    const int crlsz = mNINT((crllenfld_->box()->getFValue()-1)/SI().crlStep() )+1;
    const int zsz = mNINT(zlenfld_->box()->getFValue()/SI().zStep() )+1;

    if ( !inlsz && !crlsz && !zsz )
    {
	uiMSG().error("At least one size must be non-zero" );
	return false;
    }

    if ( !smoother_->setOperator( operatorselfld_->windowName(),
				  operatorselfld_->windowParamValue(),
				  inlsz, crlsz, zsz ) )
    {
	uiMSG().error( "Cannot set selected operator" );
	return false;
    }

    return true;
}


};//namespace

