/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackmute.cc,v 1.4 2007-10-03 14:01:33 cvskris Exp $";

#include "prestackmute.h"

#include "flatposdata.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "muter.h"
#include "prestackgather.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"


using namespace PreStack;

void Mute::initClass()
{
    PF().addCreator( Mute::createFunc, Mute::sName() );
}


Processor* Mute::createFunc()
{
    return new Mute;
}


Mute::Mute()
    : def_(*new MuteDef)
    , muter_(0)
    , tail_(false)
    , taperlen_(10)
{
}


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

    if ( muter_ ) return true;

    muter_ = new Muter( taperlen_, tail_ );
    return true;
}


bool Mute::setMuteDefID( const MultiID& mid )
{
    if ( id_==mid )
	return true;

    if ( mid.isEmpty() )
	mErrRet("No MuteDef ID provided")
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) 
	mErrRet("Cannot find MuteDef ID in Object Manager")

    if ( !MuteDefTranslator::retrieve(def_,ioobj,errmsg_) )
	mErrRet(errmsg_)
    
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
    par.get( sTaperLength(), taperlen_ );
    par.getYN( sTailMute(), tail_ );

    MultiID mid;
    if ( par.get(sMuteDef(),mid) && !setMuteDefID(mid) )
    {
	errmsg_ = "No Mute definition ID found";
	return false;
    }

    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return false; }

bool Mute::doWork( int start, int stop, int )
{
    if ( !muter_ )
	return false;

    const int nrsamples = input_->data().info().getSize(Gather::zDim());
    for ( int ioffs=start; ioffs<=stop; ioffs++ )
    {
	const float offs = input_->getOffset(ioffs);
	const float mutez = def_.value( offs, input_->getBinID() );
	const float mutepos = Muter::mutePos( mutez, input_->posData().range(false) );

	float* out = output_->data().get1D(&ioffs);
	muter_->mute( out, nrsamples, mutepos );
    }

    return true;
}
