/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "emsurface.h"

#include "cubesampling.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "filepath.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "posfilter.h"
#include "surv2dgeom.h"


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sSections = "Patches";


using namespace EM;


void SurfaceIOData::clear()
{
    rg.init(false);
    dbinfo = "";
    deepErase(valnames);
    deepErase(sections);
    deepErase(linenames);
    deepErase(linesets);
    trcranges.erase();
}


void SurfaceIOData::use( const Surface& surf )
{
    clear();

    mDynamicCastGet( const RowColSurfaceGeometry*, rcsg, &surf.geometry() );
    if ( rcsg )
    {
	StepInterval<int> hrg = rcsg->rowRange();
	rg.start.inl = hrg.start; rg.stop.inl = hrg.stop;
	rg.step.inl = hrg.step;
	hrg = rcsg->colRange();
	rg.start.crl = hrg.start; rg.stop.crl = hrg.stop;
	rg.step.crl = hrg.step;
    }

    for ( int idx=0; idx<surf.nrSections(); idx++ )
	sections += new BufferString( surf.sectionName( surf.sectionID(idx) ) );

    mDynamicCastGet(const Horizon3D*,horizon,&surf);
    if ( horizon )
    {
	for ( int idx=0; idx<horizon->auxdata.nrAuxData(); idx++ )
	    valnames += new BufferString( horizon->auxdata.auxDataName(idx) );
    }
   
    linenames.erase();
    linesets.erase();
    trcranges.erase();
    mDynamicCastGet(const Horizon2D*,horizon2d,&surf);
    if ( horizon2d )
    {
	const Horizon2DGeometry& emgeom = horizon2d->geometry();
	for ( int idx=0; idx<emgeom.nrLines(); idx++ )
	{
	    const PosInfo::GeomID geomid = emgeom.lineGeomID( idx );
	    linesets.add( S2DPOS().getLineSet(geomid.lsid_) );
	    S2DPOS().setCurLineSet( geomid.lsid_ );
	    linenames.add( S2DPOS().getLineName(geomid.lineid_) );
	    const Geometry::Horizon2DLine* geom =
		emgeom.sectionGeometry( emgeom.sectionID(0) );
	    trcranges += geom->colRange( geom->getRowIndex(geomid) );
	}
    }
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
    
    for ( int idx=0; idx<linesets.size(); idx++ )
    {
	PosInfo::GeomID geomid = S2DPOS().getGeomID( linesets.get(idx),
						     linenames.get(idx) );
	if ( !geomid.isOK() )
	    continue;

	BufferString key = IOPar::compKey( "Line", idx );
	iopar.set( IOPar::compKey(key,Horizon2DGeometry::sKeyID()),
		   geomid.toString() );
	iopar.set( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		   trcranges[idx] );
    }
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

    if ( iopar.find(Horizon2DGeometry::sKeyNrLines()) )
    {
	int nrlines = 0;
	iopar.get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( "Line", idx );

	    BufferString idstr;
	    iopar.get( IOPar::compKey(key,Horizon2DGeometry::sKeyID()), idstr );
	    PosInfo::GeomID geomid; geomid.fromString( idstr );
	    linesets.add( S2DPOS().getLineSet(geomid.lsid_) );

	    StepInterval<int> trcrange;
	    iopar.get( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		       trcrange );
	    trcranges += trcrange;
	}

	return;
    }

    IOPar* linenamespar = iopar.subselect( Horizon2DGeometry::sKeyLineNames() );
    if ( linenamespar ) linenames.usePar( *linenamespar );

    TypeSet<int> lineids;
    if ( iopar.get( Horizon2DGeometry::sKeyLineIDs(), lineids) )
    {
	for ( int idx=0; idx<lineids.size(); idx++ )
	{
	    BufferString linesetkey = Horizon2DGeometry::sKeyLineSets();
	    BufferString trcrangekey = Horizon2DGeometry::sKeyTraceRange();
	    linesetkey += idx;
	    trcrangekey += idx;
	    StepInterval<int> trcrange;
	    if ( iopar.get(trcrangekey,trcrange) )
		trcranges += trcrange;
	    MultiID linesetid;
	    if ( iopar.get(linesetkey,linesetid) )
	    {
		IOObj* ioobj = IOM().get( linesetid );
		linesets.add( ioobj->name() );
	    }
	}
    }
}


void SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    seltrcranges = sd.trcranges;
    selvalues.erase(); selsections.erase();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
    for ( int idx=0; idx<sd.sections.size(); idx++ )
	selsections += idx;
}


Surface::Surface( EMManager& man)
    : EMObject( man )
{
}


Surface::~Surface()
{}

void Surface::removeAll() {}

int Surface::nrSections() const
{ return geometry().nrSections(); }

SectionID Surface::sectionID( int idx ) const
{ return geometry().sectionID(idx); }

BufferString Surface::sectionName( const SectionID& sid ) const
{ return geometry().sectionName(sid); }

bool Surface::canSetSectionName() const { return true; }

bool Surface::setSectionName( const SectionID& sid, const char* nm, bool hist )
{ return geometry().setSectionName(sid,nm,hist); }

bool Surface::removeSection( SectionID sid, bool hist )
{ return geometry().removeSection( sid, hist ); }

bool Surface::isAtEdge( const PosID& posid ) const
{ return geometry().isAtEdge(posid); }

bool Surface::isLoaded() const
{ return geometry().isLoaded(); }

Executor* Surface::saver() { return geometry().saver(); }

Executor* Surface::loader() { return geometry().loader(); }

Geometry::Element* Surface::sectionGeometryInternal( const SectionID& sid )
{ return geometry().sectionGeometry(sid); }

EMObjectIterator* Surface::createIterator( const SectionID& sid, 
					   const CubeSampling* cs ) const
{ return geometry().createIterator( sid, cs ); }

bool Surface::enableGeometryChecks( bool nv )
{ return geometry().enableChecks( nv ); }

bool Surface::isGeometryChecksEnabled() const
{ return geometry().isChecksEnabled(); }

const SurfaceGeometry& Surface::geometry() const
{ return const_cast<Surface*>(this)->geometry(); }


void Surface::apply( const Pos::Filter& pf )
{
    PtrMan<EM::EMObjectIterator>iterator = createIterator( -1 );
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	const Coord3 pos = getPos( pid ); 
	if ( !pf.includes( (Coord) pos, (float) pos.z) )
	   unSetPos( pid, false );
    }
}


BufferString Surface::getParFileName( const IOObj& ioobj )
{
    FilePath fp( ioobj.fullUserExpr(true) );
    fp.setExtension( "par" );
    return fp.fullPath();
}


BufferString Surface::getSetupFileName( const IOObj& ioobj )
{
    FilePath fp( ioobj.fullUserExpr(true) );
    fp.setExtension( "ts" );
    return fp.fullPath();
}


bool Surface::usePar( const IOPar& par )
{
    return EMObject::usePar(par) && geometry().usePar(par);
}


void Surface::fillPar( IOPar& par ) const
{
    EMObject::fillPar( par );
    geometry().fillPar( par );
}
