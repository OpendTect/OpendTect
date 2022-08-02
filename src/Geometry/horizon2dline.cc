/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2006
________________________________________________________________________

-*/

#include "horizon2dline.h"

#include "interpol1d.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "undefval.h"
#include <limits.h>

namespace Geometry
{

Horizon2DLine::Horizon2DLine()
{}


Horizon2DLine::Horizon2DLine( Pos::GeomID lineid, const TypeSet<Coord>& path,
			      int start, int step )
{
    addRow( lineid, path, start, step );
}


Horizon2DLine::Horizon2DLine( const Horizon2DLine& ln )
    : colsampling_(ln.colsampling_)
    , geomids_(ln.geomids_)
{
    deepCopy( rows_, ln.rows_ );
}


Horizon2DLine::~Horizon2DLine()
{
    deepErase( rows_ );
}


Horizon2DLine* Horizon2DLine::clone() const
{
    return new Horizon2DLine(*this);
}


void Horizon2DLine::getGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    geomids = geomids_;
}


bool Horizon2DLine::hasLine( Pos::GeomID geomid ) const
{
    return geomids_.isPresent( geomid );
}


bool Horizon2DLine::addRow( Pos::GeomID geomid,const TypeSet<Coord>& coords,
			    int start, int step )
{
    const int index = getRowIndex( geomid );

    if ( mIsUdf(index) )
	return false;

    if ( index< 0 )
    {
	geomids_ += geomid;
	rows_ += new TypeSet<Coord3>;
	colsampling_ += SamplingData<int>( start, step );
    }

    setRow( geomid, coords, start, step );
    return true;
}


bool Horizon2DLine::addUdfRow( Pos::GeomID geomid, int start, int stop,
								  int step )
{
    const int nrnodes = (stop - start) / step + 1;
    TypeSet<Coord> coords( nrnodes, Coord::udf() );
    return addRow( geomid, coords, start, step );
}


bool Horizon2DLine::reassignRow( Pos::GeomID fromgeomid,
				 Pos::GeomID togeomid )
{
    const int idx = getRowIndex( fromgeomid );
    if ( idx < 0 )
	return false;

    geomids_[idx] = togeomid;
    return true;
}


void Horizon2DLine::syncRow( Pos::GeomID geomid,
			     const PosInfo::Line2DData& geom )
{
    const int rowidx = getRowIndex( geomid );
    if ( rowidx<0 || rowidx>=rows_.size() )
	return;

    for ( int colidx=rows_[rowidx]->size()-1; colidx>=0; colidx-- )
    {
	const double z = (*rows_[rowidx])[colidx].z;
	(*rows_[rowidx])[colidx] = Coord3( Coord::udf(), z );
    }

    const TypeSet<PosInfo::Line2DPos>& posns = geom.positions();
    for ( int tridx=posns.size()-1; tridx>=0; tridx-- )
    {
	int colidx = colsampling_[rowidx].nearestIndex( posns[tridx].nr_ );
	if ( colsampling_[rowidx].atIndex(colidx) != posns[tridx].nr_ )
	    continue;

	if ( !rows_[rowidx]->size() )
	{
	    *rows_[rowidx] += Coord3::udf();
	    colsampling_[rowidx].start = posns[tridx].nr_;
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
	(*rows_[rowidx])[colidx] = Coord3( geom.positions()[tridx].coord_, z );
    }

    for ( int colidx=rows_[rowidx]->size()-1; colidx>=0; colidx-- )
    {
	if ( Coord((*rows_[rowidx])[colidx]).isDefined() )
	    break;

	rows_[rowidx]->removeSingle( colidx );
    }

    while ( rows_[rowidx]->size() && !Coord((*rows_[rowidx])[0]).isDefined() )
    {
	rows_[rowidx]->removeSingle( 0 );
	colsampling_[rowidx].start += colsampling_[rowidx].step;
    }

    triggerNrPosCh();
}


void Horizon2DLine::removeRow( Pos::GeomID geomid )
{
    const int rowidx = getRowIndex( geomid );
    if ( rowidx<0 || rowidx>=rows_.size() )
	return;

    delete rows_.removeSingle( rowidx, false );
    geomids_.removeSingle( rowidx, false );
    colsampling_.removeSingle( rowidx, false );
    //TODO notify?
}


void Horizon2DLine::removeCols( Pos::GeomID geomid, int col1, int col2 )
{
    const int rowidx = getRowIndex( geomid );
    if ( rowidx<0 || rowidx>=rows_.size() ) return;

    const StepInterval<int> colrg = colRange( getRowIndex(geomid) );
    const int startidx = colrg.getIndex( col1 );
    const int stopidx = colrg.getIndex( col2 );

    if ( colrg.start == col1 )
    {
	rows_[rowidx]->removeRange( startidx, stopidx );
	colsampling_[rowidx].start = col2 + colrg.step;
    }
    else if ( colrg.stop == col2 )
    {
	rows_[rowidx]->removeRange( startidx, stopidx );
    }
    else
    {
	for ( int idx=startidx; idx<=stopidx; idx++ )
	    (*rows_[rowidx])[idx] = Coord3::udf();
    }
}


void Horizon2DLine::setRow( Pos::GeomID geomid, const StepInterval<int>* trcrg )
{
    const int rowidx = getRowIndex( geomid );
    if ( !rows_.validIdx(rowidx) )
	return;

    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
    if ( !geom2d )
	return;

    StepInterval<int> trcrg2use = trcrg ? *trcrg : geom2d->data().trcNrRange();
    rows_[rowidx]->setSize( trcrg2use.nrSteps()+1, Coord3::udf() );
    for ( int idx=0; idx<trcrg2use.nrSteps()+1; idx++ )
    {
	const Coord crd = geom2d->toCoord( trcrg2use.atIndex(idx) );
	(*rows_[rowidx])[idx] = Coord3( crd, mUdf(double) );
    }
}


void Horizon2DLine::setRow( Pos::GeomID geomid, const TypeSet<Coord>& path,
			    int start, int step )
{
    const int rowidx = getRowIndex( geomid );
    if ( rowidx<0 || rowidx>=rows_.size() )
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
{ return StepInterval<int>( 0, rows_.size()-1, 1 ); }


StepInterval<int> Horizon2DLine::colRange( int rowidx ) const
{
    if ( rowidx<0 || rowidx>=rows_.size() )
	return StepInterval<int>( INT_MAX, INT_MIN, 1 );

    const SamplingData<int>& sd = colsampling_[rowidx];
    return StepInterval<int>( sd.start, sd.atIndex(rows_[rowidx]->size()-1),
			      sd.step );
}


StepInterval<int> Horizon2DLine::colRangeForGeomID( Pos::GeomID geomid ) const
{
    return colRange( getRowIndex(geomid) );
}


Interval<float> Horizon2DLine::zRange( Pos::GeomID geomid ) const
{
    Interval<float> zrange( mUdf(float), -mUdf(float) );
    StepInterval<int> colrg = colRange( getRowIndex(geomid) );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	const int rowidx = getRowIndex( geomid );
	const float z = (float) getKnot( RowCol(rowidx,col) ).z;
	if ( !mIsUdf(z) )
	    zrange.include( z, false );
    }

    return zrange;
}


void Horizon2DLine::geometry( Pos::GeomID geomid,
			      PosInfo::Line2DData& ld ) const
{
    ld.setEmpty();
    const int rowidx = getRowIndex( geomid );
    if ( !rows_.validIdx(rowidx) )
	return;

    const Interval<float> myzrg( zRange(geomid) );
    const StepInterval<float> zrg( myzrg.start, myzrg.stop, ld.zRange().step );
    ld.setZRange( zrg );
    for ( int idx=0; idx<rows_[rowidx]->size(); idx++ )
    {
	PosInfo::Line2DPos pos;
	pos.nr_ = colsampling_[rowidx].atIndex( idx );
	pos.coord_ = (*rows_[rowidx])[idx];
	ld.add( pos );
    }
}


int Horizon2DLine::getRowIndex( Pos::GeomID geomid) const
{
    return geomids_.indexOf( geomid );
}


Coord3 Horizon2DLine::getKnot( const RowCol& rc ) const
{
    if ( mIsUdf(rc.row()) || rc.row() < 0 )
	return Coord3::udf();

    const int colidx = colIndex( rc.row(), rc.col() );
    return colidx>=0 ? (*rows_[rc.row()])[colidx] : Coord3::udf();
}


int Horizon2DLine::rowIndex( Pos::GeomID geomid ) const
{
    for ( int rowidx=0; rowidx<geomids_.size(); rowidx++ )
    {
	if ( geomids_[rowidx]==geomid )
	    return rowidx;
    }

    return -1;
}


bool Horizon2DLine::isDefined( GeomPosID pid ) const
{ return isKnotDefined( RowCol::fromInt64(pid) ); }


Coord3 Horizon2DLine::getPosition( GeomPosID pid ) const
{ return getKnot( RowCol::fromInt64(pid) ); }


bool Horizon2DLine::setPosition( GeomPosID pid, const Coord3& pos )
{ return setKnot( RowCol::fromInt64(pid), pos ); }


bool Horizon2DLine::setKnot( const RowCol& rc, const Coord3& pos )
{
    const int colidx = colIndex( rc.row(), rc.col() );

    if ( colidx<0 ) return false;

    (*rows_[rc.row()])[colidx] = pos;

    return true;
}


bool Horizon2DLine::isKnotDefined( const RowCol& rc ) const
{
    const int colidx = colIndex( rc.row(), rc.col() );
    return colidx>=0 ? (*rows_[rc.row()])[colidx].isDefined() : false;
}


Coord3 Horizon2DLine::computePosition( Pos::GeomID geomid, int col ) const
{
    const int rowidx = getRowIndex( geomid );
    if ( rowidx < 0 )
	return Coord3::udf();

    Coord3 position = getKnot( RowCol(rowidx,col) );
    if ( position.isDefined() )
	return position;

    const Coord crd = position.coord();
    if ( !crd.isDefined() )
	return Coord3::udf();

    StepInterval<int> colrg = colRange( rowidx );
    const int nrcols = colrg.nrSteps() + 1;
    const int curcolidx = colrg.nearestIndex( col );
    int col1_ = -1, col2_ = -1, col1 = -1, col2 = -1;
    double z1_ = mUdf(double), z2_ = mUdf(double),
	   z1 = mUdf(double), z2 = mUdf(double);
    int colidx = curcolidx - 1;
    while ( colidx >=0 )
    {
	const RowCol currowcol( rowidx, colrg.atIndex(colidx--) );
	const Coord3 pos = getKnot( currowcol );
	if ( !pos.isDefined() )
	    continue;

	if ( col1_ < 0 )
	{
	    col1_ = colidx + 1;
	    z1_ = pos.z;
	}
	else if ( col2_ < 0 )
	{
	    col2_ = colidx + 1;
	    z2_ = pos.z;
	}
	else
	    break;
    }

    colidx = curcolidx + 1;
    while ( colidx < nrcols )
    {
	const RowCol currowcol( rowidx, colrg.atIndex(colidx++) );
	const Coord3 pos = getKnot( currowcol );
	if ( !pos.isDefined() )
	    continue;

	if ( col1 < 0 )
	{
	    col1 = colidx - 1;
	    z1 = pos.z;
	}
	else if ( col2 < 0 )
	{
	    col2 = colidx - 1;
	    z2 = pos.z;
	}
	else
	    break;
    }

    if ( col1_ < 0 || col2_ < 0 || col1 < 0 || col2 < 0 )
	return Coord3::udf();

    position.z = Interpolate::poly1D<double>( (float)col2_, z2_, (float)col1_,
					      z1_, (float)col1, z1, (float)col2,
					      z2, (float)curcolidx );
    return position;
}


int Horizon2DLine::colIndex( int rowidx, int colid ) const
{
    if ( rowidx<0 || rowidx>=rows_.size() || rowidx>=colsampling_.size() )
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


bool Horizon2DLine::hasSupport( const RowCol& rc ) const
{
    StepInterval<int> colrg = colRange( rc.row() );
    const RowCol prev( rc.row(), rc.col()-colrg.step );
    const RowCol next( rc.row(), rc.col()+colrg.step );
    return isKnotDefined(prev) || isKnotDefined(next);
}


void Horizon2DLine::trimUndefParts()
{
    const int nrlines = geomids_.size();
    for ( int idx = 0; idx<nrlines; idx++ )
    {
	Pos::GeomID geomid = geomids_[idx];
	StepInterval<int> colrg = colRangeForGeomID( geomid );
	bool founddefknot = false;
	for ( int col=colrg.start; col<colrg.stop; col+=colrg.step )
	{
	    if ( isKnotDefined(RowCol(idx,col)) )
		founddefknot = true;
	    else
		continue;

	    if ( founddefknot && col!=colrg.start )
		removeCols( geomid, colrg.start, col-colrg.step );

	    break;
	}

	founddefknot = false;
	for ( int col=colrg.stop; col>=colrg.start; col-=colrg.step )
	{
	    if ( isKnotDefined(RowCol(idx,col)) )
		founddefknot = true;
	    else
		continue;

	    if ( founddefknot && col!=colrg.stop )
		removeCols( geomid, col+colrg.step, colrg.stop );

	    break;
	}

    }
}


} // namespace Geometry
