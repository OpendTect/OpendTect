/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID mUnusedVar = "$Id: seiscube2linedata.cc,v 1.8 2012-07-06 06:23:24 cvsraman Exp $";

#include "seiscube2linedata.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seis2dline.h"
#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "ioobj.h"


struct Cube2LineDataLineKeyProvider : public LineKeyProvider
{
Cube2LineDataLineKeyProvider( SeisCube2LineDataExtracter& lde ) : lde_(lde) {}

LineKey lineKey() const
{
    return LineKey( lde_.lidx_ >= 0 && lde_.lidx_ < lde_.ls_.nrLines() ?
	    	    lde_.ls_.lineName(lde_.lidx_) : "", lde_.attrnm_ );
}

    SeisCube2LineDataExtracter&	lde_;

};


SeisCube2LineDataExtracter::SeisCube2LineDataExtracter(
			const IOObj& cubein, const IOObj& lsout,
			const char* attrnm, const BufferStringSet* lnms )
    : Executor("Extract 3D data into 2D lines")
    , attrnm_(attrnm)
    , tbuf_(*new SeisTrcBuf(true))
    , rdr_(*new SeisTrcReader(&cubein))
    , ls_(*new Seis2DLineSet(lsout))
    , wrr_(*new SeisTrcWriter(&lsout))
    , fetcher_(0)
    , nrdone_(0)
    , lidx_(-1)
    , c2ldlkp_(0)
{
    if ( lnms ) lnms_ = *lnms;

    if ( !rdr_.prepareWork() )
	msg_ = rdr_.errMsg();
    else if ( ls_.nrLines() < 1 )
	msg_ = "Empty or invalid Line Set";
    else
    {
	c2ldlkp_ = new Cube2LineDataLineKeyProvider( *this );
	wrr_.setLineKeyProvider( c2ldlkp_ );
	lidx_ = 0;
	msg_ = "Handling traces";
    }
}


SeisCube2LineDataExtracter::~SeisCube2LineDataExtracter()
{
    closeDown();
    delete &tbuf_; delete &wrr_; delete &rdr_; delete &ls_;
}


void SeisCube2LineDataExtracter::closeDown()
{
    tbuf_.deepErase();
    rdr_.close(); wrr_.close();
    delete fetcher_; fetcher_ = 0;
    delete c2ldlkp_; c2ldlkp_ = 0;
    wrr_.setLineKeyProvider(0);
}


int SeisCube2LineDataExtracter::nextStep()
{
    if ( lidx_ < 0 )
	return ErrorOccurred();
    else if ( lidx_ >= ls_.nrLines() )
	{ closeDown(); return Finished(); }
    else if ( !fetcher_ )
	return getNextFetcher() ? MoreToDo() : ErrorOccurred();

    int res = fetcher_->doStep();
    if ( res != 1 )
    {
	if ( res > 1 )
	    return res;
	else if ( res == 0 )
	{
	    delete fetcher_; fetcher_ = 0;
	    lidx_++;
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


bool SeisCube2LineDataExtracter::getNextFetcher()
{
    for ( ; lidx_<ls_.nrLines(); lidx_++ )
    {
	const BufferString lnm = ls_.lineName( lidx_ );
	if ( lineshandled_.isPresent( lnm ) )
	    continue;
	if ( !lnms_.isEmpty() && !lnms_.isPresent(lnm) )
	    continue;

	int inplidx = -1;
	const LineKey deflk( lnm );
	if ( ls_.lineKey(lidx_) == deflk )
	    inplidx = lidx_;
	else
	{
	    for ( int iln=0; iln<ls_.nrLines(); iln++ )
	    {
		if ( ls_.lineKey(iln) == deflk )
		    { inplidx = iln; break; }
	    }
	}
	if ( inplidx < 0 )
	    inplidx = lidx_;

	fetcher_ = ls_.lineFetcher( inplidx, tbuf_, 1 );
	lineshandled_.add( lnm );
	return true;
    }

    return false;
}


int SeisCube2LineDataExtracter::handleTrace()
{
    PtrMan<SeisTrc> trc = tbuf_.remove( 0 );
    SeisTrcInfo ti( trc->info() );

    if ( !rdr_.seisTranslator()->goTo( SI().transform(trc->info().coord) ) )
	return MoreToDo();

    SeisTrc trc3d;
    if ( !rdr_.get(trc3d) )
	return MoreToDo();

    ti.sampling = trc3d.info().sampling;
    trc3d.info() = ti;
    if ( !wrr_.put(trc3d) )
	{ msg_ = wrr_.errMsg(); return ErrorOccurred(); }

    nrdone_++;
    return MoreToDo();
}
