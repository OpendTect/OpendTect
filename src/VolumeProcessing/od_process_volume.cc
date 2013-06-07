/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id$";

#include "batchprog.h"

#include "attribdatacubeswriter.h"
#include "ioman.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "veldesc.h"

#include "arrayndimpl.h"
#include "survinfo.h"
#include "moddepmgr.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    OD::ModDeps().ensureLoaded( "VolumeProcessing" );
    
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

    CubeSampling cs( true );
    if ( !cs.usePar( pars() ) )
	strm << "Could not read ranges - Will process full survey\n";

    if ( chain->getStep( 0 ) && chain->getStep( 0 )->needsInput() )
    {
	strm << "First step in chain (";
	strm << chain->getStep( 0 )->userName();
	strm << ") requires an input and can thus not be first.";
	return false;
    }

    PtrMan<VolProc::ChainExecutor> pce = new VolProc::ChainExecutor( *chain );


    const float zstep = chain->getZStep();
    HorSampling inputhrg = cs.hrg;
    StepInterval<int> outputzrg( mNINT32(cs.zrg.start/zstep),
				 mNINT32(cs.zrg.stop/zstep),
				 mNINT32(cs.zrg.step/zstep) );
    if ( outputzrg.step<1 ) outputzrg.step = 1;
    StepInterval<int> inputzrg = outputzrg;

    od_uint64 nrbytes = 0;
    const char itemsize = sizeof(float);

    const HorSampling survhrg = SI().sampling(false).hrg;
    const Interval<int> survzrg( mNINT32(SI().zRange(false).start/zstep),
				 mNINT32(SI().zRange(false).stop/zstep) );

    for ( int idx=chain->nrSteps()-1; idx>=0; idx-- )
    {
	const od_uint64 outputsize = ((od_uint64) inputhrg.totalNr()) *
	    (inputzrg.nrSteps()+1) * itemsize;

	od_uint64 inputsize = 0;

	const bool needsinput =  idx && chain->getStep(idx)->needsInput();
	if ( needsinput )
	{
	    inputzrg = chain->getStep(idx)->getInputZRg( inputzrg );
	    inputhrg = chain->getStep(idx)->getInputHRg( inputhrg );
	    inputzrg.limitTo( survzrg );
	    inputhrg.limitTo( survhrg );

	    if ( !chain->getStep(idx)->canInputAndOutputBeSame() )
		inputsize = ((od_uint64)inputhrg.totalNr())
		    * (inputzrg.nrSteps()+1) * itemsize;
	}

	const od_uint64 totalsize = inputsize+outputsize;
	if ( totalsize>nrbytes )
	    nrbytes = totalsize;

	if ( !needsinput )
	    break;
    }

    strm << "Allocating " << getBytesString( nrbytes ) << " memory\n";
    
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
    writer.setSelection( cs.hrg, outputzrg );
    if ( !writer.execute( &strm ) )
    {
	return false;
    }

    return true;
}
