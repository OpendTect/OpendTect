/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "batchprog.h"
#include "velocityvolumeconversion.h"

#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "multiid.h"
#include "moddepmgr.h"
#include "progressmeter.h"

#include "prog.h"

#define mErrRet( msg ) \
{ \
    strm << msg; \
    return false; \
}

bool BatchProgram::go( std::ostream& strm )
{
    OD::ModDeps().ensureLoaded("Velocity");
    
    HorSampling hrg;
    if ( !hrg.usePar( pars() ) )
	hrg.init( true );

    MultiID inputmid;
    if ( !pars().get( Vel::VolumeConverter::sKeyInput(), inputmid) )
	mErrRet( "Cannot read input volume id" )

    PtrMan<IOObj> inputioobj = IOM().get( inputmid );
    if ( !inputioobj )
	mErrRet( "Cannot read input volume object" )

    MultiID outputmid;
    if ( !pars().get( Vel::VolumeConverter::sKeyOutput(), outputmid ) )
	mErrRet( "Cannot read output volume id" )

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
	mErrRet( "Cannot read output volume object" )

    VelocityDesc veldesc;
    if ( !veldesc.usePar( pars() ) )
	mErrRet( "Cannot read output velocity definition" )

    Vel::VolumeConverter conv( *inputioobj, *outputioobj, hrg, veldesc );
    TextStreamProgressMeter progressmeter( strm );
    conv.setProgressMeter( &progressmeter );

    if ( !conv.execute() )
	mErrRet( conv.errMsg() )

    if ( veldesc.type_ != VelocityDesc::Unknown )
	veldesc.fillPar( outputioobj->pars() );
    else
	veldesc.removePars( outputioobj->pars() );

    if ( !IOM().commitChanges(*outputioobj) ) 
	mErrRet( "Cannot write velocity information" )

    return true;
}
