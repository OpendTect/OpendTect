/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurface.cc,v 1.10 2003-06-19 13:38:32 bert Exp $";

#include "emsurface.h"
#include "emsurfaceiodata.h"

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


void EM::SurfaceIOData::clear()
{
    dbinfo = "";
    deepErase(valnames);
    deepErase(patches);
}

void EM::SurfaceIOData::use( const EM::Surface& surf )
{
    clear();

    StepInterval<int> hrg;
    surf.getRange( hrg, true );
    rg.start.inl = hrg.start; rg.stop.inl = hrg.stop;
    rg.step.inl = hrg.step;
    surf.getRange( hrg, false );
    rg.start.crl = hrg.start; rg.stop.crl = hrg.stop;
    rg.step.crl = hrg.step;

    for ( int idx=0; idx<surf.nrPatches(); idx++ )
	patches += new BufferString( surf.patchName( surf.patchID(idx) ) );

    for ( int idx=0; idx<surf.nrAuxData(); idx++ )
	valnames += new BufferString( surf.auxDataName(idx) );
}


void EM::SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    selvalues.erase(); selpatches.erase();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
    for ( int idx=0; idx<sd.patches.size(); idx++ )
	selpatches += idx;
}


EM::Surface::Surface(EMManager& man, const MultiID& id_)
    : EMObject( man, id_ )
    , step( SI().getStep(true, false), SI().getStep(false,false))
    , loadedstep( SI().getStep(true, false), SI().getStep(false,false))
    , rowinterval( 0 )
    , colinterval( 0 )
{
    auxdatanames.allowNull(true);
    auxdatainfo.allowNull(true);
    auxdata.allowNull(true);
}


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


const char* EM::Surface::patchName( const EM::PatchID& patchid ) const
{
    int idx = patchids.indexOf(patchid);
    const char* res = idx!=-1 ? patchnames[idx]->buf() : 0;
    return  res && !*res ? res : 0;
}


EM::PatchID EM::Surface::addPatch( const char* nm, bool addtohistory )
{
    PatchID res = 0;
    while ( patchids.indexOf(res)!=-1 ) res++;

    addPatch( nm, res, addtohistory );
    return res;
}


bool EM::Surface::addPatch( const char* nm, PatchID patchid, bool addtohistory )
{
    if ( patchids.indexOf(patchid) != -1 ) return false;

    BufferString name;
    patchids += patchid;
    if ( nm )
	name = nm;
    else
    {
	name = "Patch ";
	name += patchid;
    }

    patchnames += new BufferString(name);

    Geometry::GridSurface* newsurf = createPatchSurface();
    surfaces += newsurf;

    for ( int idx=0; idx<nrAuxData(); idx++ )
    {
	if ( !auxdata[idx] )
	    continue;

	(*auxdata[idx]) += 0;
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfacePatchEvent( true, id(),
							patchid, name );
	manager.history().addEvent( history, 0, 0 );
    }

    return true;
}


void EM::Surface::removePatch( EM::PatchID patchid, bool addtohistory )
{
    int idx=patchids.indexOf(patchid);
    if ( idx==-1 ) return;

    BufferString name = *patchnames[idx];

    delete surfaces[idx];
    surfaces.remove( idx );
    patchids.remove( idx );
    delete patchnames[idx];
    patchnames.remove( idx );

    for ( int idy=0; idy<nrAuxData(); idy++ )
    {
	if ( !auxdata[idy] )
	    continue;

	delete (*auxdata[idy])[idx];
	auxdata[idy]->replace( 0, idx );
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfacePatchEvent( false, id(),
							patchid, name );
	manager.history().addEvent( history, 0, 0 );
    }
}


bool EM::Surface::setPos( PatchID patch, const EM::SubID& subid,
				   const Coord3& pos, bool autoconnect,
				   bool addtohistory)
{
    RowCol node;
    if ( !getGridRowCol( subid, node ) )
	return false;

    int patchindex=patchids.indexOf(patch);
    if ( patchindex==-1 ) return false;

    const Geometry::PosID posid = Geometry::GridSurface::getPosID(node);
    Geometry::GridSurface* surface = surfaces[patchindex];
    const Coord3 oldpos = surface->getGridPos( node );
    if ( oldpos==pos ) return true;

    TypeSet<EM::PosID> nodeonotherpatches;
    if ( autoconnect )
	findPos( node, nodeonotherpatches );

    const int auxdataindex = surface->indexOf( node );

    surface->setGridPos( node, pos );
    surface->setFillType( node, Geometry::GridSurface::Filled );

    if ( auxdataindex==-1 )
    {
	const int newauxdataindex = surface->indexOf( node );
	for ( int idx=0; idx<nrAuxData(); idx++ )
	{
	    if ( !auxdata[idx] ) continue;

	    TypeSet<float>* dataptr = (*auxdata[idx])[patchindex];
	    if ( !dataptr ) continue;

	    dataptr->insert( newauxdataindex, mUndefValue );
	}
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SetPosHistoryEvent( oldpos, pos,
				    EM::PosID(id(),patch,posid) );
	manager.history().addEvent( history, 0, 0 );
    }

    if ( !autoconnect ) return true;

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

    return true;
}


bool EM::Surface::setPos( const EM::PosID& posid, const Coord3& newpos,
			  bool addtohistory )
{
    if ( posid.emObject()!=id() ) return false;

    return setPos( posid.patchID(), posid.subID(), newpos, false,addtohistory);
}


Coord3 EM::Surface::getPos( const EM::PosID& posid ) const
{
    const int surfidx = patchids.indexOf( posid.patchID() );
    RowCol geomnode;
    if ( !getGridRowCol( posid.subID(), geomnode ) )
	return Coord3( mUndefValue, mUndefValue, mUndefValue );

    return surfaces[surfidx]->getGridPos( geomnode );
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
	EM::SubID subid = rowCol2SubID( rowcol );

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

	    EM::PosID posid( id(), patchids[idx], getSurfSubID(nodes[idy]) );

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
    const RowCol start = subID2RowCol(posid_.subID());
    neigbors += new TypeSet<EM::PosID>( 1, posid_ );

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	for ( int idz=0; idz<neigbors[idx]->size(); idz++ )
	{
	    EM::PosID currentposid = (*neigbors[idx])[idz];
	    const RowCol rowcol = subID2RowCol(currentposid.subID());

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
			    rowCol2SubID(neighborrowcol) );

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
    const RowCol rowcol = subID2RowCol(subid);
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


RowCol EM::Surface::loadedStep() const
{
    return loadedstep;
}


void EM::Surface::setTranslatorData( const RowCol& step_,
					const RowCol& loadedstep_,
					const RowCol& origo_,
					const Interval<int>* rowrange_,
					const Interval<int>* colrange_ )
{
    step = step_;
    loadedstep = loadedstep_;
    origo = origo_;
    delete rowinterval;
    delete colinterval;
    rowinterval = rowrange_ ? new Interval<int>( *rowrange_ ) : 0;
    colinterval = colrange_ ? new Interval<int>( *colrange_ ) : 0;
}


RowCol EM::Surface::subID2RowCol( const EM::SubID& subid )
{
    return long2rc(subid);
}


EM::SubID EM::Surface::rowCol2SubID( const RowCol& rc )
{
    return rc2long(rc);
}


bool EM::Surface::isFullResolution() const
{
    return loadedstep == step;
}


int EM::Surface::nrAuxData() const
{
    return auxdatanames.size();
}


const char* EM::Surface::auxDataName(int dataidx) const
{
    if ( auxdatanames[dataidx] )
	return *auxdatanames[dataidx];

    return 0;
}


int EM::Surface::addAuxData( const char* name )
{
    auxdatanames += new BufferString( name );
    ObjectSet<TypeSet<float> >* newauxdata = new ObjectSet<TypeSet<float> >;
    auxdata += newauxdata;
    newauxdata->allowNull(true);

    for ( int idx=0; idx<nrPatches(); idx++ )
	newauxdata += 0;

    return auxdatanames.size()-1;
}


void EM::Surface::removeAuxData( int dataidx )
{
    delete auxdatanames[dataidx];
    auxdatanames.replace( 0, dataidx );

    deepEraseArr( *auxdata[dataidx] );
    delete auxdata[dataidx];
    auxdata.replace( 0, dataidx );
}


float EM::Surface::getAuxDataVal( int dataidx, const EM::PosID& posid ) const
{
    if ( !auxdata[dataidx] ) return mUndefValue;
    const int patchidx = patchids.indexOf(posid.patchID());
    if ( patchidx==-1 ) return mUndefValue;

    const TypeSet<float>* patchauxdata = (*auxdata[dataidx])[patchidx];
    if ( !patchauxdata ) return mUndefValue;

    const int subidx = surfaces[patchidx]->indexOf(posid.subID());
    if ( subidx==-1 ) return mUndefValue;
    return (*patchauxdata)[subidx];
}


void EM::Surface::setAuxDataVal(int dataidx,const EM::PosID& posid, float val)
{
    if ( auxdata[dataidx] ) return;

    const int patchidx = patchids.indexOf(posid.patchID());
    if ( patchidx==-1 ) return;

    const int subidx = surfaces[patchidx]->indexOf(posid.subID());
    if ( subidx==-1 ) return;

    TypeSet<float>* patchauxdata = (*auxdata[dataidx])[patchidx];
    if ( !patchauxdata )
    {
	const int sz = surfaces[patchidx]->size();
	auxdata[dataidx]->replace( new TypeSet<float>(sz,mUndefValue),patchidx);
	patchauxdata = (*auxdata[dataidx])[patchidx];
    }

    (*patchauxdata)[subidx] = val;
}


bool EM::Surface::getGridRowCol( const EM::SubID& posid,
			     RowCol& nodeid ) const
{
    const RowCol emrowcol = subID2RowCol(posid) - origo;
    if ( emrowcol.row%loadedstep.row || emrowcol.col%loadedstep.col )
	return false;

    nodeid = emrowcol/loadedstep;
    return true;
}


EM::SubID EM::Surface::getSurfSubID( const RowCol& nodeid ) const
{
    return rowCol2SubID( origo+nodeid*loadedstep );
}


EM::SubID EM::Surface::getSurfSubID( const Geometry::PosID& gposid ) const
{
    const RowCol& nodeid = Geometry::GridSurface::getGridNode(gposid);
    return getSurfSubID( nodeid );
}



const Geometry::GridSurface* EM::Surface::getSurface( PatchID patchid )const
{
    const int idx = patchids.indexOf( patchid );
    return idx==-1 ? 0 : surfaces[idx];
}



void EM::Surface::cleanUp()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepErase( *auxdata[idx] );
    }

    deepErase( auxdata );

    deepErase( surfaces );
    deepErase( patchnames );
    patchids.erase();

    delete rowinterval;
    delete colinterval;
    rowinterval = 0;
    colinterval = 0;
}


void EM::Surface::getRange( StepInterval<int>& rg, bool rowdir ) const
{
    const int nrpatches = nrPatches();
    for ( int idx=0; idx<nrpatches; idx++ )
    {
	const EM::PatchID patchid = patchID( idx );
	const Geometry::GridSurface& gsurf = *getSurface( patchid );

	const Interval<int> colint( gsurf.getColInterval() );
	const RowCol firstrowcol =
	    subID2RowCol( getSurfSubID( RowCol(gsurf.firstRow(),colint.start)));
	const RowCol lastrowcol =
	    subID2RowCol( getSurfSubID( RowCol(gsurf.lastRow(),colint.stop) ) );
	if ( !idx )
	{
	    rg.start = rowdir ? firstrowcol.row : firstrowcol.col;
	    rg.stop = rowdir ? lastrowcol.row : lastrowcol.col;
	}
	else
	{
	    if ( rowdir )
		{ rg.include( firstrowcol.row ); rg.include( lastrowcol.row ); }
	    else
		{ rg.include( firstrowcol.col ); rg.include( lastrowcol.col ); }
	}
    }

    rg.step = rowdir ? loadedStep().row : loadedStep().col;
}
