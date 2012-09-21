/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2009
 RCS:           $Id$
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uidatapointsetcrossplot.h"

#include "polygon.h"
#include "keystrs.h"
#include "trigonometry.h"

static const char* sKeyNrAreas = "Nr of Selection Areas";
static const char* sKeyRect = "Rectangle";
static const char* sKeyPoly = "Polygon";
static const char* sKeyPos = "Position";


uiDataPointSetCrossPlotter::Setup::Setup()
    : noedit_(false)
    , markerstyle_(MarkerStyle2D::Square)
    , xstyle_(LineStyle::Solid,1,Color::Black())
    , ystyle_(LineStyle::Solid,1,Color::stdDrawColor(0))
    , y2style_(LineStyle::Dot,1,Color::stdDrawColor(1))
    , minborder_(10,20,20,5)
    , showcc_(true)
    , showregrline_(false)
    , showy1userdefpolyline_(false)
    , showy2userdefpolyline_(false)
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
    , maxdistest_(mUdf(double))
    , axistype_(SelectionArea::Y1)
{
}


SelectionArea::SelectionArea( const ODPolygon<int>& poly )
    : isrectangle_( false )
    , poly_(poly)
    , maxdistest_(mUdf(double))
    , axistype_(SelectionArea::Y1)
{
}


SelectionArea::SelectionArea( bool isrect )
    : isrectangle_( isrect )
    , maxdistest_(mUdf(double))
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


float SelectionArea::selectedness( uiPoint pt ) const
{
    if ( !isInside(pt) )
	return mUdf(float);
 
    if ( mIsUdf(maxdistest_) )
	maxdistest_ = maxDisToBorder();

    const double distobrder = minDisToBorder( pt );
    if ( mIsEqual(distobrder,maxdistest_,0.15) )
	return 1.00;

    if ( distobrder > maxdistest_ )
	return mUdf(float);

    return (float) ( distobrder/maxdistest_ );
}


double SelectionArea::maxDisToBorder() const
{
    if ( isrectangle_ )
	return rect_.width() > rect_.height()
			    ? ( (double)rect_.width()/(double)2 )
			    : ( (double)rect_.height()/(double)2 );
    return poly_.maxDistToBorderEstimate(mDefEps);
}


double SelectionArea::minDisToBorder( uiPoint pt ) const
{
    if ( !isInside(pt) )
	return mUdf(double);

    if ( isrectangle_ )
    {
	const int min_dist_vert_lines =
	    (pt.x-rect_.left()) < (rect_.right()-pt.x) ? pt.x-rect_.left()
	    					       : rect_.right()-pt.x;
	const int min_dist_hor_lines =
	    (pt.y-rect_.top()) < (rect_.bottom()-pt.y) ? pt.y-rect_.top()
	    					       : rect_.bottom()-pt.y;
	return min_dist_vert_lines > min_dist_hor_lines ? min_dist_hor_lines
	    						: min_dist_vert_lines;
    }

    return poly_.distTo( pt );
}


bool SelectionArea::isInside( const uiPoint& pos ) const
{
    if ( !isrectangle_ )
    {
	if ( !poly_.getRange(true).includes(pos.x,true) ||
	     !poly_.getRange(false).includes(pos.y,true) )
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


void SelectionGrp::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name().buf() );
    BufferString color;
    col_.fill( color.buf() );
    par.set( sKey::Color(), color.buf() );
    par.set( sKeyNrAreas, selareas_.size() );

    for ( int selidx=0; selidx < selareas_.size(); selidx++ )
    {
	const SelectionArea& selarea = selareas_[selidx];
	BufferString selkey;
	BufferStringSet attributes;
	selkey.add( selidx );
	
	attributes.add( selarea.xaxisnm_ );
	attributes.add(  selarea.yaxisnm_ );
	if ( selarea.axistype_ == SelectionArea::Both )
	    attributes.add( selarea.altyaxisnm_ );
	par.set( IOPar::compKey(selkey,sKey::Attributes()), attributes );
	
	if ( selarea.isrectangle_ )
	{
	    uiWorldRect rect = selarea.worldrect_;
	    BufferString key = IOPar::compKey( selkey, sKeyRect );
	    BufferString posltstr( IOPar::compKey(sKeyPos,0) );
	    BufferString posrbstr( IOPar::compKey(sKeyPos,1) );
	    if ( selarea.axistype_ == SelectionArea::Both )
	    {
		uiWorldRect altrect = selarea.altworldrect_;
		par.set( IOPar::compKey(key,posltstr), rect.left(),
			 rect.top(), altrect.top() );
		par.set( IOPar::compKey(key,posrbstr), rect.right(),
			 rect.bottom(), altrect.bottom() );
	    }
	    else
	    {
		par.set( IOPar::compKey(key,posltstr), rect.left(), rect.top());
		par.set( IOPar::compKey(key,posrbstr), rect.right(),
			 rect.bottom() );
	    }
	}
	else
	{
	    ODPolygon<double> poly = selarea.worldpoly_;
	    ODPolygon<double> altpoly = selarea.altworldpoly_;
	    const TypeSet< Geom::Point2D<double> > pts = poly.data();
	    const TypeSet< Geom::Point2D<double> > altpts = altpoly.data();

	    const bool hasaltaxis = selarea.axistype_ == SelectionArea::Both;
	    for ( int posidx=0; posidx < poly.size(); posidx++ )
	    {
		BufferString polygonstr( IOPar::compKey(selkey,sKeyPoly) );
		BufferString positionstr( IOPar::compKey(sKeyPos,posidx) );

		if ( hasaltaxis )
		    par.set( IOPar::compKey(polygonstr,positionstr),
			     pts[posidx].x, pts[posidx].y, altpts[posidx].y );
		else
		    par.set( IOPar::compKey(polygonstr,positionstr),
			     pts[posidx].x, pts[posidx].y);
	    }

	}
    }
}

void SelectionGrp::usePar( const IOPar& par )
{
    if ( !par.get(sKey::Name(),*name_) || !par.get(sKey::Color(),col_) )
	return;

    int nrselareas = 0;
    par.get( sKeyNrAreas, nrselareas );

    for ( int selidx=0; selidx<nrselareas; selidx++ )
    {
	BufferString selkey;
	selkey.add( selidx );
	BufferStringSet nms;
	par.get( IOPar::compKey(selkey,sKey::Attributes()), nms );

	BufferString rectstr = IOPar::compKey( selkey.str(), sKeyRect );
	BufferString polygonstr = IOPar::compKey( selkey.str(), sKeyPoly );
	    
	int posidx = 0;
	BufferString positionstr = IOPar::compKey( sKeyPos, posidx );

	if ( par.find( IOPar::compKey(rectstr,positionstr)) )
	{
	    TypeSet<float> ptslt, ptsrb;

	    par.get( IOPar::compKey(rectstr,positionstr), ptslt );
	    posidx++;
	    positionstr = IOPar::compKey( sKeyPos, posidx );

	    par.get( IOPar::compKey(rectstr,positionstr), ptsrb );
	    uiWorldRect rect( ptslt[0], ptslt[1], ptsrb[0], ptsrb[1] );
	    
	    SelectionArea selarea( true );
	    selarea.id_ = selidx;
	    selarea.worldrect_ = rect;
	    selarea.xaxisnm_ = nms.get( 0 ); selarea.yaxisnm_ = nms.get( 1 );
	    if ( (ptslt.size()==3) && (ptsrb.size()==3) )
	    {
		selarea.altyaxisnm_ = nms.get( 2 );
		selarea.altworldrect_ =
		    uiWorldRect( ptslt[0], ptslt[2], ptsrb[0], ptsrb[2] );
	    }

	    selareas_ += selarea;
	}
	else if ( par.find(IOPar::compKey(polygonstr,positionstr)) )
	{
	    ODPolygon<double> worldpoly, altworldpoly;
	    
	    bool hasalt = false;
	    while ( par.find(IOPar::compKey(polygonstr.buf(),
			    		    positionstr.buf())) )
	    {
		TypeSet<double> pt;
		par.get( IOPar::compKey(polygonstr,positionstr), pt );
		if ( pt.size() == 3 )
		{
		    hasalt = true;
		    altworldpoly.add( Geom::Point2D<double>(pt[0],pt[2]) );
		}
		worldpoly.add( Geom::Point2D<double>(pt[0],pt[1]) );
		posidx++;
		positionstr = IOPar::compKey( sKeyPos, posidx );
	    }

	    SelectionArea selarea( false );
	    selarea.id_ = selidx;
	    selarea.worldpoly_ = worldpoly;
	    selarea.xaxisnm_ = nms.get( 0 ); selarea.yaxisnm_ = nms.get( 1 );
	    if ( hasalt )
	    {
		selarea.altyaxisnm_ = nms.get( 2 );
		selarea.altworldpoly_ = altworldpoly;
	    }
	    selareas_ += selarea;
	}
    }
}


void SelectionGrp::addSelection( const SelectionArea& selarea )
{
    selareas_ += selarea;
}


void SelectionGrp::removeSelection( int idx )
{
    if ( selareas_.validIdx(idx) )
	selareas_.remove( idx );
}


void SelectionGrp::removeAll()
{
    selareas_.erase();
}


SelectionArea::SelAxisType SelectionGrp::getSelectionAxis( int selareaid ) const
{
    SelectionArea selarea;
    getSelectionArea( selarea, selareaid );
    return selarea.axistype_;
}


bool SelectionGrp::isValidIdx( int idx ) const
{
    return selareas_.validIdx(idx);
}


int SelectionGrp::validIdx( int selareaid ) const
{
    for ( int idx=0; idx < selareas_.size(); idx++ )
    {
	if ( selareas_[idx].id_ == selareaid )
	    return idx;
    }

    return -1;
}



bool SelectionGrp::getSelectionArea( SelectionArea& selarea, int id ) const
{
    for ( int selidx=0; selidx<selareas_.size(); selidx++ )
    {
	if ( selareas_[selidx].id_ == id )
	{
	    selarea = selareas_[selidx];
	    return true;
	}
    }

    return false;
}


SelectionArea& SelectionGrp::getSelectionArea( int idx )
{
    return selareas_[idx];
}


const SelectionArea& SelectionGrp::getSelectionArea( int idx ) const
{
    return selareas_[idx];
}


void SelectionGrp::setSelectionArea( const SelectionArea& selarea )
{
    for ( int idx=0; idx < selareas_.size(); idx++ )
    {
	if ( selareas_[idx].id_ == selarea.id_ )
	    selareas_[idx] = selarea;
    }
}


bool SelectionGrp::hasAltAxis() const
{
    for ( int idx=0; idx < selareas_.size(); idx++ )
    {
	if ( selareas_[idx].axistype_ == SelectionArea::Both )
	    return true;
    }

    return false;
}


int SelectionGrp::size() const
{ return selareas_.size(); }


int SelectionGrp::isInside( const uiPoint& pt ) const
{
    for ( int idx=0; idx<selareas_.size(); idx++ )
    {
	if ( selareas_[idx].isInside(pt) )
	    return selareas_[idx].id_;
    }

    return -1;
}


void SelectionGrp::getInfo( BufferString& info ) const
{
    info += "Selection Group Name :";
    info += name();
    info += "\n";

    Interval<double> range( mUdf(double), -mUdf(double) );

    for ( int idx=0; idx<selareas_.size(); idx++ )
    {
	const SelectionArea& selarea = getSelectionArea( idx );
	BufferStringSet axisnms = selarea.getAxisNames();

	info += "Area Nr "; info.add( idx+1 ); info += "\n";
	info += "Area Type : ";
	info += selarea.isrectangle_ ? "Rectangle \n" : "Polygon \n";

	info += selarea.xaxisnm_; info += " (range) :";
	range = selarea.getValueRange( true );
	info .add( range.start ); info += ", "; info.add( range.stop );
	info += "\n";

	info += selarea.yaxisnm_; info += " (range) :";
	range = selarea.getValueRange(false);
	info .add( range.start ); info += ", "; info.add( range.stop );
	info += "\n";

	if ( !selarea.altyaxisnm_.isEmpty() )
	{
	    info += selarea.altyaxisnm_; info += " (range) :";
	    range = selarea.getValueRange( false, true );
	    info .add( range.start ); info += ", "; info.add( range.stop);
	    info += "\n";
	}

	info += "\n";
    }
}
