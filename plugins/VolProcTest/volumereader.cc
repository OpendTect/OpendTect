/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "volumereader.h"

#include "arraynd.h"
#include "attribdatacubes.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "seisread.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "volumeprocessing.h"
#include "volprocthresholder.h"

using namespace VolProc;


void VolumeReader::initClass()
{
    VolProc::PS().addCreator( create, VolumeReader::sKeyType() );
}


VolumeReader::VolumeReader( ProcessingChain& pc )
	: ProcessingStep( pc )
	, storageid_( 0 )
	, reader_( 0 )
	, curtrc_( 0 )
{}


void VolumeReader::setStorage( const MultiID& mid )
{ storageid_ = mid; }


const MultiID& VolumeReader::getStorage() const
{ return storageid_; }


const char* VolumeReader::type() const
{ return sKeyType(); }


bool VolumeReader::needsInput( const HorSampling& ) const
{ return false; }


bool VolumeReader::setCurrentCalcPos( const BinID& newbinid )
{
    if ( !ProcessingStep::setCurrentCalcPos( newbinid ) )
    {
	errmsg_="No current calculation position";
	return false;
    }

    if ( !output_ )
    {
	pErrMsg( "No output" );
	return false;
    } 

    if ( !output_->nrCubes() )
    {
	if ( !output_->addCube( mUdf(float), false ) )
        {
	    errmsg_="Out of memory";
	    return false;
	}
    }
    
    if ( !reader_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( storageid_ );
	if ( !ioobj )
	{
	    errmsg_ = "Could not find entry for ID: ";
	    errmsg_ += storageid_;
	    return false;
	}

	reader_ = new SeisTrcReader( ioobj );
	if ( !reader_->prepareWork() )
	{
	    errmsg_ = "Could not open storage";
	    return false;
	}
    }

    mDynamicCastGet( SeisTrcTranslator*, trans, reader_->translator() );
    if ( !trans ) 
    {
	pErrMsg( "Could not get Translator" );
	return false;
    }

    if ( !trans->supportsGoTo() )
    {
	errmsg_ = "Selected input cannot be used for volume processing";
	return false;
    }

    validtrc_ = trans->goTo( curbid_ ) && reader_->get( curtrc_ );

    return true;
}


bool VolumeReader::compute( int start, int stop )
{
    if ( !validtrc_ || curtrc_.info().binid!=curbid_ )
	return true;

    const StepInterval<int> outputinlrg( output_->inlsampling.start,
	output_->inlsampling.atIndex( output_->getInlSz()-1 ),
	output_->inlsampling.step );
     
    const StepInterval<int> outputcrlrg( output_->crlsampling.start,
	output_->crlsampling.atIndex( output_->getCrlSz()-1 ),
	output_->crlsampling.step );	 

    const int outputinlidx = outputinlrg.nearestIndex( curbid_.inl );
    const int outputcrlidx = outputcrlrg.nearestIndex( curbid_.crl );
    

    Array3D<float>& outputarray = output_->getCube( 0 );

    for ( int idx=start; idx<=stop; idx++ )
    {
	const float depth = chain_.getZSampling().atIndex( idx );
	const float value = curtrc_.getValue( depth, 0 );
	outputarray.set( outputinlidx, outputcrlidx, idx, value );
    }

    return true;
}


void VolumeReader::fillPar( IOPar& par ) const
{
    ProcessingStep::fillPar( par );

    par.set( sKeyStorageID(), storageid_ );
}


bool VolumeReader::usePar( const IOPar& par )
{
    if ( !ProcessingStep::usePar( par ) )
	return false;

    if ( !par.get( sKeyStorageID(), storageid_ ) )
	return false;

    PtrMan<IOObj> ioobj = IOM().get( storageid_ );
    return ioobj;
}


const char* VolumeReader::errMsg() const
{ return errmsg_.str(); }


ProcessingStep* VolumeReader::create( ProcessingChain& pc )
{ return new VolumeReader( pc ); }
