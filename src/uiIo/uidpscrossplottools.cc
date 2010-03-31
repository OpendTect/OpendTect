/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2009
 RCS:           $Id: uidpscrossplottools.cc,v 1.2 2010-03-31 06:45:24 cvssatyaki Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidpscrossplottools.cc,v 1.2 2010-03-31 06:45:24 cvssatyaki Exp $";

#include "uidatapointsetcrossplot.h"

#include "polygon.h"

uiDataPointSetCrossPlotter::Setup::Setup()
    : noedit_(false)
    , markerstyle_(MarkerStyle2D::Square)
    , xstyle_(LineStyle::Solid,1,Color::Black())
    , ystyle_(LineStyle::Solid,1,Color::stdDrawColor(0))
    , y2style_(LineStyle::Dot,1,Color::stdDrawColor(1))
    , minborder_(10,20,20,5)
    , showcc_(true)
    , showregrline_(false)
    , showy1userdefline_(false)
    , showy2userdefline_(false)
{
}


uiDataPointSetCrossPlotter::AxisData::AxisData( uiDataPointSetCrossPlotter& cp,
						uiRect::Side s )
    : uiAxisData( s )
    , cp_( cp )
{
}


void uiDataPointSetCrossPlotter::AxisData::stop()
{
    if ( isreset_ ) return;
    colid_ = cp_.mincolid_ - 1;
    uiAxisData::stop();
}


void uiDataPointSetCrossPlotter::AxisData::setCol( DataPointSet::ColID cid )
{
    if ( axis_ && cid == colid_ )
	return;

    stop();
    colid_ = cid;
    newColID();
}


void uiDataPointSetCrossPlotter::AxisData::newColID()
{
    if ( colid_ < cp_.mincolid_ )
	return;

    renewAxis( cp_.uidps_.userName(colid_), &cp_.scene(), cp_.width(),
	       cp_.height(), 0 );
    handleAutoScale( cp_.uidps_.getRunCalc( colid_ ) );
}


uiDataPointSetCrossPlotter::SelectionArea::SelectionArea( uiRect* rect )
    : type_( uiDataPointSetCrossPlotter::SelectionArea::Rectangle )
    , rect_(rect)
    , poly_(0)
{
}


uiDataPointSetCrossPlotter::SelectionArea::SelectionArea( ODPolygon<int>* poly )
    : type_( uiDataPointSetCrossPlotter::SelectionArea::Polygon )
    , rect_(0)
    , poly_(poly)
{
}


uiDataPointSetCrossPlotter::SelectionArea::~SelectionArea()
{
    delete rect_;
    delete poly_;
}

bool uiDataPointSetCrossPlotter::SelectionArea::operator==(
	const uiDataPointSetCrossPlotter::SelectionArea& selarea ) const
{
    if ( type_ != selarea.type_ ) return false;
    if ( type_ == uiDataPointSetCrossPlotter::SelectionArea::Rectangle
	 && (*worldrect_ != *selarea.worldrect_) )
	return false;
    if ( type_ == uiDataPointSetCrossPlotter::SelectionArea::Polygon
	 && !(worldpoly_->data() == selarea.worldpoly_->data()) )
	return false;

    return true;
}


bool uiDataPointSetCrossPlotter::SelectionArea::isInside(
	const uiPoint& pos ) const
{
    if ( type_ == SelectionArea::Polygon )
    {
	if ( !poly_->getRange(true).includes(pos.x) ||
	     !poly_->getRange(false).includes(pos.y) )
	    return false;
    }
    return type_ == Polygon ? poly_->isInside(pos,true,0)
			    : rect_->contains( pos );
}


bool uiDataPointSetCrossPlotter::SelectionArea::isValid() const
{
    return type_ == Polygon ? true
			    : (rect_->width()>1 && rect_->height()>1);
}

