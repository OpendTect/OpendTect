/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emsurface.cc,v 1.70 2005-03-10 11:48:21 cvskris Exp $
________________________________________________________________________

-*/

#include "emsurface.h"
#include "emsurfaceiodata.h"
#include "emsurfacerelations.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"

#include "cubesampling.h"
#include "emsurfaceedgeline.h"
#include "emmanager.h"
#include "iopar.h"


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

    StepInterval<int> hrg = surf.geometry.rowRange();
    rg.start.inl = hrg.start; rg.stop.inl = hrg.stop;
    rg.step.inl = hrg.step;
    hrg = surf.geometry.colRange();
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
{
}


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


int Surface::nrSections() const
{ return geometry.nrSections(); }


SectionID Surface::sectionID( int idx ) const
{ return geometry.sectionID(idx); }


bool Surface::setPos( const PosID& posid, const Coord3& newpos,
		      bool addtohistory )
{
    if ( posid.objectID()!=id() ) return false;

    return geometry.setPos( posid.sectionID(),
	    		    geometry.subID2RowCol(posid.subID()),
			    newpos, addtohistory );
}


bool Surface::isDefined( const PosID& pos ) const
{ return geometry.isDefined(pos); }


Coord3 Surface::getPos( const PosID& posid ) const
{
    return geometry.getPos( posid.sectionID(),
	    		    geometry.subID2RowCol(posid.subID()) );
}


bool Surface::isChanged(int idx) const
{ return geometry.isChanged(idx); }
//{ return auxdata.isChanged(idx) || geometry.isChanged(idx); }


void Surface::resetChangedFlag()
{
    geometry.resetChangedFlag();
    auxdata.resetChangedFlag();
}


bool Surface::isLoaded() const
{ return geometry.isLoaded(); }


Executor* Surface::saver() { return geometry.saver(); }


Executor* Surface::loader() { return geometry.loader(); }


const Geometry::Element* Surface::getElement( SectionID sid ) const
{ return reinterpret_cast<const Geometry::Element*>(geometry.getSurface(sid)); }


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
