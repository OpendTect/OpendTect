/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horizon2dline.cc,v 1.14 2009-07-22 16:01:33 cvsbert Exp $";

#include "horizon2dline.h"

#include "posinfo.h"
#include "undefval.h"
#include <limits.h>

namespace Geometry
{

Horizon2DLine::Horizon2DLine()
    : firstrow_(0)
{}


Horizon2DLine::Horizon2DLine( const TypeSet<Coord>& path, int start, int step )
    : firstrow_(0)
{
    addRow( path, start, step );
}


Horizon2DLine::Horizon2DLine( const Horizon2DLine& ln )
    : firstrow_(ln.firstrow_)
    , colsampling_(ln.colsampling_)
{
    deepCopy( rows_, ln.rows_ );
}


Horizon2DLine::~Horizon2DLine()
{
    deepErase( rows_ );
}


Horizon2DLine* Horizon2DLine::clone() const
{ return new Horizon2DLine(*this); }


int Horizon2DLine::addRow( const TypeSet<Coord>& coords, int start, int step )
{
    const int res = rows_.size() + firstrow_;
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


void Horizon2DLine::syncRow( int rowid ,const PosInfo::Line2DData& geom )
{
    const int rowidx = rowid - firstrow_;
    if ( rowidx<0 || rowidx>=rows_.size() )
	return;

    for ( int colidx=rows_[rowidx]->size()-1; colidx>=0; colidx-- )
    {
	const double z = (*rows_[rowidx])[colidx].z;
	(*rows_[rowidx])[colidx] = Coord3( Coord::udf(), z );
    }

    const int nrtraces = geom.posns_.size();
    for ( int tridx=geom.posns_.size()-1; tridx>=0; tridx-- )
    {
	int colidx = colsampling_[rowidx].nearestIndex( geom.posns_[tridx].nr_ );
	if ( colsampling_[rowidx].atIndex(colidx) != geom.posns_[tridx].nr_ )
	    continue;
	
	if ( !rows_[rowidx]->size() )
	{
	    *rows_[rowidx] += Coord3::udf();
	    colsampling_[rowidx].start = geom.posns_[tridx].nr_;
	    colidx = 0;
	}

	while ( colidx<0 )
	{
	    rows_[rowidx]->insert( 0, Coord3::udf() );
	    colsampling_[rowidx].start -= colsampling_[rowidx].step;
	    colidx++;
	}

	int udfstoadd = colidx + 1 - rows_[rowidx]->size();
	while ( udfstoadd > 0 )
	{
	    *rows_[rowidx] += Coord3::udf();
	    udfstoadd--;
	}

	const double z = (*rows_[rowidx])[colidx].z;
	(*rows_[rowidx])[colidx] = Coord3( geom.posns_[tridx].coord_, z );
    }

    for ( int colidx=rows_[rowidx]->size()-1; colidx>=0; colidx-- )
    {
	if ( Coord((*rows_[rowidx])[colidx]).isDefined() )
	    break;
	
	rows_[rowidx]->remove( colidx );
    }

    while ( rows_[rowidx]->size() && !Coord((*rows_[rowidx])[0]).isDefined() )
    {
	rows_[rowidx]->remove( 0 );
	colsampling_[rowidx].start += colsampling_[rowidx].step;
    }

    triggerNrPosCh();
}

	
void Horizon2DLine::removeRow( int id )
{
    const int rowidx = id-firstrow_;
    if ( rowidx<0 )
	return;

    delete rows_[rowidx];
    rows_.remove( rowidx, false );
    colsampling_.remove( rowidx, false );
    if ( !rowidx )
	firstrow_++;
    else
    {
	//TODO notify
    }
}


void Horizon2DLine::removeCols( int rowid, int col1, int col2 )
{
    const int rowidx = rowid - firstrow_;
    if ( rowidx<0 ) return;

    const StepInterval<int> colrg = colRange( rowid );
    const int startidx = colrg.getIndex( col1 );
    const int stopidx = colrg.getIndex( col2 );

    if ( colrg.start == col1 )
    {
	rows_[rowidx]->remove( startidx, stopidx );
	colsampling_[rowidx].start = col2 + colrg.step;
    }
    else if ( colrg.stop == col2 )
    {
	rows_[rowidx]->remove( startidx, stopidx );
    }
    else
    {
	for ( int idx=startidx; idx<=stopidx; idx++ )
	    (*rows_[rowidx])[idx] = Coord3::udf();
    }
}


void Horizon2DLine::setRow( int rowid, const TypeSet<Coord>& path,
			    int start, int step )
{
    const int rowidx = rowid - firstrow_;
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
	colsampling_[rowidx].start = start;
	colsampling_[rowidx].step = step;
	triggerNrPosCh();
    }
}


StepInterval<int> Horizon2DLine::rowRange() const
{ return StepInterval<int>( firstrow_, firstrow_+rows_.size()-1, 1 ); }


StepInterval<int> Horizon2DLine::colRange( int rowid ) const
{
    const int rowidx = rowid - firstrow_;
    if ( rowidx<0 )
	return StepInterval<int>( INT_MAX, INT_MIN, 1 );

    const SamplingData<int>& sd = colsampling_[rowidx];
    return StepInterval<int>( sd.start, sd.atIndex(rows_[rowidx]->size()-1),
	    		      sd.step );
}


Interval<float> Horizon2DLine::zRange( int rowid ) const
{
    Interval<float> zrange( mUdf(float), -mUdf(float) );
    StepInterval<int> colrg = colRange( rowid );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	const float z = getKnot( RowCol(rowid,col) ).z;
	if ( !mIsUdf(z) )
	    zrange.include( z, false );
    }

    return zrange;
}


void Horizon2DLine::geometry( int rowid, PosInfo::Line2DData& ld ) const
{
    ld.posns_.erase();
    const int rowidx = rowid - firstrow_;
    if ( !rows_.validIdx(rowidx) )
	return;

    assign( ld.zrg_, zRange(rowid) );
    for ( int idx=0; idx<rows_[rowidx]->size(); idx++ )
    {
	PosInfo::Line2DPos pos;
	pos.nr_ = colsampling_[rowidx].atIndex( idx );
	pos.coord_ = (*rows_[rowidx])[idx];
	ld.posns_ += pos;
    }
}


Coord3 Horizon2DLine::getKnot( const RCol& rc ) const
{
    const int rowidx = rc.r() - firstrow_;
    const int colidx = colIndex( rowidx, rc.c() );
    return colidx>=0 ? (*rows_[rowidx])[colidx] : Coord3::udf();
}


bool Horizon2DLine::setKnot( const RCol& rc, const Coord3& pos )
{
    const int rowidx = rc.r() - firstrow_;
    const int colidx = colIndex( rowidx, rc.c() );

    if ( colidx<0 ) return false;

    (*rows_[rowidx])[colidx] = pos;

    return true;
}


bool Horizon2DLine::isKnotDefined( const RCol& rc ) const
{
    const int rowidx = rc.r() - firstrow_;
    const int colidx = colIndex( rowidx, rc.c() );
    return colidx>=0 ? (*rows_[rowidx])[colidx].isDefined() : false;
}


int Horizon2DLine::colIndex( int rowidx, int colid ) const
{
    if ( rowidx<0 || rowidx>=colsampling_.size() )
	return -1;
    const SamplingData<int>& sd = colsampling_[rowidx];
    int idx = colid-sd.start;
    if ( idx<0 || !sd.step || idx%sd.step )
	return -1;

    idx /= sd.step;
    if ( idx>=rows_[rowidx]->size() )
	return -1;

    return idx;
}


bool Horizon2DLine::hasSupport( const RCol& rc ) const
{
    StepInterval<int> colrg = colRange( rc.r() );
    const RowCol prev( rc.r(), rc.c()-colrg.step );
    const RowCol next( rc.r(), rc.c()+colrg.step );
    return isKnotDefined(prev) || isKnotDefined(next);
}


void Horizon2DLine::trimUndefParts()
{
    StepInterval<int> rowrg = rowRange();
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	StepInterval<int> colrg = colRange( row );
	bool founddefknot = false;
	for ( int col=colrg.start; col<colrg.stop; col+=colrg.step )
	{
	    if ( isKnotDefined(RowCol(row,col)) )
		founddefknot = true;
	    else
		continue;

	    if ( founddefknot && col!=colrg.start )
		removeCols( row, colrg.start, col-colrg.step );

	    break;
	}

	founddefknot = false;
	for ( int col=colrg.stop; col>=colrg.start; col-=colrg.step )
	{
	    if ( isKnotDefined(RowCol(row,col)) )
		founddefknot = true;
	    else
		continue;

	    if ( founddefknot && col!=colrg.stop )
		removeCols( row, col+colrg.step, colrg.stop );

	    break;
	}

    }
}


} // namespace Geometry
