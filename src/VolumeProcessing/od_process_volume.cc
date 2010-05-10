/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_volume.cc,v 1.21 2010-05-10 16:10:45 cvskris Exp $";

#include "batchprog.h"

#include "attribdatacubeswriter.h"
#include "ioman.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "veldesc.h"

#include "initalgo.h"
#include "initgeometry.h"
#include "initearthmodel.h"
#include "initvolumeprocessing.h"
#include "initvelocity.h"
#include "arrayndimpl.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    Algo::initStdClasses();
    VolumeProcessing::initStdClasses();
    Geometry::initStdClasses();
    EarthModel::initStdClasses();
    Velocity::initStdClasses();
    
    MultiID chainid;
    pars().get( VolProcessingTranslatorGroup::sKeyChainID(), chainid );
    PtrMan<IOObj> ioobj = IOM().get( chainid );
    if ( !ioobj )
    {
	strm << "Could not find volume processing ID: \"" << chainid << "\"\n";
	return false;
    }
    
    RefMan<VolProc::Chain> chain = new VolProc::Chain;
    BufferString errmsg;
    if ( !VolProcessingTranslator::retrieve( *chain, ioobj, errmsg ) )
    {
   	 chain = 0;
	 strm << "Could not open volume processing: \"" << ioobj->name() <<
	     "\". Error description: " << errmsg.buf();

	 return false;
    }

    if ( chain->nrSteps()<1 )
    {
	strm << "Chain is empty - nothing to do.";
	return true;
    }

    int nrcubes = 1;
    for ( int idx=0; idx<chain->nrSteps(); idx++ )
    {
	if ( !chain->getStep(idx)->canInputAndOutputBeSame() )
	{
	    nrcubes = 2;
	    break;
	}
    }

    CubeSampling cs( true );
    if ( !cs.usePar( pars() ) )
	strm << "Could not read ranges - Will process full survey\n";
    
    PtrMan<VolProc::ChainExecutor> pce = new VolProc::ChainExecutor( *chain );

    char itemsize = sizeof(float);


    od_uint64 nrbytes = cs.totalNr() * itemsize * nrcubes;
    strm << "Allocating " << getBytesString( nrbytes ) << " in memory\n";
    
    if ( !pce->setCalculationScope( cs ) )
    {
	strm << "Could not set calculation scope!";
	return false;
    } 

    if ( !pce->execute( &strm ) )
    {
	strm << "Unexecutable Chain!";
	return false;
    }	

    RefMan<const Attrib::DataCubes> cube = pce->getOutput();
    PtrMan<const VelocityDesc> veldesc = chain->getVelDesc() 
	? new VelocityDesc( *chain->getVelDesc() )
	: 0;

    //delete all internal volumes.
    pce = 0;
    chain = 0;
   
    MultiID outputid;
    pars().get( VolProcessingTranslatorGroup::sKeyOutputID(), outputid );
    PtrMan<IOObj> outputobj = IOM().get( outputid );
    if ( !outputobj )
    {
	strm << "Could not find output ID!";
	return false;
    }	

    bool docommit = false;

    VelocityDesc omfdesc;
    const bool hasveldesc = omfdesc.usePar( outputobj->pars() );
    if ( veldesc )
    {
	if ( !hasveldesc || omfdesc!=*veldesc )
	{
	    veldesc->fillPar( outputobj->pars() );
	    docommit = true;
	}
    }
    else if ( hasveldesc )
    {
	VelocityDesc::removePars( outputobj->pars() );
	docommit = true;
    }

    if ( docommit )
    {
	if ( !IOM().commitChanges( *outputobj ) )
	{
	    strm << "Warning: Could not write velocity information to database"
	            " for " << outputobj->name() << "\n\n";
	}
    }

    const TypeSet<int> indices( 1, 0 );
    Attrib::DataCubesWriter writer( outputid, *cube, indices );
    if ( !writer.execute( &strm ) )
    {
	return false;
    }

    return true;
}
