/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Nov 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uigvfreehandareaselectiontool.h"
#include "uigraphicsview.h"
#include "mouseevent.h"


uiGVFreehandAreaSelectionTool::uiGVFreehandAreaSelectionTool(uiGraphicsView& gv)
    : gv_(gv)
    , ispolygonmode_(true)
    , enabled_(true)
    , polygonselitem_(0)
    , started(this)
    , pointAdded(this)
    , stopped(this)
{
    mAttachCB( gv_.getMouseEventHandler().movement,
		uiGVFreehandAreaSelectionTool::mouseMoveCB );
    
    mAttachCB( gv_.getMouseEventHandler().buttonPressed,
		uiGVFreehandAreaSelectionTool::mousePressCB );

    mAttachCB( gv_.getMouseEventHandler().buttonReleased,
		uiGVFreehandAreaSelectionTool::mouseReleaseCB );
}


uiGVFreehandAreaSelectionTool::~uiGVFreehandAreaSelectionTool()
{
    detachAllNotifiers();
}


void uiGVFreehandAreaSelectionTool::enable()
{
    enabled_ = true;
}


void uiGVFreehandAreaSelectionTool::disable()
{
    enabled_ = false;
}


void uiGVFreehandAreaSelectionTool::setPolygonMode( bool ispoly )
{
    ispolygonmode_ = ispoly;
}


bool uiGVFreehandAreaSelectionTool::isPolygonMode() const
{
    return ispolygonmode_;
}


void uiGVFreehandAreaSelectionTool::mousePressCB( CallBacker* )
{
    //TODO
}


void uiGVFreehandAreaSelectionTool::mouseReleaseCB( CallBacker* )
{
    //TODO
}


void uiGVFreehandAreaSelectionTool::mouseMoveCB( CallBacker* )
{
    //TODO
}
