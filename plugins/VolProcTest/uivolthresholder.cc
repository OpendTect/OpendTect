/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolthresholder.h"

#include "uivolumeprocessing.h"
#include "volumeprocessing.h"
#include "volprocthresholder.h"
#include "uigeninput.h"

namespace VolProc
{

void uiVolumeThresholder::initClass()
{
    VolProc::uiPS().addCreator( create, ThresholdStep::sKeyType() );
}    


uiVolumeThresholder::uiVolumeThresholder( uiParent* p, ThresholdStep* ts )
	: uiDialog( p, uiDialog::Setup( "Volume Thresholder", 0, mNoHelpID ) )
	, thresholdstep_( ts )
{   
    thresholdfld_ = new uiGenInput( this, "Threshold",
				    FloatInpSpec(ts->getThreshold()) );
}


uiDialog* uiVolumeThresholder::create( uiParent* parent, ProcessingStep* ps )
{
    mDynamicCastGet( ThresholdStep*, ts, ps );
    if ( !ts ) return 0;

    return new uiVolumeThresholder( parent, ts );
}


bool uiVolumeThresholder::acceptOK( CallBacker* )
{
    if ( thresholdfld_->isUndef( 0 ) )
	return false;

    thresholdstep_->setThreshold( thresholdfld_->getFValue() );
    return true;
}

} //namespace
