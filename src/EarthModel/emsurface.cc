/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurface.cc,v 1.5 2003-06-03 12:46:12 bert Exp $";

#include "emsurface.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
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
#include "survinfo.h"

EM::Surface::Surface(EMManager& man, const MultiID& id_)
    : EMObject( man, id_ )
{}


EM::Surface::~Surface()
{
    cleanUp();
}


int EM::Surface::nrPatches() const
{
    return patchids.size();
}


EM::PatchID EM::Surface::patchID( int idx ) const
{
    return patchids[idx];
}


EM::PatchID EM::Surface::addPatch( bool addtohistory )
{
    PatchID res = 0;
    while ( patchids.indexOf(res)!=-1 ) res++;

    addPatch( res, addtohistory );
    return res;
}


bool EM::Surface::addPatch( PatchID patchid, bool addtohistory )
{
    if ( patchids.indexOf(patchid) != -1 ) return false;

    patchids += patchid;
    Geometry::GridSurface* newsurf = createPatchSurface();
    surfaces += newsurf;

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfacePatchEvent( true, id(), patchid );
	manager.history().addEvent( history, 0, 0 );
    }

    return true;
}


void EM::Surface::removePatch( EM::PatchID patchid, bool addtohistory )
{
    int idx=patchids.indexOf(patchid);
    if ( idx==-1 ) return;

    delete surfaces[idx];
    surfaces.remove( idx );
    patchids.remove( idx );

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfacePatchEvent( false, id(), patchid );
	manager.history().addEvent( history, 0, 0 );
    }
}


void EM::Surface::setPos( PatchID patch, const RowCol& node,
				   const Coord3& pos, bool autoconnect,
				   bool addtohistory)
{
    int idx=patchids.indexOf(patch);
    if ( idx==-1 ) return;

    const Geometry::PosID posid = Geometry::GridSurface::getPosID(node);
    Geometry::GridSurface* surface = surfaces[idx];
    const Coord3 oldpos = surface->getGridPos( node );
    if ( oldpos==pos ) return;

    TypeSet<EM::PosID> nodeonotherpatches;

    if ( autoconnect )
	findPos( node, nodeonotherpatches );

    surface->setGridPos( node, pos );
    surface->setFillType( node, Geometry::GridSurface::Filled );

    if ( addtohistory )
    {
	HistoryEvent* history = new SetPosHistoryEvent( oldpos, pos,
				    EM::PosID(id(),patch,posid) );
	manager.history().addEvent( history, 0, 0 );
    }

    if ( !autoconnect ) return;

    for ( int idx=0; idx<nodeonotherpatches.size(); idx++ )
    {
	const int patchsurfidx =
	    patchids.indexOf(nodeonotherpatches[idx].patchID());
	double otherz = surfaces[patchsurfidx]->getGridPos(node).z;
	
	if ( mIS_ZERO(otherz-pos.z) )
	{
	    if ( !surface->isLinked(posid, surfaces[patchsurfidx], posid ))
	    {
		surface->setLink( posid, surfaces[patchsurfidx], posid, true );
		// Put to history?
	    }
	}
    }
}


bool EM::Surface::setPos( const EM::PosID& posid, const Coord3& newpos,
			  bool addtohistory )
{
    if ( posid.emObject()!=id() ) return false;

    setPos( posid.patchID(), Geometry::GridSurface::getGridNode(posid.subID()),
	    newpos, false, addtohistory );

    return true;
}


Coord3 EM::Surface::getPos( const EM::PosID& posid ) const
{
    const int surfidx = patchids.indexOf( posid.patchID() );
    return surfaces[surfidx]->getPos( posid.subID() );
}


int EM::Surface::findPos( const RowCol& rowcol,
				  TypeSet<PosID>& res ) const
{
    TypeSet<Coord3> respos;
    const int nrsubsurf = nrPatches();
    for ( unsigned short surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::GridSurface* gridsurf = surfaces[surface];
	if ( !gridsurf->isDefined( rowcol ) )
	    continue;

	Coord3 pos = gridsurf->getGridPos( rowcol );
	EM::SubID subid = Geometry::GridSurface::getPosID( rowcol );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( subid!=res[idx].subID() )
		continue;

	    if ( mIS_ZERO( respos[idx].z-pos.z ) ) continue;

	    res += PosID(id(), patchID(surface), subid );
	    respos += pos;
	}
    }

    return res.size();
}


int EM::Surface::findPos( const CubeSampling& cs,
			  TypeSet<EM::PosID>* res ) const
{
    Coord xypos = SI().transform(cs.hrg.start);
    Interval<float> xinterval( xypos.x, xypos.x );
    Interval<float> yinterval( xypos.y, xypos.y );

    xypos = SI().transform(cs.hrg.stop);
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    xypos = SI().transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    xypos = SI().transform( BinID(cs.hrg.stop.inl,cs.hrg.start.crl) );
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    TypeSet<EM::PosID> posids;

    const int nrpatches = nrPatches();
    for ( int idx=0; idx<nrpatches; idx++ )
    {
	TypeSet<Geometry::PosID> nodes;
	surfaces[idx]->findPos( xinterval.center(), yinterval.center(),
				cs.zrg.center(),
				xinterval.width(), yinterval.width(),
				cs.zrg.width(), nodes );

	const int nrnodes = nodes.size();
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const BinID nodebid =
		SI().transform(surfaces[idx]->getPos(nodes[idy]));

	    if ( nodebid.inl<cs.hrg.start.inl || nodebid.inl>cs.hrg.stop.inl ||
		 nodebid.crl<cs.hrg.start.crl || nodebid.crl>cs.hrg.stop.crl )
		continue;

	    EM::PosID posid( id(), patchids[idx], nodes[idy] );

	    TypeSet<EM::PosID> clones;
	    getLinkedPos( posid, clones );
	    clones += posid;

	    const int nrclones = clones.size();
	    bool found = false;
	    for ( int idz=0; idz<nrclones; idz++ )
	    {
		if ( posids.indexOf(clones[idz]) != -1 )
		{
		    found = true;
		    break;
		}
	    }

	    if ( !found )
	    {
		posids += posid;
	    }
	}
    }

    if ( res ) *res = posids;
    return posids.size();
}


int EM::Surface::getNeighbors( const EM::PosID& posid_, TypeSet<EM::PosID>* res,
				int maxradius, bool circle ) const
{
    ObjectSet< TypeSet<EM::PosID> > neigbors;
    const RowCol start = Geometry::GridSurface::getGridNode(posid_.subID());
    neigbors += new TypeSet<EM::PosID>( 1, posid_ );

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	for ( int idz=0; idz<neigbors[idx]->size(); idz++ )
	{
	    EM::PosID currentposid = (*neigbors[idx])[idz];
	    const RowCol rowcol =
		     Geometry::GridSurface::getGridNode(currentposid.subID());

	    for ( int row=-1; row<=1; row++ )
	    {
		for ( int col=-1; col<=1; col++ )
		{
		    if ( !row && !col ) continue;

		    const RowCol neighborrowcol(rowcol.row+row,rowcol.col+col);
		    const int drow = abs(neighborrowcol.row-start.row);
		    const int dcol = abs(neighborrowcol.col-start.col);

		    if ( drow>maxradius || dcol>maxradius )
			continue;

		    if ( circle && (drow*drow+dcol*dcol)> maxradius*maxradius)
			continue;
		   

		    
		    const Geometry::GridSurface* surface =
		    			getSurface(currentposid.patchID());

		    if ( !surface->isDefined(neighborrowcol))
			continue;

		    bool found = false;
		    const EM::PosID
			    neighborposid(currentposid.emObject(),
			    currentposid.patchID(),
			    Geometry::GridSurface::getPosID(neighborrowcol) );

		    for ( int idy=0; idy<neigbors.size(); idy++ )
		    {
			const TypeSet<EM::PosID>& posids=*neigbors[idy];
			if ( posids.indexOf(neighborposid)!=-1 )
			{
			    found = true;
			    break;
			}
		    }

		    if ( found )
			continue;

		    TypeSet<EM::PosID>& posids =
					    *new TypeSet<EM::PosID>;
		    getLinkedPos( neighborposid, posids );
		    posids.insert( 0, neighborposid );

		    neigbors += &posids;

		}
	    }
	}
    }


    if ( res )
    {
	// Leave out the fist one, since it's the origin
	for ( int idx=1; idx<neigbors.size(); idx++ )
	{
	    (*res) += (*neigbors[idx])[0];
	}
    }

    const int size = neigbors.size();
    deepErase( neigbors );

    // Leave out the fist one, since it's the origin
    return size-1;
}


void EM::Surface::getLinkedPos( const EM::PosID& posid,
				TypeSet<EM::PosID>& res ) const
{
    if ( posid.emObject()!=id() )
        return; //TODO: Implement handling for this case

    const EM::SubID subid = posid.subID();
    const RowCol rowcol = Geometry::GridSurface::getGridNode(subid);
    const Geometry::GridSurface* owngridsurf = getSurface( posid.patchID() );

    const int nrsubsurf = nrPatches();
    for ( int surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::GridSurface* gridsurf = surfaces[surface];
	if ( owngridsurf->isLinked( subid, gridsurf, subid ) )
	{
	    res += EM::PosID( id(),patchids[surface], subid );
	}
    }
}


bool EM::Surface::isLoaded() const
{
    return nrPatches();
}


const Geometry::GridSurface* EM::Surface::getSurface( PatchID patchid )const
{
    const int idx = patchids.indexOf( patchid );
    return idx==-1 ? 0 : surfaces[idx];
}



void EM::Surface::cleanUp()
{
    deepErase( surfaces );
    patchids.erase();
}
