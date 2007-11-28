/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_volume.cc,v 1.5 2007-11-28 13:57:52 cvskris Exp $";

#include "batchprog.h"

#include "horinterfiller.h"
#include "windowfunction.h"
#include "ioman.h"
#include "volumeprocessing.h"
#include "volumewriter.h"
#include "volumeprocessingtrans.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    VolProc::initBuiltinClasses();
    WindowFunction::initBuiltinClasses();

    
    MultiID chainid;
    pars().get( VolProcessingTranslatorGroup::sKeyChainID(), chainid );
    PtrMan<IOObj> ioobj = IOM().get( chainid );
    
    RefMan<VolProc::ProcessingChain> chain = new VolProc::ProcessingChain;
    BufferString errmsg;
    if ( !VolProcessingTranslator::retrieve( *chain, ioobj, errmsg ) )
    {
   	 chain = 0;
	 strm << "Could not open storage: " << chainid <<
	     	 ". Error description: ";
	 strm << errmsg.buf();
	 return false;
    }

    if ( chain->nrSteps()<1 )
    {
	strm << "Chain is empty - nothing to do.";
	return true;
    }

    CubeSampling cs;
    if ( !cs.usePar( pars() ) )
    {
	strm << "Could not read ranges.";
	return false;
    }
    
    VolProc::ProcessingChainExecutor pce( *chain );
    RefMan<Attrib::DataCubes> cube = new Attrib::DataCubes;

    cube->setSizeAndPos( cs );
    if ( !pce.setCalculationScope( cube ) )
    {
	strm << "Could not set calculation scope!";
	return false;
    } 

    if ( !pce.execute( &strm ) )
    {
	strm << "Unexecutable Chain!";
	return false;
    }	
   
    MultiID outputid;
    pars().get( VolProcessingTranslatorGroup::sKeyOutputID(), outputid );
    PtrMan<IOObj> outputobj = IOM().get( outputid );
    if ( !outputobj )
    {
	strm << "Could not find output ID!";
	return false;
    }	

    VolProc::VolumeWriter writer( outputid, *cube );
    if ( !writer.execute( &strm ) )
    {
	return false;
    }

    return true;
}
