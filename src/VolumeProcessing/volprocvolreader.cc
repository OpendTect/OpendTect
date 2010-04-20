/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID = "$Id: volprocvolreader.cc,v 1.4 2010-04-20 22:03:25 cvskris Exp $";

#include "volprocvolreader.h"

#include "arraynd.h"
#include "ioman.h"
#include "ioobj.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seistrc.h"

namespace VolProc
{

void VolumeReader::initClass()
{
    VolProc::PS().addCreator( create, VolumeReader::sKeyType(),
	    		      VolumeReader::sUserName() );
}
    
    
VolumeReader::VolumeReader(Chain& pc)
    : Step( pc )
{}


VolumeReader::~VolumeReader()
{
    deepErase( readers_ );
}    


bool VolumeReader::setVolumeID( const MultiID& mid )
{
    deepErase( readers_ );
    translators_.erase();

    mid_ = mid;

    return addReader();
}    


bool VolumeReader::prepareComp( int nrthreads )
{
    if ( readers_.size()!=1 || translators_.size()!=1 )
    {
	deepErase( readers_ );
	translators_.erase();
	return false;
    }

    for ( int idx=1; idx<nrthreads; idx++ )
	addReader();

    return true;
}


bool VolumeReader::addReader()
{
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !ioobj )
    {
	deepErase( readers_ );
	translators_.erase();
	return false;
    }

    SeisTrcReader* reader = new SeisTrcReader( ioobj );
    if ( !reader || !reader->prepareWork() )
    {
	delete reader;

	deepErase( readers_ );
	translators_.erase();

	return false;
    }

    mDynamicCastGet( SeisTrcTranslator*, translator,
		     reader->translator() );

    if ( !translator || !translator->supportsGoTo() )
    {
	delete reader;
	deepErase( readers_ );
	translators_.erase();
	return false;
    }

    readers_ += reader;
    translators_ += translator;

    return true;
}


Step*  VolumeReader::create( Chain& pc )
{ return new VolumeReader( pc ); }


bool VolumeReader::computeBinID( const BinID& bid, int thread )
{
    if ( readers_.size()<=thread || translators_.size()<=thread || !isOK() ||
	 !output_ )
	return false;

    const StepInterval<int> outputinlrg( output_->inlsampling_.start,
   			 output_->inlsampling_.atIndex( output_->getInlSz()-1 ),
			 output_->inlsampling_.step );

    if ( !outputinlrg.includes( bid.inl ) ||
         (bid.inl-outputinlrg.start)%outputinlrg.step )
	return false;

    const StepInterval<int> outputcrlrg( output_->crlsampling_.start,
			output_->crlsampling_.atIndex( output_->getCrlSz()-1 ),
			output_->crlsampling_.step );

    if ( !outputcrlrg.includes( bid.crl ) ||
         (bid.crl-outputcrlrg.start)%outputcrlrg.step )
	return false;

    bool hastrc = translators_[thread]->goTo(bid);

    SeisTrc trc;
    if ( hastrc )
	hastrc = readers_[thread]->get( trc );

    const Array3D<float>* inputarr = input_ && input_->nrCubes()
       	? &input_->getCube( 0 ) : 0;

    StepInterval<int> inputinlrg;
    if ( inputarr )
    {
	inputinlrg = input_->inlsampling_.interval( input_->getInlSz() );
	if ( !inputinlrg.includes( bid.inl ) ||
	      (bid.inl-inputinlrg.start)%inputinlrg.step )
	    inputarr = 0;
    }

    StepInterval<int> inputcrlrg;
    if ( inputarr )
    {
	inputcrlrg = input_->crlsampling_.interval( input_->getCrlSz() );
	if ( !inputcrlrg.includes( bid.crl ) ||
	      (bid.crl-inputcrlrg.start)%inputcrlrg.step )
	    inputarr = 0;
    }

    const int inputinlidx = inputinlrg.nearestIndex( bid.inl );
    const int inputcrlidx = inputcrlrg.nearestIndex( bid.crl );
    const int outputinlidx = outputinlrg.nearestIndex( bid.inl );
    const int outputcrlidx = outputcrlrg.nearestIndex( bid.crl );

    Array3D<float>& outputarray = output_->getCube(0);
    for ( int idx=outputarray.info().getSize(2)-1; idx>=0; idx-- )
    {
	bool dobg;
	float val;
	if ( hastrc )
	{
	    const int cursample = output_->z0_+idx;
	    const double z = output_->zstep_ * cursample;
	    val = trc.getValue( z, 0 );
	    dobg = mIsUdf(val);
	}
	else
	    dobg = true;

	if ( dobg )
	{
	    val = inputarr
		? inputarr->get(inputinlidx,inputcrlidx,idx)
		: mUdf( float );
	}   

        outputarray.set( outputinlidx, outputcrlidx, idx, val );
    } 
     
    return true;
}


void VolumeReader::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );
    pars.set( sKeyVolumeID(), mid_ );
}


bool VolumeReader::usePar( const IOPar& pars )
{
    deepErase( readers_ );
    translators_.erase();

    if ( !Step::usePar( pars ) )
	return false;

    MultiID mid;
    pars.get( sKeyVolumeID(), mid );
    return setVolumeID( mid );
}


bool VolumeReader::isOK() const
{
    return translators_.size();
}


}; //namespace
