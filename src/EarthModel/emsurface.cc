/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emsurface.cc,v 1.77 2005-09-14 08:15:38 cvskris Exp $
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


using namespace EM;

class SurfaceIterator : public EMObjectIterator
{
public:
    		SurfaceIterator( const Surface& surf,
				 const SectionID& sectionid )
		    : surface( surf )
		    , sid( sectionid )
		    , inited( false )
		    , allsids( sectionid==-1 )
		{
		    if ( allsids )
			sid = surface.sectionID(0);
		}

    PosID	next()
		{
		    if ( !inited )
		    {
			if ( !initSection() )
			    return PosID(-1,-1,-1);
		    }
		    else
		    {
			rc.col += colrg.step;
			if ( !colrg.includes(rc.col) )
			{
			    rc.col = colrg.start;
			    rc.row += rowrg.step;
			    if ( !rowrg.includes(rc.row) )
			    {
				inited = false;
				if ( !nextSection() )
				    return PosID(-1,-1,-1);
			    }
			}
		    }

		    if ( !surface.geometry.isDefined(sid,rc) )
			return next();


		    return PosID(surface.id(),sid,rc.getSerialized());
		}

    int		maximumSize() const
		{
		    if ( allsids )
		    {
			int sum = 0;
			for ( int idx=0; idx<surface.nrSections(); idx++ )
			    sum += maximumSize(surface.sectionID(idx));
			return sum;
		    }

		    return maximumSize(sid);
		}

    int		maximumSize( const SectionID& sid ) const
		{
		    return (surface.geometry.rowRange(sid).nrSteps()+1) *
			   (surface.geometry.colRange(sid).nrSteps()+1);
		}

protected:
			

    bool	initSection()
		{
		    while ( true )
		    {
			rowrg = surface.geometry.rowRange(sid);
			colrg = surface.geometry.colRange(sid);


/*
 * 			TODO: Don't know if this is neccessary. The check was
 * 			commented out since single node surfaces does not work.
			if ( rowrg.start==rowrg.stop &&
			     colrg.start==colrg.stop )
			    return nextSection();
			    */

			rc.row = rowrg.start;
			rc.col = colrg.start;
			inited = true;
			break;
		    }

		    return true;
		}

    bool	nextSection()
		{
		    if ( !allsids ) return false;

		    inited = false;
		    int idx=0;
		    for ( ; idx<surface.nrSections(); idx++ )
		    {
			if ( surface.sectionID(idx)==sid )
			    break;
		    }

		    if ( idx<surface.nrSections()-1 )
		    {
			sid = surface.sectionID(++idx);
			return initSection();
		    }

		    return false;
		}

    RowCol		rc;
    SectionID		sid;
    StepInterval<int>	rowrg;
    StepInterval<int>	colrg;
    bool		inited;

    const bool		allsids;
    const Surface&	surface;
};



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


BufferString Surface::sectionName( const SectionID& sid ) const
{
    BufferString res = geometry.sectionName(sid);
    return res;
}


bool Surface::canSetSectionName() const { return true; }


bool Surface::setSectionName( const SectionID& sid, const char* nm, bool hist )
{ return geometry.setSectionName(sid,nm,hist); }


bool Surface::removeSection( SectionID sid, bool hist )
{
    geometry.removeSection( sid, hist );
    return true;
}


bool Surface::isAtEdge( const PosID& posid ) const
{ return geometry.isAtEdge(posid); }


bool Surface::isChanged(int idx) const
{ return EMObject::isChanged(idx); }
//{ return auxdata.isChanged(idx) || geometry.isChanged(idx); }


void Surface::resetChangedFlag(int idx)
{
    EMObject::resetChangedFlag(idx);
    auxdata.resetChangedFlag();
}


bool Surface::isLoaded() const
{ return geometry.isLoaded(); }


Executor* Surface::saver() { return geometry.saver(); }


Executor* Surface::loader() { return geometry.loader(); }


Geometry::Element* Surface::getElementInternal( SectionID sid )
{
    const Geometry::Element* elem =
	(const Geometry::Element*) geometry.getSurface(sid);
    return const_cast<Geometry::Element*>(elem);
}


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


EMObjectIterator* Surface::createIterator( const SectionID& sid ) const
{
    return new SurfaceIterator( *this, sid );
}
