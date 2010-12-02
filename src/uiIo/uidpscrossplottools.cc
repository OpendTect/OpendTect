/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2009
 RCS:           $Id: uidpscrossplottools.cc,v 1.3 2010-12-02 10:07:52 cvssatyaki Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidpscrossplottools.cc,v 1.3 2010-12-02 10:07:52 cvssatyaki Exp $";

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


SelectionArea::SelectionArea( const uiRect& rect )
    : isrectangle_( true )
    , rect_(rect)
    , axistype_(SelectionArea::Y1)
{
}


SelectionArea::SelectionArea( const ODPolygon<int>& poly )
    : isrectangle_( false )
    , poly_(poly)
    , axistype_(SelectionArea::Y1)
{
}


SelectionArea::SelectionArea( bool isrect )
    : isrectangle_( isrect )
    , axistype_(SelectionArea::Y1)
{
}


SelectionArea::~SelectionArea()
{
}

bool SelectionArea::operator==( const SelectionArea& selarea ) const
{
    if ( isrectangle_ != selarea.isrectangle_ ) return false;
    if ( isrectangle_ && (worldrect_ != selarea.worldrect_) )
	return false;
    else if ( !isrectangle_ && !(worldpoly_.data()==selarea.worldpoly_.data()) )
	return false;

    return true;
}


bool SelectionArea::isInside( const uiPoint& pos ) const
{
    if ( !isrectangle_ )
    {
	if ( !poly_.getRange(true).includes(pos.x) ||
	     !poly_.getRange(false).includes(pos.y) )
	    return false;
    }

    return !isrectangle_ ? poly_.isInside(pos,true,0) : rect_.contains( pos );
}


bool SelectionArea::isValid() const
{
    return !isrectangle_ ? true : (rect_.width()>1 && rect_.height()>1);
}

Interval<double> SelectionArea::getValueRange( bool forxaxis, bool alt ) const
{
    Interval<double> intv;
    if ( isrectangle_ )
    {
	if ( forxaxis )
	{
	    intv.start = worldrect_.left();
	    intv.stop = worldrect_.right();
	}
	else
	{
	    intv.start = worldrect_.bottom();
	    intv.stop = worldrect_.top();
	    
	    if ( alt )
	    {
		intv.start = altworldrect_.bottom();
		intv.stop = altworldrect_.top();
	    }
	}
    }
    else
    {
	intv = worldpoly_.getRange( forxaxis );
	if ( alt )
	    intv = altworldpoly_.getRange( forxaxis );
    }

    return intv;
}

BufferStringSet SelectionArea::getAxisNames() const
{
    BufferStringSet selaxisnm;
    selaxisnm.add( xaxisnm_ );
    selaxisnm.add( yaxisnm_ );
    if ( !altyaxisnm_.isEmpty() )
	selaxisnm.add( altyaxisnm_ );
    return selaxisnm;
}
