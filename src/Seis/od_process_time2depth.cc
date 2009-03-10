/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_time2depth.cc,v 1.1 2009-03-10 12:26:19 cvskris Exp $";

#include "batchprog.h"

#include "cubesampling.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "multiid.h"
#include "seiszaxisstretcher.h"
#include "timedepthconv.h"

#include "prog.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    CubeSampling cs;
    if ( !cs.usePar( pars() ) )
    {
	strm << "Cannot read output range";
	return false;
    }

    MultiID inputmid;
    if ( !pars().get("Input volume", inputmid) )
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
    if ( !pars().get("Output volume", outputmid ) )
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

    MultiID velmid;
    if ( !pars().get("Velocity model", velmid) )
    {
	strm << "Cannot read velocity volume id"; 
	return false;
    }

    RefMan<Time2DepthStretcher> ztransform = new Time2DepthStretcher;
    if ( !ztransform->setVelData( velmid ) || !ztransform->isOK() )
    {
	strm << "Velocity model is not usable";
	return false;
    }

    SeisZAxisStretcher exec( *inputioobj, *outputioobj, cs, *ztransform,
	   		     true );
    exec.setName( "Time to depth conversion");
    if ( !exec.isOK() )
    {
	strm << "Cannot initialize readers/writers";
	return false;
    }

    return exec.execute( &strm );
}
