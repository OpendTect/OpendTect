/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.4 2002-05-22 06:17:03 kristofer Exp $";

#include "emhorizon.h"
#include "geomcompositesurface.h"
#include "geomtristripset.h"
#include "emhorizontransl.h"
#include "executor.h"
#include "grid.h"
#include "geom2dsnappedsurface.h"

#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"

EarthModel::Horizon::Horizon(EMManager& man, const MultiID& id_)
    : EMObject( man, id_ )
    , surfaces( *new Geometry::CompositeGridSurface )
{ }


EarthModel::Horizon::~Horizon()
{
    delete &surfaces;
}


int EarthModel::Horizon::findPos( int inl, int crl, TypeSet<PosID>& res ) const
{
    res.erase();

    const int nrsubsurf = surfaces.nrSubSurfaces();
    for ( unsigned short surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::GridNode gridnode(inl,crl);
	Geometry::Pos pos = surfaces.getPos( surface, gridnode );
	if ( !pos.isDefined() ) continue;

	unsigned long surfpid = Geometry::GridSurface::getPosId( gridnode );

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


void EarthModel::Horizon::addSquare( int inl, int crl,
				     float inl0crl0z, float inl0crl1z,
				     float inl1crl0z, float inl1crl1z )
{
    surfaces.addSquare( Geometry::GridNode( inl, crl ),
	    			inl0crl0z, inl0crl1z, inl1crl0z, inl1crl1z );
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

    const GridNode n00( 0, 0 );
    const GridNode n01( 0, 1 );
    const GridNode n11( 1, 1 );

    const Coord c00 = grid.getCoord( n00 );
    const Coord c01 = grid.getCoord( n01 );
    const Coord c11 = grid.getCoord( n11 );


    surfaces.getSurfaces()[0]->setTransform( c00.x, c00.y, n00.row, n00.col,
	    				     c01.x, c01.y, n01.row, n01.col,
					     c11.x, c11.y, n11.row, n11.col );

    for ( int row=0; row<nrrows; row++ )
    {
	for ( int col=0; col<nrcols; col++ )
	{
	    GridNode node( row, col );
	    Coord coord = grid.getCoord( node );
	    Geometry::Pos pos(coord.x, coord.y, grid.getValue( node ));

	    surfaces.getSurfaces()[0]->setPos( node, pos );
	}
    }

    return true;
}



void EarthModel::Horizon::getTriStrips(
				Geometry::TriangleStripSet* tristrips ) const
{
}
