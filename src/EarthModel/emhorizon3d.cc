/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.17 2002-09-12 15:01:22 nanne Exp $";

#include "emhorizon.h"
#include "geomcompositesurface.h"
#include "geomtristripset.h"
#include "emhorizontransl.h"
#include "executor.h"
#include "grid.h"
#include "geom2dsnappedsurface.h"
#include "survinfo.h"
#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "linsolv.h"


#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"

EarthModel::Horizon::Horizon(EMManager& man, const MultiID& id_)
    : EMObject( man, id_ )
    , surfaces( *new Geometry::CompositeGridSurface )
    , a11( 1 ) , a12( 0 ) , a13( 0 ) , a21( 0 ) , a22( 1 ) , a23( 0 )
    , b11( 1 ) , b12( 0 ) , b13( 0 ) , b21( 0 ) , b22( 1 ) , b23( 0 )

{ }


EarthModel::Horizon::~Horizon()
{
    delete &surfaces;
}


int EarthModel::Horizon::findPos( const RowCol& rowcol,
				  TypeSet<PosID>& res ) const
{
    res.erase();

    const int nrsubsurf = surfaces.nrSubSurfaces();
    for ( unsigned short surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::Pos pos = surfaces.getPos( surface, rowcol );
	if ( !pos.isDefined() ) continue;

	unsigned long surfpid = Geometry::GridSurface::getPosId( rowcol );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( surfpid!=getSurfPID(res[idx]) ) continue;

	    unsigned short tmpsurf = getSurfID(res[idx]);
	    Geometry::Pos tmppos = surfaces.getPos( tmpsurf, surfpid );
	    if ( mIS_ZERO( tmppos.z-pos.z ) ) continue;

	    PosID pid = getPosID( surface, surfpid );
	    res += pid;
	}
    }

    return res.size();
}


void EarthModel::Horizon::addSquare( const RowCol& rowcol,
				     float inl0crl0z, float inl0crl1z,
				     float inl1crl0z, float inl1crl1z )
{
    surfaces.addSquare( rowcol, inl0crl0z, inl0crl1z, inl1crl0z, inl1crl1z );
}


unsigned short EarthModel::Horizon::getSurfID( PosID posid )
{
    return (posid.subid>>32) & 0x0000FFFF;
}


unsigned long EarthModel::Horizon::getSurfPID( PosID posid )
{
    return posid.subid & 0x00000000FFFFFFFFl;
}


EarthModel::PosID EarthModel::Horizon::getPosID( unsigned short surfid,
						 unsigned long  surfpid ) const
{
    PosID res;
    res.subid = ( ((unsigned long long) surfid)<<32 ) + surfpid;
    res.objid = id();
    return res;
}


Executor* EarthModel::Horizon::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );

    Executor* exec = EarthModelHorizonTranslator::reader( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}


bool  EarthModel::Horizon::isLoaded() const
{
    return surfaces.nrSubSurfaces();
}


Executor* EarthModel::Horizon::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );

    Executor* exec = EarthModelHorizonTranslator::writer( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}


bool EarthModel::Horizon::import( const Grid& grid )
{
    while ( surfaces.nrSubSurfaces() ) surfaces.removeSubSurface( 0 );
    surfaces.addSubSurface();

    const int nrrows = grid.nrRows();
    const int nrcols = grid.nrCols();

    const GridNode node00( 0, 0 );
    const GridNode node01( 0, nrrows-1 );
    const GridNode node10( nrcols-1, 0 );

    const Coord coord00 = grid.getCoord( node00 );
    const Coord coord01 = grid.getCoord( node01 );
    const Coord coord10 = grid.getCoord( node10 );

    setTransform( coord00.x, coord00.y, node00.row, node00.col,
		  coord01.x, coord01.y, node01.row, node01.col,
		  coord10.x, coord10.y, node10.row, node10.col );

    for ( int row=0; row<nrrows; row++ )
    {
	for ( int col=0; col<nrcols; col++ )
	{
	    GridNode gridnode( col, row );
	    Coord coord = grid.getCoord( gridnode );
	    float val = grid.getValue( gridnode );

	    Geometry::Pos pos(coord.x, coord.y, val );
	    surfaces.getSurfaces()[0]->setPos( gridnode, pos );
	}
    }

    for ( int row=0; row<nrrows-1; row++ )
    {
	for ( int col=0; col<nrcols-1; col++ )
	{
	    GridNode gn00( col, row );
	    GridNode gn01( col, row+1 );
	    GridNode gn10( col+1, row );
	    GridNode gn11( col+1, row+1 );

	    surfaces.getSurfaces()[0]->setFillType( gn00,
					Geometry::GridSurface::Filled );
	}
    }

    return true;
}


Coord EarthModel::Horizon::getCoord( const RowCol& node ) const
{
    return Coord( a11*node.row+a12*node.col+a13,
	    	  a21*node.row+a22*node.col+a23 );
}


RowCol EarthModel::Horizon::getClosestNode( const Coord& pos ) const
{
    return RowCol( mNINT(b11*pos.x+b12*pos.y + b13),
	           mNINT(b21*pos.x+b22*pos.y + b23) );
}


void EarthModel::Horizon::setTransform(
    float x1, float y1, float i0_1, float i1_1,
    float x2, float y2, float i0_2, float i1_2,
    float x3, float y3, float i0_3, float i1_3 )
{
    for ( int idx=0; idx<surfaces.nrSubSurfaces(); idx++ )
    {
	surfaces.getSurfaces()[idx]->setTransform( x1, y1, i0_1, i1_1,
						   x2, y2, i0_2, i1_2,
						   x3, y3, i0_3, i1_3 );
    }

    Array2DImpl<double> A(3,3);
    A.set( 0, 0, i0_1 );
    A.set( 0, 1, i1_1 );
    A.set( 0, 2, 1 );

    A.set( 1, 0, i0_2 );
    A.set( 1, 1, i1_2 );
    A.set( 1, 2, 1 );

    A.set( 2, 0, i0_3 );
    A.set( 2, 1, i1_3 );
    A.set( 2, 2, 1 );

    double b[] = { x1, x2, x3 };
    double x[3];

    LinSolver<double> linsolver( A );
    linsolver.apply( b, x );
    a11 = x[0]; a12 = x[1]; a13 = x[2];

    b[0] = y1; b[1] = y2; b[2] = y3;

    linsolver.apply( b, x );
    a21 = x[0]; a22 = x[1]; a23 = x[2];

    A.set( 0, 0, x1 );
    A.set( 0, 1, y1 );
    A.set( 0, 2, 1 );

    A.set( 1, 0, x2 );
    A.set( 1, 1, y2 );
    A.set( 1, 2, 1 );

    A.set( 2, 0, x3 );
    A.set( 2, 1, y3 );
    A.set( 2, 2, 1 );

    b[0] = i0_1; b[1] = i0_2; b[2] = i0_3;

    LinSolver<double> linsolverB( A );
    linsolverB.apply( b, x );
    b11 = x[0]; b12 = x[1]; b13 = x[2];

    b[0] = i1_1; b[1] = i1_2; b[2] = i1_3;

    linsolverB.apply( b, x );
    b21 = x[0]; b22 = x[1]; b23 = x[2];
}
