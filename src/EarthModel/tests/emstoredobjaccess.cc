/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "testprog.h"

#include "embody.h"
#include "emhorizon3d.h"
#include "emfaultstickset.h"
#include "emstoredobjaccess.h"
#include "executor.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "arrayndimpl.h"
#include "ioman.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "survinfo.h"



#define mErrRet(s) { od_ostream::logStream() << s << od_endl; return false; }

static bool initLoader( EM::StoredObjAccess& soa )
{
    if ( !soa.add( MultiID(100020,2) ) ) // Hor3D: 1-Top
	mErrRet( soa.getError(0) );
    if ( !soa.add( MultiID(100020,4) ) ) // Body: Slumps-2b
	mErrRet( soa.getError(0) );
    if ( !soa.add( MultiID(100020,5) ) ) // SSIS-Grid-Faultsticks
	mErrRet( soa.getError(0) );

    if ( soa.add( MultiID(100020,99795) ) )
	mErrRet( "ID 100020.99795 should give error" );

    soa.dismiss( MultiID(100020,99795) );

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
    for (int i=0; i<arr->info().getSize(0); i++ )
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
    for (int i=0; i<arr->info().getSize(0); i++ )
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
    for (int i=0; i<arr->info().getSize(0); i++ )
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

    EM::StoredObjAccess& soa = *new EM::StoredObjAccess;
    if ( !initLoader(soa) )
	return false;

    Executor* exec = soa.reader();
    TextTaskRunner ttr( od_cout() );
    if ( !ttr.execute(*exec) )
	return false;

    delete exec;

    mDynamicCastGet(EM::Horizon3D*,hor3d,soa.object(0))
    mRunStandardTest(hor3d,"object(0) must be Hor3D")
    mDynamicCastGet(EM::Body*,body,soa.object(1))
    mRunStandardTest(body,"object(1) must be Body")
    mDynamicCastGet(EM::FaultStickSet*,fss,soa.object(2))
    mRunStandardTest(fss,"object(2) must be FaultStickSet")

    const Coord crd_ix_400_900( 620620, 6081472 );
    const float zval = hor3d->getZValue( crd_ix_400_900 );
    mRunStandardTest(zval>0.549 && zval<0.6,"Horizon Z Value check")
       /*
       od_cout() << "Z Val at 400/900: " << zval << od_endl;
       -> Z Val at 400/900: 0.54925388
       */

    TrcKeyZSampling cs;
    mRunStandardTest( body->getBodyRange(cs),"Get body ranges")
    mRunStandardTest( cs.hsamp_.start_.inl()>390 && cs.hsamp_.stop_.crl()<870,
			"Check on body ranges" )
       /*
       od_cout() << "Inl/Crl rg: "
	<< cs.hsamp_.start_.inl() << '-' << cs.hsamp_.stop_.inl() << ", "
	<< cs.hsamp_.start_.crl() << '-' << cs.hsamp_.stop_.crl() << od_endl;
	-> Inl/Crl rg: 392-474, 810-864
       */

    const EM::FaultStickSetGeometry& fssgeom = fss->geometry();
    mRunStandardTest( fssgeom.nrSticks() == 4 && fssgeom.nrKnots(0) == 7,
			"Check nr Sticks and knots of Stick in FaultStickSet" );
       /*
	od_cout() << fssgeom.nrSticks(0) << " sticks, "
		<< fssgeom.nrKnots(0,0) << " knots." << od_endl;
       -> 4 sticks, 7 knots.
       */
    delete &soa;

    const BufferString hor3dnm( "test_horizon3d" );
    if (   !createHorizon3D(hor3dnm.str())
	|| !testHorizon3D(hor3dnm.str())
	|| !removeHorizon3D(hor3dnm.str()) )
	return false;

    return true;
}
