/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "prestackmute.h"

#include "arrayndslice.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "iopar.h"
#include "muter.h"
#include "prestackgather.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "separstr.h"
#include "uistrings.h"

namespace PreStack
{

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
    id_ = DBKey::getInvalid();
    while ( def_.size() )
	def_.remove( 0 );
}


void Mute::setTailMute( bool yn )
{ tail_ = yn; delete muter_; muter_ = 0; }


void Mute::setTaperLength( float l )
{ taperlen_ = l; delete muter_; muter_ = 0; }


bool Mute::setMuteDefID( const DBKey& dbky )
{
    if ( id_==dbky )
	return true;

    if ( dbky.isInvalid() )
	mErrRet( uiStrings::phrDBIDNotValid() )
    PtrMan<IOObj> ioobj = dbky.getIOObj();
    if ( !ioobj )
	mErrRet( uiStrings::phrCannotFindObjInDB() );

    if ( !MuteDefTranslator::retrieve(def_,ioobj,errmsg_) )
    {
	uiString errstr( ioobj->phrCannotReadObj() );
	if ( !errmsg_.isEmpty() )
	    errstr.appendPhrase( errmsg_ );
	mErrRet( errstr );
    }

    id_ = dbky;

    return true;
}


void Mute::fillPar( IOPar& par ) const
{
    PtrMan <IOObj> ioobj = id_.getIOObj();
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

    DBKey dbky;
    if ( par.get(sMuteDef(),dbky) )
	setMuteDefID( dbky );

    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return false; }

bool Mute::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !muter_ )
	return false;

    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	const int outidx = outidx_[idx];
	const int ioffs = offsets_[idx];

	Gather* output = outputs_[outidx];
	const Gather* input = inputs_[outidx];

	const int nrsamples = input->size(Gather::zDim()==0);
	const float offs = input->getOffset(ioffs);

	const float mutez = def_.value( offs, input->getBinID() );
	if ( mIsUdf(mutez) )
	    continue;

	const float mutepos =
	    Muter::mutePos( mutez, input->posData().range(false) );

	Array1DSlice<float> slice( output->data() );
	slice.setPos( 0, ioffs );
	if ( !slice.init() )
	    continue;

	muter_->mute( slice, nrsamples, mutepos );
    }

    return true;
}

} // namespace PreStack
