/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: binidsurface.cc,v 1.3 2005-03-10 11:45:04 cvskris Exp $";

#include "binidsurface.h"

#include "parametricsurfaceimpl.h"

#include "arrayndimpl.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"


namespace Geometry
{


BinIDSurface::BinIDSurface( const RCol& newstep )
    : ParametricSurface( RowCol(0,0), newstep ) 
    , depths( 0 )
{ }


BinIDSurface::BinIDSurface( const BinIDSurface& b )
    : ParametricSurface( b.origo, b.step ) 
    , depths( b.depths ? new Array2DImpl<float>(*b.depths) : 0 )
{ }


BinIDSurface::~BinIDSurface()
{ delete depths; }


BinIDSurface* BinIDSurface::clone() const
{ return new BinIDSurface(*this); }


bool BinIDSurface::insertRow(int row) 
{
    mInsertStart( rowidx, row, nrRows() );
    mCloneRowVariable( float, depths, computePosition(param).z, mUndefValue )
    return true;
}


bool BinIDSurface::insertCol(int col) 
{
    mInsertStart( colidx, col, nrCols() );
    mCloneColVariable( float, depths, computePosition(param).z, mUndefValue )
    return true;
}


bool BinIDSurface::removeRow( int row )
{
    pErrMsg( "not implemented ");
    return true;
}


bool BinIDSurface::removeCol( int col )
{
    pErrMsg( "not implemented ");
    return true;
}


Coord3 BinIDSurface::getKnot( const RCol& rc, bool interpolifudf ) const
{
    const int index = getKnotIndex(rc);
    if ( index==-1 ) return Coord3::udf();

    return Coord3(SI().transform(BinID(rc)), depths->getData()[index]);
}


void BinIDSurface::_setKnot( int idx, const Coord3& np )
{
    if ( !depths )
    {
	depths = new Array2DImpl<float>( 1, 1 );
	idx = 0;
    }

    depths->getData()[idx] = np.z;
}


int BinIDSurface::nrRows() const
{ return depths ? depths->info().getSize(rowDim()) : 0; }


int BinIDSurface::nrCols() const
{ return depths ? depths->info().getSize(colDim()) : 0; }


};

