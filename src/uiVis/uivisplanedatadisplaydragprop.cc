/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivisplanedatadisplaydragprop.h"

#include "settings.h"
#include "uibuttonstateedit.h"
#include "uimsg.h"
#include "visplanedatadisplay.h"

uiVisPlaneDataDisplayDragProp::uiVisPlaneDataDisplayDragProp(uiParent* p,
	                            visSurvey::PlaneDataDisplay* pdd )
    : uiDlgGroup( p, tr("Mouse interaction") )
    , initialscrollstate_( pdd->getTranslationDragKeys(true) )
    , initialpanstate_( pdd->getTranslationDragKeys(false) )
    , pdd_( pdd )
{
    scrollstate_ = new uiButtonStateEdit( this, tr("Scroll movement"),
					  initialscrollstate_ );
    panstate_ = new uiButtonStateEdit( this, tr("Pan movement"),
					  initialpanstate_ );
    panstate_->attach( alignedBelow, scrollstate_ );

}


uiVisPlaneDataDisplayDragProp::~uiVisPlaneDataDisplayDragProp()
{}


bool uiVisPlaneDataDisplayDragProp::acceptOK()
{
    if ( scrollstate_->getState()==panstate_->getState() )
    {
	uiMSG().error( tr("Scroll and Pan movement cannot be the same") );
	return false;
    }

    pdd_->setTranslationDragKeys( true, scrollstate_->getState() );
    pdd_->setTranslationDragKeys( false, panstate_->getState() );

    mSettUse( set, "dTect.MouseInteraction", pdd_->sKeyDepthKey(),
	      scrollstate_->getState() );
    mSettUse( set, "dTect.MouseInteraction", pdd_->sKeyPlaneKey(),
	      panstate_->getState() );
    Settings::common().write();

    return true;
}


bool uiVisPlaneDataDisplayDragProp::revertChanges()
{
    pdd_->setTranslationDragKeys( true, initialscrollstate_ );
    pdd_->setTranslationDragKeys( false, initialpanstate_ );
    return true;
}
