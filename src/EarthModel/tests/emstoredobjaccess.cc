/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2016
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "emstoredobjaccess.h"
#include "od_ostream.h"
#include "multiid.h"

#define mErrRet(s) { od_cout() << s << od_endl; return false; }

static bool initLoader( EM::StoredObjAccess& soa )
{
/*
    if ( !soa.add( MultiID("100020.3") ) ) // Hor3D: Demo 6 --> FS8
	mErrRet( soa.getError(0) );
    if ( !soa.add( MultiID("100020.53") ) ) // Body: Slumps-2b
	mErrRet( soa.getError(0) );
    if ( !soa.add( MultiID("100020.33") ) ) // SSIS-Grid-Faultsticks
	mErrRet( soa.getError(0) );

    if ( soa.add( MultiID("100020.99795") ) )
	mErrRet( "ID 100020.99795 should give error" );
    soa.dismiss( "100020.99795" );

*/
    return true;
}


#undef mErrRet
#define mErrRet(s) { od_cout() << s << od_endl; ExitProgram(1); }

#include "testprog.h"
#include "emhorizon3d.h"
#include "embody.h"
#include "emfaultstickset.h"
#include "executor.h"
#include "moddepmgr.h"


int main( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded( "EarthModel" );


    EM::StoredObjAccess soa;
    if ( !initLoader(soa) )
	ExitProgram(1);

/*
    Executor* exec = soa.reader();
    TextTaskRunner ttr( od_cout() );
    if ( !ttr.execute(*exec) )
	ExitProgram(1);

    delete exec;

    mDynamicCastGet(EM::Horizon3D*,hor3d,soa.object(0))
    if ( !hor3d )
	mErrRet("object(0) is null or not Hor3D")
    mDynamicCastGet(EM::Body*,body,soa.object(1))
    if ( !body )
	mErrRet("object(1) is null or not Body")
    mDynamicCastGet(EM::FaultStickSet*,fss,soa.object(2))
    if ( !fss )
	mErrRet("object(2) is null or not FaultStickSet")

    const Coord crd_ix_400_600( 613123, 6081263 );
    const float zval = hor3d->getZValue( crd_ix_400_600 );
*/
    /*
       od_cout() << "Z Val at 400/600: " << zval << od_endl;
       -> Z Val at 400/600: 0.65230799
    */
/*
    if ( zval < 0.65 || zval > 0.66 )
	mErrRet("Hor Z Value not as expected")

    TrcKeyZSampling cs;
    if ( !body->getBodyRange(cs) )
	mErrRet("Cannot get Body ranges")
    if ( cs.hsamp_.start_.inl() < 390 || cs.hsamp_.stop_.crl() > 870 )
	mErrRet("Body range not as expected")

*/

    /*
       od_cout() << "Inl/Crl rg: "
	<< cs.hsamp_.start_.inl() << '-' << cs.hsamp_.stop_.inl() << ", "
	<< cs.hsamp_.start_.crl() << '-' << cs.hsamp_.stop_.crl() << od_endl;
	-> Inl/Crl rg: 392-474, 810-864
    */

/*
    const EM::FaultStickSetGeometry& fssgeom = fss->geometry();
    if ( fssgeom.nrSticks(0) != 4 || fssgeom.nrKnots(0,0) != 7 )
	mErrRet("Nr Sticks or knots not as expected")
*/
    /*
	od_cout() << fssgeom.nrSticks(0) << " sticks, "
		<< fssgeom.nrKnots(0,0) << " knots." << od_endl;
       -> 4 sticks, 7 knots.
    */

    return ExitProgram( 0 );
}
