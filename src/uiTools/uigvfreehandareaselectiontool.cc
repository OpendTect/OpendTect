/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigvfreehandareaselectiontool.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"

#include "mouseevent.h"
#include "polygon.h"


uiGVFreehandAreaSelectionTool::uiGVFreehandAreaSelectionTool(uiGraphicsView& gv)
    : gv_(gv)
    , ispolygonmode_(true)
    , enabled_(true)
    , polygonselitem_(0)
    , odpolygon_(*new ODPolygon<int>())
    , started(this)
    , pointAdded(this)
    , stopped(this)
{
    polygonselitem_ = gv_.scene().addItem( new uiPolygonItem() );
    polygonselitem_->setPenColor( OD::Color(255,0,0) );

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
    delete &odpolygon_;
}


void uiGVFreehandAreaSelectionTool::enable()
{
    enabled_ = true;
    polygonselitem_->setVisible( enabled_ );
}


void uiGVFreehandAreaSelectionTool::disable()
{
    enabled_ = false;
    polygonselitem_->setVisible( enabled_ );
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
    odpolygon_.setEmpty();
}


void uiGVFreehandAreaSelectionTool::mouseReleaseCB( CallBacker* )
{
    //TODO
}


void uiGVFreehandAreaSelectionTool::mouseMoveCB( CallBacker* )
{
    const MouseEvent& ev = gv_.getMouseEventHandler().event();
    if ( ev.leftButton() && isEnabled() )
    {
	if ( isPolygonMode() )
	{
	    const Geom::Point2D<int>& pos = ev.pos();
	    if ( odpolygon_.size() > 3 )
	    {
		if ( !odpolygon_.isInside(pos,true,0) )
		    odpolygon_.add( pos );
	    }
	    else
		odpolygon_.add( pos );
	}
	else
	{
	    if ( odpolygon_.isEmpty() )
		odpolygon_.add( ev.pos() );

	    const Geom::Point2D<int>& startpos = odpolygon_.getVertex( 0 );
	    odpolygon_.setEmpty();
	    odpolygon_.add( startpos );
	    odpolygon_.add( Geom::Point2D<int>(startpos.x,ev.pos().y) );
	    odpolygon_.add( ev.pos() );
	    odpolygon_.add( Geom::Point2D<int>(ev.pos().x,startpos.y) );
	    odpolygon_.add( startpos );
	}
    }
   
    polygonselitem_->setPolygon( odpolygon_ );
}
