/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_velocityconv.cc,v 1.1 2009-12-04 19:02:37 cvskris Exp $";

#include "batchprog.h"
#include "velocityvolumeconversion.h"

#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "multiid.h"
#include "progressmeter.h"
//#include "seiszaxisstretcher.h"
//#include "timedepthconv.h"

#include "prog.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    HorSampling hrg;
    if ( !hrg.usePar( pars() ) )
    {
	hrg.init( true );
    }

    MultiID inputmid;
    if ( !pars().get( Vel::VolumeConverter::sKeyInput(), inputmid) )
    {
	strm << "Cannot read input volume id"; 
	return false;
    }

    PtrMan<IOObj> inputioobj = IOM().get( inputmid );
    if ( !inputioobj )
    {
	strm << "Cannot read input volume object"; 
	return false;
    }

    MultiID outputmid;
    if ( !pars().get( Vel::VolumeConverter::sKeyOutput(), outputmid ) )
    {
	strm << "Cannot read output volume id"; 
	return false;
    }

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
    {
	strm << "Cannot read output volume object"; 
	return false;
    }

    VelocityDesc veldesc;
    if ( !veldesc.usePar( pars() ) )
    {
	strm << "Cannot read output velocity definition";
	return false;
    }

    Vel::VolumeConverter conv( inputioobj, outputioobj, hrg, veldesc );
    inputioobj.set( 0, false );
    outputioobj.set( 0, false );
    TextStreamProgressMeter progressmeter( strm );
    conv.setProgressMeter( &progressmeter );

    if ( !conv.execute() )
    {
	strm << conv.errMsg();
	return false;
    }

    return true;
}
