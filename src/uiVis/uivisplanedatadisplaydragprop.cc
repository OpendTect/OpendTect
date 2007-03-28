/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Feb 2007
 RCS:           $Id: uivisplanedatadisplaydragprop.cc,v 1.2 2007-03-28 14:24:51 cvskris Exp $
________________________________________________________________________

-*/

#include "uivisplanedatadisplaydragprop.h"

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

    return true;
}


bool uiVisPlaneDataDisplayDragProp::revertChanges()
{
    pdd_->setTranslationDragKeys( true, initialscrollstate_ );
    pdd_->setTranslationDragKeys( false, initialpanstate_ );
    return true;
}
