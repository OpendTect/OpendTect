/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisplanedatadisplaydragprop.cc,v 1.5 2009/07/22 16:01:43 cvsbert Exp $";

#include "uivisplanedatadisplaydragprop.h"

#include "settings.h"
#include "uibuttonstateedit.h"
#include "uimsg.h"
#include "visplanedatadisplay.h"

uiVisPlaneDataDisplayDragProp::uiVisPlaneDataDisplayDragProp(uiParent* p,
	                            visSurvey::PlaneDataDisplay* pdd )
    : uiDlgGroup( p, "Mouse interaction" )
    , initialscrollstate_( pdd->getTranslationDragKeys(true) )
    , initialpanstate_( pdd->getTranslationDragKeys(false) )
    , pdd_( pdd )
{
    scrollstate_ = new uiButtonStateEdit( this, "Scroll movement",
	    				  initialscrollstate_ );
    panstate_ = new uiButtonStateEdit( this, "Pan movement",
	    				  initialpanstate_ );
    panstate_->attach( alignedBelow, scrollstate_ );

}


bool uiVisPlaneDataDisplayDragProp::acceptOK()
{
    if ( scrollstate_->getState()==panstate_->getState() )
    {
	uiMSG().error( "Scroll and Pan movement cannot be the same" );
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
