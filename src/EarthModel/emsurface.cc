/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurface.cc,v 1.64 2004-08-09 14:09:31 kristofer Exp $";

#include "emsurface.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "emsurfacerelations.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "emhingeline.h"
#include "emhistoryimpl.h"
#include "emmanager.h"
#include "geomgridsurface.h"
#include "geommeshsurface.h"
#include "grid.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "linsolv.h"
#include "pca.h"
#include "toplist.h"
#include "ptrman.h"
#include "survinfo.h"
#include "settings.h"


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sSections = "Patches";


namespace EM
{

void SurfaceIOData::clear()
{
    dbinfo = "";
    deepErase(valnames);
    deepErase(sections);
}

void SurfaceIOData::use( const Surface& surf )
{
    clear();

    StepInterval<int> hrg;
    surf.geometry.getRange( hrg, true );
    rg.start.inl = hrg.start; rg.stop.inl = hrg.stop;
    rg.step.inl = hrg.step;
    surf.geometry.getRange( hrg, false );
    rg.start.crl = hrg.start; rg.stop.crl = hrg.stop;
    rg.step.crl = hrg.step;

    for ( int idx=0; idx<surf.geometry.nrSections(); idx++ )
	sections +=
	    new BufferString( surf.geometry.sectionName(
			surf.geometry.sectionID(idx) ) );

    for ( int idx=0; idx<surf.auxdata.nrAuxData(); idx++ )
	valnames += new BufferString( surf.auxdata.auxDataName(idx) );
}


void SurfaceIOData::fillPar( IOPar& iopar ) const
{
    iopar.set( sDbInfo, dbinfo );

    IOPar bidpar;
    rg.fillPar( bidpar );
    iopar.mergeComp( bidpar, sRange );

    IOPar valnmspar;
    valnames.fillPar( valnmspar );
    iopar.mergeComp( valnmspar, sValnms );

    IOPar sectionpar;
    sections.fillPar( sectionpar );
    iopar.mergeComp( sectionpar, sSections );
}


void SurfaceIOData::usePar( const IOPar& iopar )
{
    iopar.get( sDbInfo, dbinfo );

    IOPar* bidpar = iopar.subselect(sRange);
    if ( bidpar ) rg.usePar( *bidpar );

    IOPar* valnmspar = iopar.subselect(sValnms);
    if ( valnmspar ) valnames.usePar( *valnmspar );

    IOPar* sectionpar = iopar.subselect(sSections);
    if ( sectionpar ) sections.usePar( *sectionpar );
}


void SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    selvalues.erase(); selsections.erase();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
    for ( int idx=0; idx<sd.sections.size(); idx++ )
	selsections += idx;
}


Surface::Surface( EMManager& man, const ObjectID& id_,
		      SurfaceGeometry& geom)
    : EMObject( man, id_ )
    , relations( *new SurfaceRelations(*this ) )
    , edgelinesets( *new EdgeLineManager(*this) )
    , geometry( geom )
    , auxdata( *new SurfaceAuxData(*this) )
{ }


Surface::~Surface()
{
    cleanUp();
    delete &relations;
    delete &edgelinesets;
    delete &geometry;
    delete &auxdata;
}


void Surface::cleanUp()
{
    auxdata.removeAll();
    geometry.removeAll();
    relations.removeAll();
    edgelinesets.removeAll();
}


bool Surface::setPos( const PosID& posid, const Coord3& newpos,
			  bool addtohistory )
{
    if ( posid.objectID()!=id() ) return false;

    return geometry.setPos( posid.sectionID(),
	    		    geometry.subID2RowCol(posid.subID()),
			    newpos, false, addtohistory );
}


bool Surface::isDefined( const PosID& pos ) const
{ return geometry.isDefined(pos); }


Coord3 Surface::getPos( const PosID& posid ) const
{
    return geometry.getPos( posid.sectionID(),
	    		    geometry.subID2RowCol(posid.subID()) );
}


bool Surface::isChanged(int idx) const
{ return auxdata.isChanged(idx) || geometry.isChanged(idx); }


void Surface::resetChangedFlag()
{
    geometry.resetChangedFlag();
    auxdata.resetChangedFlag();
}


bool Surface::isLoaded() const
{ return geometry.isLoaded(); }


bool Surface::usePar( const IOPar& par )
{
    return EMObject::usePar(par) && geometry.usePar(par) &&
	   relations.usePar(par) && edgelinesets.usePar(par);
}


void Surface::fillPar( IOPar& par ) const
{
    EMObject::fillPar(par);
    relations.fillPar(par);
    edgelinesets.fillPar(par);
    geometry.fillPar(par);
}

}; //namespace
