/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/


#include "batchprog.h"
#include "velocityvolumeconversion.h"

#include "iopar.h"
#include "ioobj.h"
#include "dbkey.h"
#include "moddepmgr.h"
#include "progressmeterimpl.h"
#include "seisioobjinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

#include "prog.h"

#define mErrRet( msg ) { strm << msg; return false; }

mLoad1Module("Velocity")

bool BatchProgram::doWork( od_ostream& strm )
{
    DBKey inputmid;
    if ( !pars().get( Vel::VolumeConverter::sKeyInput(), inputmid) )
	mErrRet( "Cannot read input volume id" )

    PtrMan<IOObj> inputioobj = inputmid.getIOObj();
    if ( !inputioobj )
	mErrRet( "Cannot read input volume object" )

    TrcKeySampling hrg;
    if ( !hrg.usePar(pars()) )
    {
	SeisIOObjInfo seisinfo( inputioobj );
	TrcKeyZSampling cs;
	seisinfo.getRanges( cs );
	hrg = cs.hsamp_;
    }

    DBKey outputmid;
    if ( !pars().get( Vel::VolumeConverter::sKeyOutput(), outputmid ) )
	mErrRet( "Cannot read output volume id" )

    PtrMan<IOObj> outputioobj = outputmid.getIOObj();
    if ( !outputioobj )
	mErrRet( "Cannot read output volume object" )

    VelocityDesc veldesc;
    if ( !veldesc.usePar( pars() ) )
	mErrRet( "Cannot read output velocity definition" )

    Vel::VolumeConverter conv( *inputioobj, *outputioobj, hrg, veldesc );
    TextStreamProgressMeter progressmeter( strm );
    ( (Task&)conv ).setProgressMeter( &progressmeter );

    if ( !conv.execute() )
    {
	if ( !conv.errMsg().isEmpty() )
	    strm << mFromUiStringTodo( conv.errMsg() ) << od_endl;

	return false;
    }

    if ( veldesc.type_ != VelocityDesc::Unknown )
	veldesc.fillPar( outputioobj->pars() );
    else
	veldesc.removePars( outputioobj->pars() );

    auto uirv = outputioobj->commitChanges();
    if ( !uirv.isOK() )
	mErrRet( ::toString(uirv) )

    return true;
}
