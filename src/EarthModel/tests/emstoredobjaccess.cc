/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2016
-*/

#include "batchprog.h"
#include "testprog.h"

#include "embody.h"
#include "emfaultstickset.h"
#include "emhorizon3d.h"
#include "emstoredobjaccess.h"
#include "executor.h"
#include "moddepmgr.h"
#include "dbkey.h"



#define mErrRet(s) { od_ostream::logStream() << s << od_endl; return false; }

static bool initLoader( EM::StoredObjAccess& soa )
{
    if ( !soa.add( DBKey("100020.2") ) ) // Hor3D: 1-Top
	mErrRet( soa.getError(0) );
    if ( !soa.add( DBKey("100020.4") ) ) // Body: Slumps-2b
	mErrRet( soa.getError(0) );
    if ( !soa.add( DBKey("100020.5") ) ) // SSIS-Grid-Faultstick
	mErrRet( soa.getError(0) );

    if ( soa.add( DBKey("100020.99795") ) )
	mErrRet( "ID 100020.99795 should give error" );
    soa.dismiss( DBKey("100020.99795") );

    return true;
}


mLoad1Module("EarthModel")
{
    mInitBatchTestProg();

    EM::StoredObjAccess soa;
    if ( !initLoader(soa) )
	return false;

    Executor* exec = soa.reader();
    LoggedTaskRunner ltr( od_cout() );
    if ( !ltr.execute(*exec) )
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

    return true;
}
