/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.22 2003-05-05 11:57:30 kristofer Exp $";

#include "emhorizon.h"

#include "arrayndimpl.h"
#include "emhistoryimpl.h"
#include "emhorizontransl.h"
#include "emmanager.h"
#include "executor.h"
#include "geom2dsnappedsurface.h"
#include "grid.h"
#include "ioman.h"
#include "ioobj.h"
#include "linsolv.h"
#include "ptrman.h"

EarthModel::Horizon::Horizon(EMManager& man, const MultiID& id_)
    : Surface( man, id_ )
    , a11( 1 ) , a12( 0 ) , a13( 0 ) , a21( 0 ) , a22( 1 ) , a23( 0 )
    , b11( 1 ) , b12( 0 ) , b13( 0 ) , b21( 0 ) , b22( 1 ) , b23( 0 )
{}


EarthModel::Horizon::~Horizon()
{ }



Geometry::GridSurface* EarthModel::Horizon::createPatchSurface() const
{
    Geometry::Snapped2DSurface* newsurf = new Geometry::Snapped2DSurface();
    RowCol rc00( 0, 0 );
    RowCol rc10( 1, 0 );
    RowCol rc11( 1, 1 );

    Coord pos00 = getCoord( rc00 );
    Coord pos10 = getCoord( rc10 );
    Coord pos11 = getCoord( rc11 );
    
    newsurf->setTransform(  pos00.x, pos00.y, rc00.row, rc00.col,
			    pos10.x, pos10.y, rc10.row, rc10.col,
			    pos11.x, pos11.y, rc11.row, rc11.col );
    return newsurf;
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
    return nrPatches();
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
    cleanUp();

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

    const EarthModel::PatchID part = addPatch(true);

    for ( int row=0; row<nrrows; row++ )
    {
	for ( int col=0; col<nrcols; col++ )
	{
	    GridNode gridnode( col, row );
	    Coord coord = grid.getCoord( gridnode );
	    float val = grid.getValue( gridnode );

	    Coord3 pos(coord.x, coord.y, val );
	    setPos( part, gridnode, pos, false, true );
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
    for ( int idx=0; idx<surfaces.size(); idx++ )
    {
	dynamic_cast<Geometry::Snapped2DSurface*>(
		surfaces[idx])->setTransform(  x1, y1, i0_1, i1_1,
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
