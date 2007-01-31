/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID = "$Id: horizon2dline.cc,v 1.4 2007-01-31 11:54:28 cvsjaap Exp $";

#include "horizon2dline.h"
#include "undefval.h"

namespace Geometry
{

Horizon2DLine::Horizon2DLine()
    : firstrow_( 0 )
{}


Horizon2DLine::Horizon2DLine( const TypeSet<Coord>& path, int start, int step )
    : firstrow_( 0 )
{
    addRow( path, start, step );
}


Horizon2DLine::Horizon2DLine( const Horizon2DLine& ln )
    : firstrow_( ln.firstrow_ )
    , colsampling_( ln.colsampling_ )
{
    deepCopy( rows_, ln.rows_ );
}


Horizon2DLine::~Horizon2DLine()
{
    deepErase( rows_ );
}


Horizon2DLine* Horizon2DLine::clone() const
{ return new Horizon2DLine( *this ); }


int Horizon2DLine::addRow(const TypeSet<Coord>& coords, int start, int step )
{
    const int res = rows_.size()+firstrow_;
    rows_ += new TypeSet<Coord3>;
    colsampling_ += SamplingData<int>( start, step );
    setRow( res, coords, start, step );
    return res;
}


int Horizon2DLine::addUdfRow( int start, int stop, int step )
{
    TypeSet<Coord> coords;
    for ( int idx=start; idx<=stop; idx+=step )
	coords += Coord::udf();

    return addRow( coords, start, step );
}


void Horizon2DLine::removeRow( int id )
{
    const int rowidx = id-firstrow_;
    if ( rowidx<0 )
	return;

    delete rows_[rowidx];
    rows_.removeFast( rowidx );
    colsampling_.removeFast( rowidx );
    if ( !rowidx )
	firstrow_++;
    else
    {
	//TODO notify
    }
}


void Horizon2DLine::setRow( int id,
		const TypeSet<Coord>& path, int start, int step )
{
    const int rowidx = id-firstrow_;
    if ( rowidx<0 )
	return;

    const bool samedimensions =
	path.size()==rows_[rowidx]->size() &&
	start==colsampling_[rowidx].start && step==colsampling_[rowidx].step;

    rows_[rowidx]->setSize( path.size(), Coord3::udf() );

    for ( int idx=path.size()-1; idx>=0; idx-- )
	(*rows_[rowidx])[idx] = Coord3( path[idx], mUdf(float) );

    if ( samedimensions )
	triggerMovement();
    else
    {
	colsampling_[rowidx].start = start; colsampling_[rowidx].step = step;
	triggerNrPosCh();
    }
}


StepInterval<int> Horizon2DLine::rowRange() const
{ return StepInterval<int>( firstrow_, firstrow_+rows_.size()-1, 1 ); }


StepInterval<int> Horizon2DLine::colRange( int rowid ) const
{
    const int rowidx = rowid-firstrow_;
    if ( rowidx<0 )
	return StepInterval<int>( INT_MAX, INT_MIN, 1 );

    const SamplingData<int>& sd = colsampling_[rowidx];
    return StepInterval<int>( sd.start, sd.atIndex(rows_[rowidx]->size()-1),
	    		      sd.step );
}
    

Coord3 Horizon2DLine::getKnot( const RCol& rc ) const
{
    const int rowidx = rc.r()-firstrow_;
    const int colidx = colIndex( rowidx, rc.c() );
    return colidx>=0 ? (*rows_[rowidx])[colidx] : Coord3::udf();
}


bool Horizon2DLine::setKnot( const RCol& rc, const Coord3& pos )
{
    const int rowidx = rc.r()-firstrow_;
    const int colidx = colIndex( rowidx, rc.c() );

    if ( colidx<0 ) return false;

    (*rows_[rowidx])[colidx] = pos;

    return true;
}


bool Horizon2DLine::isKnotDefined( const RCol& rc ) const
{
    const int rowidx = rc.r()-firstrow_;
    const int colidx = colIndex( rowidx, rc.c() );
    return colidx>=0 ? (*rows_[rowidx])[colidx].isDefined() : false;
}


int Horizon2DLine::colIndex( int rowidx, int colid ) const
{
    if ( rowidx >= colsampling_.size() )
	return -1;
    const SamplingData<int>& sd = colsampling_[rowidx];
    int idx = colid-sd.start;
    if ( idx<0 || idx%sd.step )
	return -1;

    idx /= sd.step;
    if ( idx>=rows_[rowidx]->size() )
	return -1;

    return idx;
}


}; //Namespace

