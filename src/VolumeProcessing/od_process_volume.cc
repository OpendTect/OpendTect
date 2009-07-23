/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_volume.cc,v 1.17 2009-07-23 02:07:04 cvskris Exp $";

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

    CubeSampling cs( true );
    if ( !cs.usePar( pars() ) )
	strm << "Could not read ranges - Will process full survey\n";
    
    PtrMan<VolProc::ChainExecutor> pce = new VolProc::ChainExecutor( *chain );
    RefMan<Attrib::DataCubes> cube = new Attrib::DataCubes;

    char itemsize = cube && cube->nrCubes() && cube->getCube(0).getStorage()
	? cube->getCube(0).getStorage()->bytesPerItem()
	: sizeof(float);

    od_uint64 nrbytes = cs.totalNr() * itemsize * 2;
    strm << "Allocating " << getBytesString( nrbytes ) << "in memory\n";
    
    cube->setSizeAndPos( cs );
    if ( !pce->setCalculationScope( cube ) )
    {
	strm << "Could not set calculation scope!";
	return false;
    } 

    if ( !pce->execute( &strm ) )
    {
	strm << "Unexecutable Chain!";
	return false;
    }	

    pce = 0; //delete all internal volumes.
   
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
    const VelocityDesc* veldesc = chain->getVelDesc();
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
