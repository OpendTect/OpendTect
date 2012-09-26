/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocfaultangle.h"

#include "uigeninput.h"
#include "uivolprocchain.h"
#include "volprocchain.h"
#include "uispinbox.h"

namespace VolProc
{


uiFaultAngle::uiFaultAngle( uiParent* p, VolProc::FaultAngle* fa )
    : uiStepDialog( p, FaultAngle::sFactoryDisplayName(), fa )
    , fltaz_( fa )
{
    setHelpID( mTODOHelpID );

    const char* outopt[] = { "Fault flag", "Fault azimuth", "Fault dip", 0 };
    outputoptfld_ = new uiGenInput( this, "Output", StringListInpSpec(outopt) );
    outputoptfld_->setValue( (int)fa->getutputOption() );

    minlengthfld_ = new uiLabeledSpinBox( this, "Minimum fault length" );
    minlengthfld_->attach( alignedBelow, outputoptfld_ );
    minlengthfld_->box()->setInterval( StepInterval<int>(1,1000,1) );
    minlengthfld_->box()->setValue( fa->minFaultLength() );

    dothinningfld_ = new uiGenInput( this, "Use thinning", 
	    BoolInpSpec(fa->doThinning()) );
    dothinningfld_->attach( alignedBelow, minlengthfld_ );
    
    overlapratefld_ = new uiGenInput( this, "Max overlap rate",
	    FloatInpSpec(fa->overlapRate()) );
    overlapratefld_->attach( alignedBelow, dothinningfld_ );
    
    domergefld_ = new uiGenInput( this, "Use merge", 
	    BoolInpSpec(true) ); //fa->doMerge()) );
    domergefld_->attach( alignedBelow, overlapratefld_ );
    domergefld_->valuechanged.notify( mCB(this,uiFaultAngle,mergeChgCB) );
    domergefld_->setSensitive( false );
    
    thresholdfld_ = new uiGenInput(this, "Fault threshold", 
	    FloatInpSpec(fa->threshold()));
    thresholdfld_->attach( alignedBelow, domergefld_ );
    
    isabovefld_ = new uiGenInput( this, "Fault value",
	    BoolInpSpec(fa->isFltAbove(),"Above threshold","Below threshold"));
    isabovefld_->attach( alignedBelow, thresholdfld_ );

    addNameFld( isabovefld_ ); 
}


uiFaultAngle::~uiFaultAngle()
{
}


void uiFaultAngle::mergeChgCB( CallBacker* )
{
    const bool usemerge = domergefld_->getBoolValue();
    thresholdfld_->display( usemerge );
    isabovefld_->display( usemerge );
}


uiStepDialog* uiFaultAngle::createInstance( uiParent* parent, Step* ps )
{
    mDynamicCastGet( VolProc::FaultAngle*, hf, ps );
    if ( !hf ) return 0;

    return new uiFaultAngle( parent, hf );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiFaultAngle::acceptOK( CallBacker* cb )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    fltaz_->setOutputOption( 
	    (FaultAngle::OutputOption)outputoptfld_->getIntValue() );
    fltaz_->setMinFaultLength( minlengthfld_->box()->getValue() );
    fltaz_->doThinning( dothinningfld_->getBoolValue() );
    fltaz_->doMerge( domergefld_->getBoolValue() );
    fltaz_->overlapRate( overlapratefld_->getfValue() );
    fltaz_->threshold( thresholdfld_->getfValue() );
    fltaz_->isFltAbove( isabovefld_->getBoolValue() );

    return true;
}


};//namespace

