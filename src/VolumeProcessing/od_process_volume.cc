    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "arrayndimpl.h"
#include "ioman.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "seisdatapack.h"
#include "seisdatapackwriter.h"
#include "survinfo.h"
#include "veldesc.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "uistrings.h"


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( "VolumeProcessing" );
    OD::ModDeps().ensureLoaded( "Well" );

    MultiID chainid;
    pars().get( VolProcessingTranslatorGroup::sKeyChainID(), chainid );
    PtrMan<IOObj> ioobj = IOM().get( chainid );
    if ( !ioobj )
    {
	strm << "Could not find volume processing, ID: '" << chainid << "'\n";
	return false;
    }

    RefMan<VolProc::Chain> chain = new VolProc::Chain;
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain,ioobj,errmsg) )
    {
	 chain = 0;
	 strm << "Could not open volume processing: \"" << ioobj->name() <<
	     "\". Error description: " << errmsg.getFullString();

	 return false;
    }

    if ( chain->nrSteps()<1 )
    {
	strm << "Chain is empty - nothing to do.";
	return true;
    }

    PtrMan<IOPar> subselpar = pars().subselect(
	    IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    TrcKeyZSampling cs( true );
    if ( !subselpar || !cs.usePar(*subselpar) )
	strm << "Could not read ranges - Will process full survey\n";

    if ( chain->getStep(0) && chain->getStep(0)->needsInput() )
    {
	strm << "First step in chain (";
	strm << chain->getStep(0)->userName();
	strm << ") requires an input and can thus not be first.";
	return false;
    }

    PtrMan<VolProc::ChainExecutor> pce = new VolProc::ChainExecutor( *chain );

    const float zstep = chain->getZStep();
    TrcKeySampling inputhrg = cs.hsamp_;

    const StepInterval<int> outputzrg( mNINT32(cs.zsamp_.start/zstep),
				 mNINT32(Math::Ceil(cs.zsamp_.stop/zstep)),
			   mMAX(mNINT32(Math::Ceil(cs.zsamp_.step/zstep)),1) );
			   //real -> index, outputzrg is the index of z-samples
    StepInterval<int> inputzrg = outputzrg;

    od_uint64 nrbytes = 0;
    const char itemsize = sizeof(float);

    const TrcKeySampling survhrg = SI().sampling(false).hsamp_;
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

    if ( !pce->setCalculationScope(cs.hsamp_,outputzrg) )
    {
	strm << "Could not set calculation scope!";
	return false;
    }

    if ( !pce->go(strm) )
    {
	strm << "Unexecutable Chain!";
	return false;
    }

    ConstDataPackRef<RegularSeisDataPack> cube = pce->getOutput();
    if ( cube ) DPM( DataPackMgr::SeisID() ).obtain( cube->id() );
    ConstPtrMan<VelocityDesc> veldesc = chain->getVelDesc()
	? new VelocityDesc( *chain->getVelDesc() )
	: 0;

    //delete all internal volumes.
    pce = 0;
    chain = 0;

    MultiID outputid;
    pars().get( "Output.0.Seismic.ID", outputid );
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
	    strm << uiStrings::phrCannotWriteDBEntry(outputobj->uiName());
	}
    }

    if ( !cube )
    {
	strm << "Could not retrieve output";
	return false;
    }

    TypeSet<int> indices;
    for ( int idx=0; idx<cube->nrComponents(); idx++ )
	indices += idx;

    SeisDataPackWriter writer( outputid, *cube, indices );
    writer.setSelection( cs.hsamp_, outputzrg );
    if ( !writer.go(strm) )
	return false;

    return true;
}
