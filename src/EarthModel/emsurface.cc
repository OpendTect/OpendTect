/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurface.cc,v 1.2 2003-05-12 08:19:50 kristofer Exp $";

#include "emsurface.h"

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

EarthModel::Surface::Surface(EMManager& man, const MultiID& id_)
    : EMObject( man, id_ )
{}


EarthModel::Surface::~Surface()
{
    cleanUp();
}


int EarthModel::Surface::nrPatches() const
{
    return patchids.size();
}


EarthModel::PatchID EarthModel::Surface::patchID(int idx) const
{
    return patchids[idx];
}


EarthModel::PatchID EarthModel::Surface::addPatch(bool addtohistory)
{
    PatchID res = 0;
    while ( patchids.indexOf(res)!=-1 ) res++;

    addPatch( res, addtohistory );
    return res;
}


bool EarthModel::Surface::addPatch(PatchID patchid, bool addtohistory)
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


void EarthModel::Surface::removePatch(EarthModel::PatchID patchid,
				     bool addtohistory)
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


void  EarthModel::Surface::setPos( PatchID patch, const RowCol& node,
				   const Coord3& pos, bool autoconnect,
				   bool addtohistory)
{
    int idx=patchids.indexOf(patch);
    if ( idx==-1 ) return;

    const Geometry::PosID posid = Geometry::GridSurface::getPosID(node);
    Geometry::GridSurface* surface = surfaces[idx];
    const Coord3 oldpos = surface->getGridPos( node );
    if ( oldpos==pos ) return;

    TypeSet<EarthModel::PosID> nodeonotherpatches;

    if ( autoconnect )
	findPos( node, nodeonotherpatches );

    surface->setGridPos(node, pos);

    if ( addtohistory )
    {
	HistoryEvent* history = new SetPosHistoryEvent( oldpos, pos,
				    EarthModel::PosID(id(),patch,posid) );
	manager.history().addEvent( history, 0, 0 );
    }

    if ( !autoconnect ) return;

    for ( int idx=0; idx<nodeonotherpatches.size(); idx++ )
    {
	const int patchsurfidx = patchids.indexOf(nodeonotherpatches[idx].patchID());
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


bool EarthModel::Surface::setPos( const EarthModel::PosID& posid,
				  const Coord3& newpos, bool addtohistory )
{
    if ( posid.emObject()!=id() ) return false;

    setPos( posid.patchID(), Geometry::GridSurface::getGridNode(posid.subID()),
	    newpos, false, addtohistory );

    return true;
}


Coord3 EarthModel::Surface::getPos(const EarthModel::PosID& posid) const
{
    const int surfidx = patchids.indexOf( posid.patchID() );
    return surfaces[surfidx]->getPos( posid.subID() );
}


int EarthModel::Surface::findPos( const RowCol& rowcol,
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
	EarthModel::SubID subid = Geometry::GridSurface::getPosID( rowcol );

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


int EarthModel::Surface::getNeighbors(	const EarthModel::PosID& posid_,
					TypeSet<EarthModel::PosID>* res,
       					int maxradius, bool circle ) const
{
    ObjectSet< TypeSet<EarthModel::PosID> > neigbors;
    const RowCol start = Geometry::GridSurface::getGridNode(posid_.subID());
    neigbors += new TypeSet<EarthModel::PosID>( 1, posid_ );

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	for ( int idz=0; idz<neigbors[idx]->size(); idz++ )
	{
	    EarthModel::PosID currentposid = (*neigbors[idx])[idz];
	    const RowCol rowcol =
		     Geometry::GridSurface::getGridNode(currentposid.subID());

	    for ( int row=-1; row<=1; row++ )
	    {
		for ( int col=-1; col<=1; col++ )
		{
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
		    const EarthModel::PosID
			    neighborposid(currentposid.emObject(),
			    currentposid.patchID(),
			    Geometry::GridSurface::getPosID(neighborrowcol) );

		    for ( int idy=0; idy<neigbors.size(); idy++ )
		    {
			const TypeSet<EarthModel::PosID>& posids=*neigbors[idy];
			if ( posids.indexOf(neighborposid)!=-1 )
			{
			    found = true;
			    break;
			}
		    }

		    if ( found )
			continue;

		    TypeSet<EarthModel::PosID>& posids =
			*new TypeSet<EarthModel::PosID>( 1, neighborposid );
		    getLinkedPos( neighborposid, posids );
		    neigbors += &posids;

		}
	    }
	}
    }


    if ( res )
    {
	for ( int idx=0; idx<neigbors.size(); idx++ )
	{
	    (*res) += (*neigbors[idx])[0];
	}
    }

    const int size = neigbors.size();
    deepErase( neigbors );

    return size;
}


void EarthModel::Surface::getLinkedPos( const EarthModel::PosID& posid,
					TypeSet<EarthModel::PosID>& res ) const
{
    if ( posid.emObject()!=id() )
        return; //TODO: Implement handling for this case

    const EarthModel::SubID subid = posid.subID();
    const RowCol rowcol = Geometry::GridSurface::getGridNode(subid);
    const Geometry::GridSurface* owngridsurf = getSurface( posid.patchID() );

    const int nrsubsurf = nrPatches();
    for ( int surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::GridSurface* gridsurf = surfaces[surface];
	if ( owngridsurf->isLinked( subid, gridsurf, subid ) )
	{
	    res += EarthModel::PosID( id(),patchids[surface], subid );
	}
    }
}


const Geometry::GridSurface* EarthModel::Surface::getSurface(PatchID patchid)const
{
    const int idx = patchids.indexOf( patchid );
    return idx==-1 ? 0 : surfaces[idx];
}



void EarthModel::Surface::cleanUp()
{
    deepErase( surfaces );
}
