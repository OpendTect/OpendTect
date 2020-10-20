/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/


#include "batchprog.h"

#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "seisrangeseldata.h"
#include "seiszaxisstretcher.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "moddepmgr.h"

#include "prog.h"


#define mErrRet(s) { strm << s << od_endl; return false; }

mLoad2Modules("Seis","Well")

bool BatchProgram::doWork( od_ostream& strm )
{
    DBKey inpdbky;
    if ( !pars().get(IOPar::compKey(sKey::Input(),sKey::ID()),inpdbky) )
	mErrRet( "Cannot find input ID" )
    PtrMan<IOObj> inputioobj = getIOObj( inpdbky );
    if ( !inputioobj )
	mErrRet( "Cannot find input data" )
    DBKey outputdbky;
    if ( !pars().get(IOPar::compKey(sKey::Output(),sKey::ID()),outputdbky) )
	mErrRet( "Cannot find output ID" )
    PtrMan<IOObj> outputioobj = getIOObj( outputdbky );
    if ( !outputioobj )
	mErrRet( "Cannot find output data" )
    PtrMan<IOPar> ztranspar = pars().subselect( "ZTrans" );
    if ( !ztranspar || ztranspar->isEmpty() )
	mErrRet( "Cannot find Z tranformation parameters" )
    RefMan<ZAxisTransform> ztransform = ZAxisTransform::create( *ztranspar );
    if ( !ztransform )
	mErrRet( "Cannot construct Z transform" )
    if ( !ztransform->isOK() )
	mErrRet( "Velocity model is not usable" )

    bool istime2depth = SI().zIsTime();
    pars().getYN( "Time to depth", istime2depth );

    VelocityDesc veldesc;
    const bool isvel = veldesc.usePar( inputioobj->pars() ) &&
			veldesc.isVelocity();
    const bool isvrms = veldesc.type_ == VelocityDesc::RMS;
    if ( isvel )
    {
	//would we convert Thomsen? nothing prepared for this now
	strm << "\nDetected that the stretching will be done on velocities.\n"
		"Will stretch in z-domain and convert back to velocities.\n";
	if ( isvrms )
	{
	    strm << "\nDetected that the input cube contains RMS velocities.\n"
		"RMS velocities are not present in Depth domain;\n"
		"a conversion to interval velocities will thus be processed.\n";
	}
    }

    Seis::RangeSelData rsd( pars() );
    SeisZAxisStretcher stretcher( *inputioobj, *outputioobj, *ztransform,
				  true, isvel, &rsd );
    if ( isvel )
    {
	stretcher.setVelTypeIsVint( veldesc.type_ == VelocityDesc::Interval );
	if ( isvrms )
	    stretcher.setVelTypeIsVrms( isvrms );
    }

    LoggedTaskRunner taskrunner( strm );
    return taskrunner.execute( stretcher );
}
