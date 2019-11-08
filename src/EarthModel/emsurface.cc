/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "emsurface.h"

#include "trckeyzsampling.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "posfilter.h"
#include "posinfo2dsurv.h"


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";


namespace EM
{

void SurfaceIOData::clear()
{
    rg.init(false);
    dbinfo = "";
    valnames.setEmpty();
    linenames.setEmpty();
    linesets.setEmpty();
    geomids.setEmpty();
    trcranges.setEmpty();
}


void SurfaceIOData::use( const Surface& surf )
{
    clear();

    mDynamicCastGet( const RowColSurfaceGeometry*, rcsg, &surf.geometry() );
    if ( rcsg )
    {
	StepInterval<int> hrg = rcsg->rowRange();
	rg.start_.inl() = hrg.start; rg.stop_.inl() = hrg.stop;
	rg.step_.inl() = hrg.step;
	hrg = rcsg->colRange();
	rg.start_.crl() = hrg.start; rg.stop_.crl() = hrg.stop;
	rg.step_.crl() = hrg.step;
    }

    mDynamicCastGet(const Horizon3D*,horizon,&surf);
    if ( horizon )
    {
	for ( int idx=0; idx<horizon->auxdata.nrAuxData(); idx++ )
	    valnames.add( horizon->auxdata.auxDataName(idx) );
    }

    linenames.setEmpty();
    linesets.setEmpty();
    geomids.setEmpty();
    trcranges.setEmpty();
    mDynamicCastGet(const Horizon2D*,horizon2d,&surf);
    if ( horizon2d )
    {
	const Horizon2DGeometry& emgeom = horizon2d->geometry();
	for ( int idx=0; idx<emgeom.nrLines(); idx++ )
	{
	    const Pos::GeomID geomid = emgeom.geomID( idx );
	    linenames.add( emgeom.lineName(idx) );
	    geomids += geomid;
	    trcranges += emgeom.colRange( geomid );
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

    int nrlines = geomids.size();
    if ( !nrlines ) nrlines = linenames.size();
    iopar.set( Horizon2DGeometry::sKeyNrLines(), nrlines );
    for ( int idx=0; idx<nrlines; idx++ )
    {
	if ( geomids.validIdx(idx) && geomids[idx].isValid() )
	    iopar.set( IOPar::compKey(sKey::GeomID(),idx), geomids[idx] );
	else if ( linesets.validIdx(idx) && linenames.validIdx(idx) )
	{
	    const BufferString oldlnm( linesets.get(idx), "-",
				       linenames.get(idx) );
	    const Pos::GeomID geomid = SurvGeom::getGeomID( oldlnm );
	    if ( geomid.isValid() )
		iopar.set( IOPar::compKey(sKey::GeomID(),idx), geomid );
	}

	BufferString key = IOPar::compKey( "Line", idx );
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

    if ( iopar.find(Horizon2DGeometry::sKeyNrLines()) )
    {
	int nrlines = 0;
	iopar.get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( "Line", idx );
	    Pos::GeomID geomid;
	    if ( !iopar.get(IOPar::compKey(sKey::GeomID(),idx),geomid) )
	    {
		BufferString idstr;
		if ( iopar.get(IOPar::compKey(key,Horizon2DGeometry::sKeyID()),
			       idstr) )
		{
		    PosInfo::Line2DKey l2dkey; l2dkey.fromString( idstr );
		    if ( S2DPOS().curLineSetID() != l2dkey.lsID() )
			S2DPOS().setCurLineSet( l2dkey.lsID() );
		    const BufferString oldlnm(
			    S2DPOS().getLineSet(l2dkey.lsID()), "-",
				S2DPOS().getLineName(l2dkey.lineID()) );
		    geomid = Survey::Geometry::getGeomID( oldlnm );
		}
	    }

	    if ( !geomid.isValid() )
		continue;

	    geomids += geomid;
	    linenames.add( geomid.name() );
	    StepInterval<int> trcrange;
	    iopar.get( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		       trcrange );
	    trcranges += trcrange;
	}
    }
}


void SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    seltrcranges = sd.trcranges;
    selvalues.setEmpty();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
}


Surface::Surface( const char* nm )
    : Object(nm)
    , shift_(0)
{
}


Surface::Surface( const Surface& oth )
    : Object(oth.name())
    , shift_(0)
{
    *this = oth;
}


mImplMonitorableAssignment(Surface,Object);

Surface::~Surface()
{}

void Surface::removeAll() {}

void Surface::copyClassData( const Surface& oth )
{ geometry() = oth.geometry(); }

Monitorable::ChangeType Surface::compareClassData( const Surface& oth ) const
{ return cNoChange(); }

bool Surface::isAtEdge( const PosID& posid ) const
{ return geometry().isAtEdge(posid); }

bool Surface::isLoaded() const
{ return geometry().isLoaded(); }

Executor* Surface::saver() { return geometry().saver(); }

Executor* Surface::loader() { return geometry().loader(); }

ObjectIterator* Surface::createIterator( const TrcKeyZSampling* cs ) const
{ return geometry().createIterator( cs ); }

bool Surface::enableGeometryChecks( bool nv )
{ return geometry().enableChecks( nv ); }

bool Surface::isGeometryChecksEnabled() const
{ return geometry().isChecksEnabled(); }

const SurfaceGeometry& Surface::geometry() const
{ return const_cast<Surface*>(this)->geometry(); }

const Geometry::Element* Surface::geometryElement() const
{ return geometry().geometryElement(); }

Geometry::Element* Surface::geometryElement()
{ return geometry().geometryElement(); }


void Surface::apply( const Pos::Filter& pf )
{
    PtrMan<EM::ObjectIterator>iterator = createIterator();
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.isInvalid() )
	    break;

	const Coord3 pos = getPos( pid );
	if ( !pf.includes(pos.getXY(),(float)pos.z_) )
	   unSetPos( pid, false );
    }
}


BufferString Surface::getParFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    File::Path fp( ioobj.mainFileName() );
    fp.setExtension( sParFileExtension() );
    return fp.fullPath();
}


BufferString Surface::getSetupFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    File::Path fp( ioobj.mainFileName() );
    fp.setExtension( "ts" );
    return fp.fullPath();
}


BufferString Surface::getParentChildFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    File::Path fp( ioobj.mainFileName() );
    fp.setExtension( "pcd" );
    return fp.fullPath();
}


float Surface::getShift() const
{
    return shift_;
}


void Surface::setShift( float shift )
{
    mLock4Write();
    shift_ = shift;
    touch();
}


bool Surface::usePar( const IOPar& par )
{
    return Object::usePar(par) && geometry().usePar(par);
}


void Surface::fillPar( IOPar& par ) const
{
    Object::fillPar( par );
    geometry().fillPar( par );
}

} // namespace EM
