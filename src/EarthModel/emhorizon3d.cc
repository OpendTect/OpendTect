/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.21 2003-04-22 11:01:52 kristofer Exp $";

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
    : EMObject( man, id_ )
    , a11( 1 ) , a12( 0 ) , a13( 0 ) , a21( 0 ) , a22( 1 ) , a23( 0 )
    , b11( 1 ) , b12( 0 ) , b13( 0 ) , b21( 0 ) , b22( 1 ) , b23( 0 )
{}


EarthModel::Horizon::~Horizon()
{
    cleanUp();
}


int EarthModel::Horizon::nrParts() const
{
    return partids.size();
}


EarthModel::PartID EarthModel::Horizon::partID(int idx) const
{
    return partids[idx];
}


EarthModel::PartID EarthModel::Horizon::addPart(bool addtohistory)
{
    PartID res = 0;
    while ( partids.indexOf(res)!=-1 ) res++;

    addPart( res, addtohistory );
    return res;
}


bool EarthModel::Horizon::addPart(PartID partid, bool addtohistory)
{
    if ( partids.indexOf(partid) != -1 ) return false;

    partids += partid;
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
    surfaces += newsurf;

    if ( addtohistory )
    {
	HistoryEvent* history = new HorizonPartEvent( true, id(), partid );
	manager.history().addEvent( history, 0, 0 );
    }

    return true;
}


void EarthModel::Horizon::removePart(EarthModel::PartID partid,
				     bool addtohistory)
{
    int idx=partids.indexOf(partid);
    if ( idx==-1 ) return;

    delete surfaces[idx];
    surfaces.remove( idx );
    partids.remove( idx );

    if ( addtohistory )
    {
	HistoryEvent* history = new HorizonPartEvent( false, id(), partid );
	manager.history().addEvent( history, 0, 0 );
    }
}


void  EarthModel::Horizon::setPos( PartID part, const RowCol& node,
				   const Coord3& pos, bool autoconnect,
				   bool addtohistory)
{
    int idx=partids.indexOf(part);
    if ( idx==-1 ) return;

    const Geometry::PosID posid = Geometry::GridSurface::getPosId(node);
    Geometry::Snapped2DSurface* surface = surfaces[idx];
    const Coord3 oldpos = surface->getGridPos( node );
    if ( oldpos==pos ) return;

    TypeSet<EarthModel::PosID> nodeonotherparts;
    if ( autoconnect )
	findPos( node, nodeonotherparts );

    surface->setGridPos(node, pos);

    if ( addtohistory )
    {
	HistoryEvent* history = new SetPosHistoryEvent( oldpos, pos,
				    EarthModel::PosID(id(),part,posid) );
	manager.history().addEvent( history, 0, 0 );
    }

    if ( !autoconnect ) return;

    for ( int idx=0; idx<nodeonotherparts.size(); idx++ )
    {
	const int partsurfidx = partids.indexOf(nodeonotherparts[idx].partID());
	double otherz = surfaces[partsurfidx]->getGridPos(node).z;
	
	if ( mIS_ZERO(otherz-pos.z) )
	{
	    if ( !surface->isLinked(posid, surfaces[partsurfidx], posid ))
	    {
		surface->setLink( posid, surfaces[partsurfidx], posid, true );
		// Put to history?
	    }
	}
    }
}


bool EarthModel::Horizon::setPos( const EarthModel::PosID& posid,
				  const Coord3& newpos, bool addtohistory )
{
    if ( posid.emObject()!=id() ) return false;

    setPos( posid.partID(), Geometry::GridSurface::getGridNode(posid.subID()),
	    newpos, false, addtohistory );

    return true;
}


Coord3 EarthModel::Horizon::getPos(const EarthModel::PosID& posid) const
{
    const int surfidx = partids.indexOf( posid.partID() );
    return surfaces[surfidx]->getPos( posid.subID() );
}


int EarthModel::Horizon::findPos( const RowCol& rowcol,
				  TypeSet<PosID>& res ) const
{
    TypeSet<Coord3> respos;
    const int nrsubsurf = nrParts();
    for ( unsigned short surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::GridSurface* gridsurf = surfaces[surface];
	if ( !gridsurf->isDefined( rowcol ) )
	    continue;

	Coord3 pos = gridsurf->getGridPos( rowcol );
	EarthModel::SubID subid = Geometry::GridSurface::getPosId( rowcol );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( subid!=res[idx].subID() )
		continue;

	    if ( mIS_ZERO( respos[idx].z-pos.z ) ) continue;

	    res += PosID(id(), partID(surface), subid );
	    respos += pos;
	}
    }

    return res.size();
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
    return nrParts();
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

    const EarthModel::PartID part = addPart(true);

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


const Geometry::GridSurface* EarthModel::Horizon::getSurface(PartID partid)const
{
    const int idx = partids.indexOf( partid );
    return idx==-1 ? 0 : surfaces[idx];
}



void EarthModel::Horizon::cleanUp()
{
    deepErase( surfaces );
}


void EarthModel::Horizon::setTransform(
    float x1, float y1, float i0_1, float i1_1,
    float x2, float y2, float i0_2, float i1_2,
    float x3, float y3, float i0_3, float i1_3 )
{
    for ( int idx=0; idx<surfaces.size(); idx++ )
    {
	surfaces[idx]->setTransform(   x1, y1, i0_1, i1_1,
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
