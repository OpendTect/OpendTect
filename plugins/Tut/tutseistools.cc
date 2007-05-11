
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/

static const char* rcsID = "$Id: tutseistools.cc,v 1.2 2007-05-11 12:43:42 cvsbert Exp $";

#include "tutseistools.h"
#include "seisread.h"
#include "seiswrite.h"
#include "ioobj.h"


Tut::SeisTools::SeisTools()
    : Executor("Tut Seismic Tools")
    , inioobj_(0), outioobj_(0)
    , action_(Scale)
    , factor_(1), shift_(0)
    , weaksmooth_(false)
    , totnr_(-1)
    , nrdone_(0)
    , rdr_(0)
    , wrr_(0)
{
}


Tut::SeisTools::~SeisTools()
{
    delete inioobj_; delete outioobj_;
    delete rdr_; delete wrr_;
}


void Tut::SeisTools::setInput( const IOObj& ioobj )
{
    delete inioobj_; inioobj_ = ioobj.clone();
}


void Tut::SeisTools::setOutput( const IOObj& ioobj )
{
    delete outioobj_; outioobj_ = ioobj.clone();
}


const char* Tut::SeisTools::message() const
{
    static const char* acts[] = { "Scaling", "Squaring", "Smoothing" };
    return errmsg_.isEmpty() ? acts[action_] : errmsg_.buf();
}


int Tut::SeisTools::totalNr() const
{
    if ( totnr_ >= 0 || !inioobj_ )
	return totnr_;

    // SeisIOObjInfo ioobjinfo( *inioobj_ );
    return totnr_;
}


#define mOpenIO(clss,obj,ioobj) \
    if ( !obj ) \
    { \
	obj = new clss( ioobj ); \
	const char* emsg = obj->errMsg(); \
	if ( emsg && *emsg ) \
	{ \
	    errmsg_ = emsg; \
	    return Executor::ErrorOccurred; \
	} \
	return Executor::MoreToDo; \
    }

int Tut::SeisTools::nextStep()
{
    mOpenIO( SeisTrcReader, rdr_, inioobj_ );
    mOpenIO( SeisTrcWriter, wrr_, outioobj_ );

    return Executor::Finished;
}
