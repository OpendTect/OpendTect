/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jun 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "grid2d.h"

#include "horsampling.h"
#include "iopar.h"
#include "survinfo.h"
#include "trigonometry.h"


static BinID getNearestPoint( const BinID& pos, const TypeSet<Coord>& points )
{
    int nearestidx = -1;
    double mindist = mUdf(double);
    for ( int idx=0; idx<points.size(); idx++ )
    {
	const double dist = ( SI().transform(pos) ).distTo( points[idx] );
	if ( dist < mindist )
	{
	    mindist = dist;
	    nearestidx = idx;
	}
    }

    return nearestidx < 0 ? BinID::udf() : SI().transform( points[nearestidx] );
}


Grid2D::Line::Line( const BinID& start, const BinID& stop )
    : start_(start),stop_(stop)
{}


bool Grid2D::Line::isReasonable() const
{
    return SI().isReasonable( start_ ) && SI().isReasonable( stop_ );
}


void Grid2D::Line::limitTo( const HorSampling& hs )
{
    const bool startin = hs.includes( start_ );
    const bool stopin = hs.includes( stop_ );
    if ( startin && stopin )
	return;

    Coord svert[4];
    svert[0] = SI().transform( hs.start );
    svert[1] = SI().transform( BinID(hs.start.inl,hs.stop.crl) );
    svert[2] = SI().transform( hs.stop );
    svert[3] = SI().transform( BinID(hs.stop.inl,hs.start.crl) );

    Line2 line( SI().transform(start_), SI().transform(stop_) );
    TypeSet<Coord> points;
    for ( int idx=0; idx<4; idx++ )
    {
	Line2 bound( svert[idx], idx < 3 ? svert[idx+1] : svert[0] );
	Coord pt = line.intersection( bound );
	if ( !mIsUdf(pt.x) && !mIsUdf(pt.y) )
	    points += pt;
    }

    if ( !points.size() )
	start_ = stop_ = BinID::udf();
    else
    {
	if ( !startin )
	    start_ = getNearestPoint( start_, points );
	if ( !stopin )
	    stop_ = getNearestPoint( stop_, points );
    }
}


// Grid2D
void Grid2D::set( const TypeSet<int>& inls, const TypeSet<int>& crls,
		      const HorSampling& hs )
{
    empty();
    for ( int idx=0; idx<inls.size(); idx++ )
    {
	const BinID start( inls[idx], hs.start.crl );
	const BinID stop( inls[idx], hs.stop.crl );
	dim0lines_ += new Grid2D::Line( start, stop );
    }

    for ( int idx=0; idx<crls.size(); idx++ )
    {
	const BinID start( hs.start.inl, crls[idx] );
	const BinID stop( hs.stop.inl, crls[idx] );
	dim1lines_ += new Grid2D::Line( start, stop );
    }
}


void Grid2D::set( const Grid2D::Line& baseln, double pardist, double perpdist,
		      const HorSampling& hs )
{
    empty();
    const Coord startpt = SI().transform( baseln.start_ );
    const Coord stoppt = SI().transform( baseln.stop_ );
    Line2 rline( startpt, stoppt );
    rline.start_ = Coord::udf();
    rline.stop_ = Coord::udf();
    createParallelLines( rline, pardist, hs, dim0lines_ );
    const Coord centrpt = ( startpt + stoppt ) / 2;
    Line2 rlineperp( 0, 0 );
    rline.getPerpendicularLine( rlineperp, centrpt );
    createParallelLines( rlineperp, perpdist, hs, dim1lines_ );
}


Grid2D::~Grid2D()
{
    empty();
}


void Grid2D::empty()
{
    deepErase( dim0lines_ );
    deepErase( dim1lines_ );
}


int Grid2D::size( bool dim ) const
{
    return dim ? dim0lines_.size() : dim1lines_.size();
}


int Grid2D::totalSize() const
{
    return dim0lines_.size() + dim1lines_.size();
}


const Grid2D::Line* Grid2D::getLine( int idx, bool dim ) const
{
    const ObjectSet<Grid2D::Line>& lines = dim ? dim0lines_ : dim1lines_;
    return lines.validIdx(idx) ? lines[idx] : 0;
}


void Grid2D::createParallelLines( const Line2& baseline, double dist,
				      const HorSampling& hs,
				      ObjectSet<Grid2D::Line>& lines )
{
    if ( hs.nrInl() == 1 || hs.nrCrl() == 1 )
	return;

    Coord svert[4];
    svert[0] = SI().transform( hs.start );
    svert[1] = SI().transform( BinID(hs.start.inl,hs.stop.crl) );
    svert[2] = SI().transform( hs.stop );
    svert[3] = SI().transform( BinID(hs.stop.inl,hs.start.crl) );

    Line2 sbound[4];			// Survey boundaries
    for ( int idx=0; idx<4; idx++ )
	sbound[idx] = Line2( svert[idx], idx < 3 ? svert[idx+1] : svert[0] );

    bool posfinished = false, negfinished = false;
    for ( int idx=0; idx<100; idx++ )
    {
	if ( posfinished && negfinished )
	    break;

	Line2 posline( 0, 0 );
	Line2 negline( 0, 0 );
	if ( !idx )
	    posline = baseline;
	else
	{
	    if ( !posfinished )
		baseline.getParallelLine( posline, dist*idx );
	    if ( !negfinished )
		baseline.getParallelLine( negline, -dist*idx );
	}

	TypeSet<Coord> endsposline;
	TypeSet<Coord> endsnegline;
	for ( int bdx=0; bdx<4; bdx++ )
	{
	    if ( !posfinished )
	    {
		const Coord pos = posline.intersection( sbound[bdx] );
		if ( !mIsUdf(pos.x) && !mIsUdf(pos.y) )
		    endsposline += pos;
	    }

	    if ( idx && !negfinished )
	    {
		const Coord pos = negline.intersection( sbound[bdx] );
		if ( !mIsUdf(pos.x) && !mIsUdf(pos.y) )
		    endsnegline += pos;
	    }
	}

	if ( endsposline.size() < 2 )
	    posfinished = true;
	else
	    lines += new Grid2D::Line( SI().transform(endsposline[0]),
		    			SI().transform(endsposline[1]) );

	if ( !idx ) continue;
	if ( endsnegline.size() < 2 )
	    negfinished = true;
	else
	    lines.insertAt( new Grid2D::Line(SI().transform(endsnegline[0]),
					SI().transform(endsnegline[1])), 0 );
    }
}


#define mLimitLines(dimstr) \
    for ( int idx=0; idx<dimstr##lines_.size(); idx++ ) \
    { \
	dimstr##lines_[idx]->limitTo( cs ); \
	if ( !dimstr##lines_[idx]->isReasonable() ) \
	    delete dimstr##lines_.removeSingle( idx-- ); \
    }

void Grid2D::limitTo( const HorSampling& cs )
{
    mLimitLines(dim0)
    mLimitLines(dim1)
}


