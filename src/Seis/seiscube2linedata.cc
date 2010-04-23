/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID = "$Id: seiscube2linedata.cc,v 1.1 2010-04-23 12:45:58 cvsbert Exp $";

#include "seiscube2linedata.h"
#include "seisread.h"
#include "seisbuf.h"
#include "seis2dline.h"
#include "seis2dlineio.h"
#include "ioobj.h"


SeisCube2LineDataExtracter::SeisCube2LineDataExtracter(
			const IOObj& cubein, const IOObj& lsout,
			const char* attrnm, const BufferStringSet* lnms )
    : Executor("Extract 3D data into 2D lines")
    , nrdone_(0)
    , tbuf_(*new SeisTrcBuf(true))
    , rdr_(*new SeisTrcReader(&cubein))
    , ls_(*new Seis2DLineSet(lsout))
    , fetcher_(0)
    , putter_(0)
    , lidx_(-1)
{
    if ( lnms ) lnms_ = *lnms;

    if ( !rdr_.prepareWork() )
	msg_ = rdr_.errMsg();
    else if ( ls_.nrLines() < 1 )
	msg_ = "Empty or invalid Line Set";
    else
    {
	lidx_ = 0;
	msg_ = "Handling traces";
    }
}


SeisCube2LineDataExtracter::~SeisCube2LineDataExtracter()
{
    delete &tbuf_;
}


int SeisCube2LineDataExtracter::nextStep()
{
    if ( lidx_ < 0 )
	return ErrorOccurred();
    else if ( lidx_ >= ls_.nrLines() )
	return Finished();

    if ( !fetcher_ )
    {
	msg_ = "TODO: create fetcher_ and putter_";
	return ErrorOccurred();
    }

    int res = fetcher_->doStep();
    if ( res != 1 )
    {
	if ( res > 1 )
	    return res;
	else if ( res == 0 )
	{
	    delete fetcher_; fetcher_ = 0;
	    delete putter_; putter_ = 0;
	    return MoreToDo();
	}
	else
	{
	    msg_ = fetcher_->message();
	    return ErrorOccurred();
	}
    }

    res = handleTrace();
    msg_ = fetcher_->message();
    return res;
}


int SeisCube2LineDataExtracter::handleTrace()
{
    //TODO
    nrdone_++;
    return MoreToDo();
}
