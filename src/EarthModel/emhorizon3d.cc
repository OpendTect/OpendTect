/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.13 2002-06-28 08:40:19 kristofer Exp $";

#include "emhorizon.h"
#include "geomcompositesurface.h"
#include "geomtristripset.h"
#include "emhorizontransl.h"
#include "executor.h"
#include "grid.h"
#include "geom2dsnappedsurface.h"
#include "survinfo.h"

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

    const GridNode node00( 0, 0 );
    const GridNode node01( 0, 1 );
    const GridNode node10( 1, 0 );

    const Coord coord00 = grid.getCoord( node00 );
    const Coord coord01 = grid.getCoord( node01 );
    const Coord coord10 = grid.getCoord( node10 );

    surfaces.getSurfaces()[0]->setTransform(
	    coord00.x, coord00.y, node00.row, node00.col,
	    coord01.x, coord01.y, node01.row, node01.col,
	    coord10.x, coord00.y, node10.row, node10.col );

    const int nrrows = grid.nrRows();
    const int nrcols = grid.nrCols();

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

	    if ( !mIsUndefined( grid.getValue( gn00 ) ) &&
		    !mIsUndefined( grid.getValue( gn01 ) ) &&
		    !mIsUndefined( grid.getValue( gn10 ) ) &&
		    !mIsUndefined( grid.getValue( gn11 ) ) )
	    {
		surfaces.getSurfaces()[0]->setFillType( gn00,
					Geometry::GridSurface::Filled );
	    }
	}
    }

    return true;
}


Coord EarthModel::Horizon::getCoord( const RowCol& node ) const
{
    if ( !surfaces.getSurfaces()[0] ) return Coord( mUndefValue, mUndefValue );
    Geometry::Pos pos = surfaces.getSurfaces()[0]->getPos( node );
    return Coord( pos.x, pos.y );
}


RowCol EarthModel::Horizon::getClosestNode( const Coord& pos ) const
{
    RowCol res;
    surfaces.getSurfaces()[0]->transform( pos.x, pos.y, res );
    return res;
}


void EarthModel::Horizon::getTriStrips(
	    ObjectSet<Geometry::TriangleStripSet>& tristrips, int res ) const
{
    deepErase( tristrips );
    for ( int idx=0; idx<surfaces.getSurfaces().size(); idx++ )
    {
	Geometry::TriangleStripSet* subhorstrip =
	    					new Geometry::TriangleStripSet;
	surfaces.getSurfaces()[idx]->fillTriStripSet( subhorstrip, 0,
							0, 0, 0, res, res );
	tristrips += subhorstrip;
    }
}
