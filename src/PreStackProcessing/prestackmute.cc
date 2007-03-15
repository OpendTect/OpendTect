/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackmute.cc,v 1.1 2007-03-15 17:28:52 cvskris Exp $";

#include "prestackmute.h"

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

    if ( id_.isEmpty() )
	mErrRet("No MuteDef ID provided")
    PtrMan<IOObj> ioobj = IOM().get( id_ );
    if ( !ioobj ) 
	mErrRet("Cannot find MuteDef ID in Object Manager")

    if ( !MuteDefTranslator::retrieve(def_,ioobj,errmsg_) )
	mErrRet(errmsg_)

    muter_ = new Muter( taperlen_, tail_ );
    return true;
}


void Mute::fillPar( IOPar& par ) const
{
    par.set( sMuteDef(), id_ );
    par.set( sTaperLength(), taperlen_ );
    par.setYN( sTailMute(), tail_ );
}


bool Mute::usePar( const IOPar& par )
{
    par.get( sTaperLength(), taperlen_ );
    par.getYN( sTailMute(), tail_ );
    if ( !par.get(sMuteDef(),id_) && id_.isEmpty() )
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
	const float offs = input_->offsets()[ ioffs ];
	const float mutez = def_.value( offs, input_->getBinID() );
	const float mutepos = Muter::mutePos( mutez, input_->zSampling() );

	float* out = output_->data().get1D(&ioffs);
	muter_->mute( out, nrsamples, mutepos );
    }

    return true;
}
