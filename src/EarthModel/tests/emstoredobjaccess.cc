/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "testprog.h"

#include "embody.h"
#include "emfault.h"
#include "emfaultstickset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emstoredobjaccess.h"
#include "horizonsorter.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "arrayndimpl.h"
#include "ioman.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "survinfo.h"


#define valTest( val, expval, defeps, desc ) \
{ \
    errmsg.set( "Expected: " ).add( toStringPrecise(expval) ) \
	  . add( "; Retrieved: " ).add( toStringPrecise(val) ); \
    mRunStandardTestWithError(mIsEqual(val,expval,defeps),desc,errmsg.str()); \
}


static bool testEMStoredObjAccess()
{
    EM::StoredObjAccess soa;
    mRunStandardTest( soa.add( MultiID(100020,2) ),
	      "EM::StoredObjAccess: Add Horizon 3D (1-Top)" );
    mRunStandardTest( soa.add( MultiID(100020,21) ),
	      "EM::StoredObjAccess: Add Horizon 2D (2D Demo0 - FS4)" );
    mRunStandardTest( soa.add( MultiID(100020,5) ),
	      "EM::StoredObjAccess: Add FaultStickSet (SSIS-Grid-Faultsticks)");
    mRunStandardTest( soa.add( MultiID(100020,9) ),
	      "EM::StoredObjAccess: Add Fault (SingleStickPickedOn426)" );
    mRunStandardTest( soa.add( MultiID(100020,4) ),
	      "EM::StoredObjAccess: Add Geobody (Slumps-2b)" );
    mRunStandardTest( !soa.add( MultiID(100020,99795) ),
	      "EM::StoredObjAccess: Add non-existing MultiID should fail" );
    soa.dismiss( MultiID(100020,99795) );

    PtrMan<Executor> exec = soa.reader();
    TextTaskRunner ttr( tstStream() );
    const bool res = ttr.execute( *exec.ptr() );
    mRunStandardTestWithError( res, exec->name(), ::toString(soa.getError()) );
    exec = nullptr;

    mDynamicCastGet(const EM::Horizon3D*,hor3d,soa.object(0))
    mRunStandardTest(hor3d,"object(0) must be an Horizon3D")
    mDynamicCastGet(const EM::Horizon2D*,hor2d,soa.object(1))
    mRunStandardTest(hor2d,"object(1) must be an Horizon2D")
    mDynamicCastGet(const EM::FaultStickSet*,fss,soa.object(2))
    mRunStandardTest(fss,"object(2) must be a FaultStickSet")
    mDynamicCastGet(const EM::Fault*,flt,soa.object(3))
    mRunStandardTest(flt,"object(3) must be a Fault")
    mDynamicCastGet(const EM::Body*,body,soa.object(4))
    mRunStandardTest(body,"object(4) must be a Geobody")

    BufferString errmsg;
    const Coord crd_ix_400_900( 620620, 6081472 );
    float zval = hor3d->getZValue( crd_ix_400_900 );
    valTest( zval, 0.549256265, 1e-6f, "Horizon 3D Z Value check" );

    const Coord crd_lt_Line_900( 619665.70, 6088541.50 );
    zval = hor2d->getZValue( crd_lt_Line_900 );
    valTest( zval, 0.92633921, 1e-6f, "Horizon 2D Z Value check" );

    const EM::FaultStickSetGeometry& fssgeom = fss->geometry();
    mRunStandardTest( fssgeom.nrSticks() == 4 && fssgeom.nrKnots(0) == 7,
		      "Check nr Sticks and knots of Stick in FaultStickSet" );

    const EM::FaultGeometry& fltgeom = flt->geometry();
    getNonConst(fltgeom).selectAllSticks();
    mRunStandardTest( fltgeom.nrSelectedSticks() == 2,
		      "Check nr Sticks and knots of Stick in Fault" );

    TrcKeyZSampling cs;
    mRunStandardTest( body->getBodyRange(cs),"Get body ranges")
    mRunStandardTest( cs.hsamp_.start_.inl()>390 && cs.hsamp_.stop_.crl()<870,
		      "Check on body ranges" )

    return true;
}


static bool testHorizonSorting( bool is2d )
{
    TypeSet<MultiID> inputids;
    if ( is2d )
    {
	inputids += MultiID(100020,82); // Deep
	inputids += MultiID(100020,31); // Base
	inputids += MultiID(100020,32); // Top
	inputids += MultiID(100020,80); // Shallow
    }
    else
    {
	inputids += MultiID(100020,7); // Flat at 1100ms
	inputids += MultiID(100020,3); // Base horizon
	inputids += MultiID(100020,2); // Top horizon
	inputids += MultiID(100020,6); // Flat at 1000ms
    }

    HorizonSorter sorter( inputids, is2d );
    TextTaskRunner ttr( tstStream() );
    const bool res = ttr.execute( sorter );
    mRunStandardTestWithError( res,
		BufferString("Sorting ", is2d ? "2D" : "3D", " horizons"),
		toString(sorter.uiMessage()) );

    TypeSet<MultiID> sortedids;
    sorter.getSortedList( sortedids );
    mRunStandardTest( sortedids.size() == inputids.size(),
		      "Number of sorted horizons" );
    if ( is2d )
    {
	mRunStandardTest( sortedids[0] == MultiID(100020,80) &&
			  sortedids[1] == MultiID(100020,32) &&
			  sortedids[2] == MultiID(100020,31) &&
			  sortedids[3] == MultiID(100020,82),
			  "2D Horizons are properly sorted" );
    }
    else
    {
	mRunStandardTest( sortedids[0] == MultiID(100020,2) &&
			  sortedids[1] == MultiID(100020,3) &&
			  sortedids[2] == MultiID(100020,6) &&
			  sortedids[3] == MultiID(100020,7),
			  "3D Horizons are properly sorted" );
    }

    return true;
}


static RefMan<EM::Horizon3D> getHorizon3D( const char* horname, bool create,
					 BufferString& errmsg )
{
    EM::EMObject* horobj;
    if ( create )
    {
	const EM::ObjectID horid = EM::EMM().createObject(
					    EM::Horizon3D::typeStr(), horname );
	horobj = EM::EMM().getObject( horid );
    }
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( horname,
				  EMHorizon3DTranslatorGroup::sGroupName() );
	if ( !ioobj )
	{
	    errmsg = toString( IOM().uiMessage() );
	    return nullptr;
	}
	horobj = EM::EMM().loadIfNotFullyLoaded( ioobj->key() );
    }

    mDynamicCastGet(EM::Horizon3D*,hor3d,horobj)
    return hor3d;
}


static bool removeHorizon3D( const char* hornm )
{
    PtrMan<IOObj> ioobj = IOM().get( hornm,
				     EMHorizon3DTranslatorGroup::sGroupName() );
    mRunStandardTestWithError( ioobj && IOM().to(ioobj->key()) &&
			       IOM().implRemove(ioobj->key(), true),
			      "Remove horizon", toString(IOM().uiMessage()) );

    return true;
}


static bool createHorizon3D( const char* hornm )
{
    BufferString errmsg;
    RefMan<EM::Horizon3D> newhor = getHorizon3D( hornm, true, errmsg );
    mRunStandardTest( newhor.ptr(), "Create 3D horizon" );

    const ZDomain::Info* zdom = &newhor->zDomain();
    const float zfac = zdom && zdom->isTime() ?
					    (float) zdom->userFactor() : 1.f;
    const TrcKeySampling tks = SI().sampling( false ).hsamp_;
    PtrMan<Array2D<float>> arr = new Array2DImpl<float>( tks.nrInl(),
							 tks.nrCrl() );
    for ( int i=0; i<arr->info().getSize(0); i++ )
    {
	for ( int j=0; j<arr->info().getSize(1); j++ )
	{
	    const float val = float(i+j) / zfac;
	    arr->set( i, j, val );
	}
    }

    if ( newhor->setArray2D(arr.ptr(), tks.start_, tks.step_, false) )
    {
	newhor->setFullyLoaded( true );
	PtrMan<Executor> saver = newhor->saver();
	mRunStandardTest( saver && TaskRunner::execute(nullptr, *saver),
			  "Save 3D horizon" );
    }

    const BufferString attribnm( "attrib_1" ) ;
    int auxidx = newhor->auxdata.auxDataIndex( attribnm.buf() );
    if ( auxidx==-1 )
	auxidx = newhor->auxdata.addAuxData( attribnm.buf() );

    for (int i=0; i<arr->info().getSize(0); i++ )
    {
	for ( int j=0; j<arr->info().getSize(1); j++ )
	{
	    const TrcKey trckey = tks.trcKeyAt( i, j );
	    newhor->auxdata.setAuxDataVal( auxidx, trckey,
					   -arr->get(i,j)*zfac );
	}
    }
    PtrMan<Executor> auxsaver = newhor->auxdata.auxDataSaver( auxidx, true );
    mRunStandardTest( auxsaver && TaskRunner::execute( nullptr, *auxsaver ),
		      "Save 3D horizon data" );

    return true;
}


static bool testHorizon3D( const char* hornm )
{
    BufferString errmsg;
    RefMan<EM::Horizon3D> hor3d = getHorizon3D( hornm, false, errmsg );
    mRunStandardTestWithError( hor3d.ptr(), "Read 3D horizon", errmsg.str() );

    PtrMan<Array2D<float>> arr = hor3d->createArray2D();
    mRunStandardTest( arr, "Retrieve 3D horizon Z array" );

    const ZDomain::Info* zdom = &hor3d->zDomain();
    const float zfac = zdom && zdom->isTime() ?
					    (float) zdom->userFactor() : 1.f;
    bool allclose = true;
    for ( int i=0; i<arr->info().getSize(0); i++ )
    {
	for ( int j=0; j<arr->info().getSize(1); j++ )
	{
	    if ( !mIsEqual(arr->get(i, j)*zfac, (i+j), 0.01) )
	    {
		allclose = false;
		errmsg = "expected: ";
		errmsg.add(i+j)
		      .add(" but got: ")
		      .add(arr->get(i,j)*zfac)
		      .add(" at: ")
		      .add(i).add(", ").add(j);
		break;
	    }
	}
	if ( !allclose )
	    break;
    }
    mRunStandardTestWithError( allclose, "Check 3D horizon Z values",
			       errmsg.str() );

    PtrMan<Executor> auxloader = hor3d->auxdata.auxDataLoader( "attrib_1" );
    mRunStandardTest( auxloader && TaskRunner::execute(nullptr, *auxloader),
		      "Read 3D horizon data" );

    mRunStandardTest( hor3d->auxdata.hasAuxDataName("attrib_1"),
		      "Check horizon has attribute" );

    int iaux = hor3d->auxdata.auxDataIndex( "attrib_1" );
    PtrMan<Array2D<float>> auxarr = hor3d->auxdata.createArray2D( iaux );
    mRunStandardTest( auxarr, "Retrieve 3D horizon data array");

    allclose = true;
    for ( int i=0; i<arr->info().getSize(0); i++ )
    {
	for ( int j=0; j<arr->info().getSize(1); j++ )
	{
	    if ( !mIsEqual(arr->get(i,j)*zfac,-auxarr->get(i,j), 0.01) )
	    {
		allclose = false;
		errmsg = "expected: ";
		errmsg.add(-arr->get(i,j)*zfac)
		      .add(" but got: ").add(auxarr->get(i,j))
		      .add(" at: ").add(i).add(", ").add(j);
		break;
	    }
	}
	if ( !allclose )
	    break;
    }
    mRunStandardTestWithError( allclose, "Check 3D horizon data values",
			       errmsg.str() );

    return true;
}


mLoad1Module("EarthModel")

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();

    const BufferString hor3dnm( "test_horizon3d" );
    if ( !testEMStoredObjAccess() ||
	 !testHorizonSorting(false) ||
	 !testHorizonSorting(true) ||
	 !createHorizon3D(hor3dnm.str()) ||
	 !testHorizon3D(hor3dnm.str()) ||
	 !removeHorizon3D(hor3dnm.str()) )
	return false;

    return true;
}
