/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUnusedVar = "$Id: prestackmute.cc,v 1.23 2012-07-02 14:11:38 cvsbruno Exp $";

#include "prestackmute.h"

#include "arrayndslice.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "muter.h"
#include "prestackgather.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "separstr.h"

using namespace PreStack;

Mute::Mute()
    : Processor( sFactoryKeyword() )
    , def_(*new MuteDef)
    , muter_(0)
    , tail_(false)
    , taperlen_(10)
{}


Mute::~Mute()
{
    delete muter_;
    delete &def_;
}


#define mErrRet(s) \
{ delete muter_; muter_ = 0; errmsg_ = s; return false; }

bool Mute::prepareWork()
{
    if ( !Processor::prepareWork() )
	return false;

    outidx_.erase();
    offsets_.erase();
    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	const Gather* input = inputs_[idx];
	if ( !input || !outputs_[idx] )
	    continue;

	const int nroffsets = input->size( Gather::offsetDim()==0 );
	for ( int ioff=nroffsets-1; ioff>=0; ioff-- )
	{
	    outidx_ += idx;
	    offsets_ += ioff;
	}
    }

    if ( muter_ ) return true;

    muter_ = new Muter( taperlen_, tail_ );

    return true;
}


void Mute::setEmptyMute()
{
    id_ = MultiID();
    while ( def_.size() )
	def_.remove( 0 );
}


void Mute::setTailMute( bool yn )
{ tail_ = yn; delete muter_; muter_ = 0; }


void Mute::setTaperLength( float l )
{ taperlen_ = l; delete muter_; muter_ = 0; }


bool Mute::setMuteDefID( const MultiID& mid )
{
    if ( id_==mid )
	return true;

    if ( mid.isEmpty() )
	mErrRet("No MuteDef ID provided.")
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) 
	mErrRet("Cannot find MuteDef ID in Object Manager.")

    if ( !MuteDefTranslator::retrieve(def_,ioobj,errmsg_) )
    {
	BufferString msg = "Mute definition \"";
	msg += ioobj->name();
	msg += "\" cannot be read.";
	msg += FileMultiString::separatorStr();
	msg += errmsg_;
	mErrRet( msg.buf() );
    }
    
    id_ = mid;
    
    return true;
}


void Mute::fillPar( IOPar& par ) const
{
    PtrMan <IOObj> ioobj = IOM().get( id_ );
    if ( ioobj )
	par.set( sMuteDef(), id_ );

    par.set( sTaperLength(), taperlen_ );
    par.setYN( sTailMute(), tail_ );
}


bool Mute::usePar( const IOPar& par )
{
    float taperlen;
    if ( par.get( sTaperLength(), taperlen ) )
	setTaperLength( taperlen );

    bool tail;
    if ( par.getYN( sTailMute(), tail ) )
	setTailMute( tail );

    MultiID mid;
    if ( par.get(sMuteDef(),mid) && !setMuteDefID(mid) )
    {
	errmsg_ = "No Mute definition ID found.";
	return false;
    }

    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return false; }

bool Mute::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !muter_ )
	return false;

    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	const int outidx = outidx_[idx];
	const int ioffs = offsets_[idx];

	Gather* output = outputs_[outidx];
	const Gather* input = inputs_[outidx];

	const int nrsamples = input->size(Gather::zDim()==0);
	const float offs = input->getOffset(ioffs);
	TypeSet< Interval<float> > muteitvs;
	def_.computeIntervals( offs, input->getBinID(), muteitvs );
	if ( muteitvs.isEmpty() )
	    continue;

	TypeSet< Interval<float> > muteitvspos;
	Muter::muteIntervalsPos( muteitvs, muteitvspos, 
				 input->posData().range(false) );

	Array1DSlice<float> slice( output->data() );
	slice.setPos( 0, ioffs );
	if ( !slice.init() )
	    continue;

	muter_->muteIntervals( slice, nrsamples, muteitvspos );
    }

    return true;
}
