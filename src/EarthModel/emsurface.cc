/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurface.cc,v 1.1 2003-05-05 12:06:06 kristofer Exp $";

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


int EarthModel::Surface::getNeighbors( const EarthModel::PosID& posid_,
				       TypeSet<EarthModel::PosID>* res ) const
{
    TypeSet<EarthModel::PosID> neigbors;
    TypeSet<EarthModel::PosID> posids;
    getLinkedPos( posid_, posids );
    posids += posid_;

    for ( int idy=0; idy<posids.size(); idy++ )
    {
	const EarthModel::PosID& posid = posids[idy];
	const RowCol rowcol = Geometry::GridSurface::getGridNode(posid.subID());

	for ( int drow=-1; drow<=1; drow++ )
	{
	    for ( int dcol=-1; dcol<=1; dcol++ )
	    {
		const RowCol neighborrowcol( rowcol.row+drow, rowcol.col+dcol );
		const EarthModel::PosID neighborposid( posid.emObject(),
			posid.patchID(),
			Geometry::GridSurface::getPosID(neighborrowcol) );
		TypeSet<EarthModel::PosID> neighborposids;
		getLinkedPos( neighborposid, neighborposids );
		neighborposids += neighborposid;

		bool found = false;
		for ( int idx=0; idx<neighborposids.size(); idx++ )
		{
		    if ( neigbors.indexOf(neighborposids[idx])!=-1 )
		    {
			found = true;
			break;
		    }
		}

		if ( !found ) neigbors += neighborposid;

	    }
	}
    }

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	if ( getPos(neigbors[idx]).isDefined() )
	{
	    neigbors.remove(idx);
	    idx--;
	}
    }


    if ( res ) *res = neigbors;
    return neigbors.size();
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
