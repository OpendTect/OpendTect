/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.12 2002-05-29 06:28:54 kristofer Exp $";

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

    setTransformation( *surfaces.getSurfaces()[0] );

    for ( int row=0; row<nrrows; row++ )
    {
	for ( int col=0; col<nrcols; col++ )
	{
	    GridNode gridnode( col, row );
	    Coord coord = grid.getCoord( gridnode );
	    float val = grid.getValue( gridnode );

	    Geometry::Pos pos(coord.x, coord.y, val );

	    BinID binid = SI().transform( coord );
	    Geometry::GridNode surfnode = getNode( binid );

	    surfaces.getSurfaces()[0]->setPos( surfnode, pos );
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
		Coord coord = grid.getCoord( gn00 );
		BinID binid = SI().transform( coord );
		Geometry::GridNode surfnode = getNode( binid );
		surfaces.getSurfaces()[0]->setFillType( surfnode,
					Geometry::GridSurface::Filled );
	    }
	}
    }

    return true;
}


BinID EarthModel::Horizon::getBid( const Geometry::GridNode& node )
{
    BinID start = SI().range().start;
    BinID step = SI().step();

    return BinID( start.inl+node.row*step.inl,start.crl+node.col*step.crl );
}


Geometry::GridNode EarthModel::Horizon::getNode( const BinID& bid )
{
    BinID start = SI().range().start;
    BinID step = SI().step();

    return Geometry::GridNode(  (bid.inl-start.inl)/step.inl,
	    			(bid.crl-start.crl)/step.crl );
}


void EarthModel::Horizon::setTransformation( Geometry::Snapped2DSurface& surf )
{
    const BinID start = SI().range().start;
    const BinID step = SI().step();

    const BinID bid00 = start;
    const BinID bid01( start.inl+step.crl, start.crl + step.crl );
    const BinID bid11( start.inl+4*step.inl, start.crl + 14*step.crl );

    const Coord c00( SI().transform( bid00 ) );
    const Coord c01( SI().transform( bid01 ) );
    const Coord c11( SI().transform( bid11 ) );

    const RowCol rc00 = getNode( bid00 );
    const RowCol rc01 = getNode( bid01 );
    const RowCol rc11 = getNode( bid11 );

    surf.setTransform(	c00.x, c00.y, rc00.row, rc00.col,
			c01.x, c01.y, rc01.row, rc01.col,
			c11.x, c11.y, rc11.row, rc11.col);
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
